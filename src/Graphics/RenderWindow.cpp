#include "RenderWindow.hpp"
#include "Utility/BasicTypes.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__linux__)
#define REDWIRE2_WAYLAND
// #define REDWIRE2_X11 //TODO: need to enable if compiling for X11, not sure how to automatically detect yet
#elif defined(__CYGWIN__) || defined(_WIN64) || defined(_WIN32)
#define REDWIER2_WIN32
#elif defined(__APPLE__) && defined(__MACH__)
#define REDWIER2_COCOA
#else
#error "Unsupported targeted platform!"
#endif

namespace rw
{
static GLFWwindow* create_window(Int2 size, const std::string& name);
static void create_renderer(Int2 size, GLFWwindow* handle);
static std::unordered_map<std::string, bgfx::ProgramHandle> load_shaders();

constexpr auto ResetFlags = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X16;

RenderWindow::RenderWindow(Int2 size, const std::string& name) : handle(create_window(size, name)), window_size(size)
{
    create_renderer(size, handle);
    shaders = load_shaders();
    initialize();
}

RenderWindow::~RenderWindow()
{
    for (bgfx::ProgramHandle shader : shaders | std::views::values) bgfx::destroy(shader);
    shaders.clear();

    bgfx::shutdown();
    glfwDestroyWindow(handle);
    handle = nullptr;
    glfwTerminate();
}

bgfx::ProgramHandle RenderWindow::get_shader(const std::string& name) const
{
    auto iterator = shaders.find(name);
    if (iterator != shaders.end()) return iterator->second;
    throw std::runtime_error("Unknown shader " + name + '.');
}

bgfx::ViewId RenderWindow::get_view_id(RenderLayer layer) const
{
    auto index = static_cast<uint32_t>(layer);
    if (index < view_ids.size()) return view_ids[index];
    throw std::runtime_error("Unknown RenderLayer " + index);
}

void RenderWindow::update()
{
    Int2 current_size;
    glfwGetWindowSize(handle, &current_size.x, &current_size.y);
    if (current_size == window_size) return;

    window_size = current_size;

    bgfx::reset(window_size.x, window_size.y, ResetFlags);

    for (bgfx::ViewId view_id : view_ids) bgfx::setViewRect(view_id, 0, 0, bgfx::BackbufferRatio::Equal);
}

void RenderWindow::render()
{
    bgfx::frame();
    for (bgfx::ViewId view_id : view_ids) bgfx::touch(view_id);
}

void RenderWindow::initialize()
{
    view_ids.resize(3);
    view_ids[static_cast<uint32_t>(RenderLayer::Background)] = 10;
    view_ids[static_cast<uint32_t>(RenderLayer::Components)] = 20;
    view_ids[static_cast<uint32_t>(RenderLayer::Interface)] = 15;

    for (bgfx::ViewId view_id : view_ids)
    {
        bgfx::setViewRect(view_id, 0, 0, bgfx::BackbufferRatio::Equal);
        bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);
    }

    bgfx::setViewClear(get_view_id(RenderLayer::Background), BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
}

static GLFWwindow* create_window(Int2 size, const std::string& name)
{
    //Initialize GLFW
    glfwSetErrorCallback([](int code, const char* description)
    {
        std::string error(description);
        error += '(' + std::to_string(code) + ')';
        throw std::runtime_error(error);
    });

    if (not glfwInit()) throw std::runtime_error("Unable to initialize GLFW.");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* handle = glfwCreateWindow(size.x, size.y, name.c_str(), nullptr, nullptr);
    if (handle == nullptr) throw std::runtime_error("Failed to create GLFW window.");
    return handle;
}

#if defined(REDWIRE2_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif defined(REDWIRE2_X11)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(REDWIER2_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(REDWIER2_COCOA)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

static void create_renderer(Int2 size, GLFWwindow* handle)
{
    //Find preferred renderer
    std::vector<bgfx::RendererType::Enum> renderers(bgfx::RendererType::Count);
    auto count = bgfx::getSupportedRenderers(renderers.size(), renderers.data());
    renderers.resize(count);

    auto try_candidate = [&renderers](bgfx::RendererType::Enum& result, bgfx::RendererType::Enum candidate)
    {
        bool supported = std::ranges::find(renderers, candidate) != renderers.end();
        if (supported) result = candidate;
        return supported;
    };

    bgfx::RendererType::Enum renderer;
    bool has_renderer = try_candidate(renderer, bgfx::RendererType::Vulkan) ||
                        try_candidate(renderer, bgfx::RendererType::OpenGL) ||
                        try_candidate(renderer, bgfx::RendererType::OpenGLES);

    if (not has_renderer) throw std::runtime_error("Cannot find supported renderer on the current platform.");

    //Initialize BGFX
    auto get_display_handle = []
    {
#if defined(REDWIRE2_WAYLAND)
        return glfwGetWaylandDisplay();
#elif defined(REDWIRE2_X11)
        return glfwGetX11Display();
#else
        return nullptr;
#endif
    };

    auto get_window_handle = [](GLFWwindow* pointer)
    {
#if defined(REDWIRE2_WAYLAND)
        return glfwGetWaylandWindow(pointer);
#elif defined(REDWIRE2_X11)
        return reinterpret_cast<void*>(glfwGetX11Window(pointer));
#elif defined(REDWIER2_WIN32)
        return glfwGetWin32Window(pointer);
#elif defined(REDWIER2_COCOA)
        return glfwGetCocoaWindow(pointer);
#else
        return nullptr;
#endif
    };

    auto get_window_handle_type = []
    {
#if defined(REDWIRE2_WAYLAND)
        return bgfx::NativeWindowHandleType::Wayland;
#else
        return bgfx::NativeWindowHandleType::Default;
#endif
    };

    bgfx::renderFrame();
    bgfx::Init init{};

#ifndef NDEBUG
    init.debug = true;
#endif

    init.type = renderer;
    init.resolution.width = size.x;
    init.resolution.height = size.y;
    init.resolution.reset = BGFX_RESET_VSYNC;

    init.platformData.ndt = get_display_handle();
    init.platformData.nwh = get_window_handle(handle);
    init.platformData.type = get_window_handle_type();

    if (not bgfx::init(init)) throw std::runtime_error("Unable to initialize BGFX.");

    bgfx::reset(size.x, size.y, ResetFlags);
}

static std::unordered_map<std::string, bgfx::ProgramHandle> load_shaders()
{
    namespace fs = std::filesystem;
    std::unordered_map<std::string, bgfx::ProgramHandle> result;

    auto get_shader_folder = []
    {
        switch (bgfx::getRendererType())
        {
            case bgfx::RendererType::Vulkan: return "spirv";
            case bgfx::RendererType::OpenGL: return "glsl";
            case bgfx::RendererType::OpenGLES: return "essl";
            default: throw std::runtime_error("Bad renderer.");
        }
    };

    for (const auto& entry : fs::directory_iterator(fs::path("Shaders") / get_shader_folder()))
    {
        if (not entry.is_regular_file()) continue;
        std::string name = entry.path().filename().string();
        name.resize(name.find_first_of('.'));

        if (result.contains(name)) continue;

        auto load_memory = [](const fs::path& path)
        {
            std::ifstream stream(path, std::ios::binary);
            std::vector bytes((std::istreambuf_iterator(stream)),
                              (std::istreambuf_iterator<char>()));

            void* data = std::malloc(bytes.size());
            std::ranges::copy(bytes, static_cast<char*>(data));
            return bgfx::makeRef(data, bytes.size(), [](void* pointer, void*) { std::free(pointer); });
        };

        fs::path path = entry.path();

        auto shader = bgfx::createProgram(
            bgfx::createShader(load_memory(path.replace_filename(name + ".vert.bin"))),
            bgfx::createShader(load_memory(path.replace_filename(name + ".frag.bin"))),
            true);

        auto emplace = result.emplace(name, shader);
        assert(emplace.second);
    }

    return result;
}
} // rw
