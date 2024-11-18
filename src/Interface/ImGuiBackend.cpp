#include "Interface/ImGuiBackend.hpp"
#include "Interface/RenderWindow.hpp"
#include "Utility/SimpleTypes.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <bgfx/bgfx.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace rw
{
static void configure_spacing(ImGuiStyle& style);
static void configure_colors(ImGuiStyle& style);

ImGuiBackend::ImGuiBackend(const RenderWindow& window) : window(window), handle(window.get_handle())
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = "rsc/imgui.ini";

    ImGui_ImplGlfw_InitForOther(handle, true);

    //Create vertex layout
    vertex_layout
            .begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

    //Create font texture
    unsigned char* font_data;
    int font_width, font_height;
    io.Fonts->AddFontFromFileTTF("Assets/JetBrainsMono/JetBrainsMono-Bold.ttf", 16.0f);
    io.Fonts->GetTexDataAsRGBA32(&font_data, &font_width, &font_height);
    const bgfx::Memory* font_memory = bgfx::copy(font_data, font_width * font_height * 4);
    font_texture = bgfx::createTexture2D(font_width, font_height, false, 1, bgfx::TextureFormat::BGRA8, 0, font_memory);
    font_uniform = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    //Configure styles
    auto& style = ImGui::GetStyle();
    configure_spacing(style);
    configure_colors(style);
}

ImGuiBackend::~ImGuiBackend()
{
    bgfx::destroy(font_texture);
    bgfx::destroy(font_uniform);
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiBackend::update()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiBackend::render()
{
    ImGui::Render();

    bgfx::ProgramHandle shader = window.get_shader("ImGui");
    bgfx::ViewId view_id = window.get_view_id(RenderWindow::View::Interface);

    for (const ImDrawList* draw_list : ImGui::GetDrawData()->CmdLists)
    {
        uint32_t vertex_count = draw_list->VtxBuffer.size();
        uint32_t index_count = draw_list->IdxBuffer.size();

        if (not bgfx::getAvailTransientVertexBuffer(vertex_count, vertex_layout) ||
            not bgfx::getAvailTransientIndexBuffer(index_count))
        {
            std::printf("Unable to request transient buffer of size %d and %d for ImGui.", vertex_count, index_count);
            break;
        }

        bgfx::TransientVertexBuffer vertex_buffer{};
        bgfx::TransientIndexBuffer index_buffer{};

        bgfx::allocTransientVertexBuffer(&vertex_buffer, vertex_count, vertex_layout);
        bgfx::allocTransientIndexBuffer(&index_buffer, index_count);

        std::ranges::copy(draw_list->VtxBuffer, reinterpret_cast<ImDrawVert*>(vertex_buffer.data));
        std::ranges::copy(draw_list->IdxBuffer, reinterpret_cast<ImDrawIdx*>(index_buffer.data));

        uint32_t offset = 0;

        for (const ImDrawCmd& command : draw_list->CmdBuffer)
        {
            if (command.UserCallback != nullptr)
            {
                command.UserCallback(draw_list, &command);
                continue;
            }

            if (command.ElemCount == 0) continue;

            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA |
                           BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));

            auto convert = [](float value)
            {
                constexpr float Min = std::numeric_limits<uint16_t>::min();
                constexpr float Max = std::numeric_limits<uint16_t>::max();
                return static_cast<uint16_t>(std::lround(std::min(std::max(value, Min), Max)));
            };

            uint16_t min_x = convert(command.ClipRect.x);
            uint16_t min_y = convert(command.ClipRect.y);
            uint16_t size_x = convert(command.ClipRect.z - command.ClipRect.x);
            uint16_t size_y = convert(command.ClipRect.w - command.ClipRect.y);
            bgfx::setScissor(min_x, min_y, size_x, size_y);

            bgfx::TextureHandle texture = font_texture;
            if (command.TextureId != nullptr) texture.idx = reinterpret_cast<uintptr_t>(command.TextureId);
            bgfx::setTexture(0, font_uniform, texture);

            bgfx::setVertexBuffer(0, &vertex_buffer, 0, vertex_count);
            bgfx::setIndexBuffer(&index_buffer, offset, command.ElemCount);
            bgfx::submit(view_id, shader);
            offset += command.ElemCount;
        }
    }
}

static void configure_spacing(ImGuiStyle& style)
{
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(8.0f, 2.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.ItemSpacing = ImVec2(4.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;

    style.WindowRounding = 1.0f;
    style.ChildRounding = 1.0f;
    style.FrameRounding = 1.0f;
    style.PopupRounding = 1.0f;
    style.ScrollbarRounding = 1.0f;
    style.GrabRounding = 1.0f;
    style.LogSliderDeadzone = 1.0f;
    style.TabRounding = 1.0f;

    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ColorButtonPosition = ImGuiDir_Left;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
}

static void configure_colors(ImGuiStyle& style)
{
    constexpr float Alpha0 = 0.33f;
    constexpr float Alpha1 = 0.61f;
    ImVec4 main(0.8666667f, 0.26666668f, 0.29803923f, 1.0f);
    ImVec4 white0(0.9360392f, 0.7044314f, 0.72623533f, 1.0f);
    ImVec4 white1(0.98039216f, 0.9843137f, 1.0f, 1.0f);
    ImVec4 background0(0.078431375f, 0.08235294f, 0.09019608, 1.0f);
    ImVec4 background1(0.13725491f, 0.15294118f, 0.18039216, 1.0f);
    ImVec4 contrast(0.21568628f, 0.23137255f, 0.24705882, 1.0f);

    auto with_alpha = [](const ImVec4& value, float alpha) { return ImVec4(value.x, value.y, value.z, alpha); };

    style.Colors[ImGuiCol_Text] = white1;
    style.Colors[ImGuiCol_TextDisabled] = white0;
    style.Colors[ImGuiCol_WindowBg] = background0;
    style.Colors[ImGuiCol_ChildBg] = ImVec4();
    style.Colors[ImGuiCol_PopupBg] = background0;
    style.Colors[ImGuiCol_Border] = with_alpha(main, Alpha1);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4();
    style.Colors[ImGuiCol_FrameBg] = ImVec4();
    style.Colors[ImGuiCol_FrameBgHovered] = contrast;
    style.Colors[ImGuiCol_FrameBgActive] = main;
    style.Colors[ImGuiCol_TitleBg] = background0;
    style.Colors[ImGuiCol_TitleBgActive] = contrast;
    style.Colors[ImGuiCol_TitleBgCollapsed] = background0;
    style.Colors[ImGuiCol_MenuBarBg] = background1;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4();
    style.Colors[ImGuiCol_ScrollbarGrab] = background1;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = contrast;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = main;
    style.Colors[ImGuiCol_CheckMark] = main;
    style.Colors[ImGuiCol_SliderGrab] = main;
    style.Colors[ImGuiCol_SliderGrabActive] = white0;
    style.Colors[ImGuiCol_Button] = ImVec4();
    style.Colors[ImGuiCol_ButtonHovered] = contrast;
    style.Colors[ImGuiCol_ButtonActive] = main;
    style.Colors[ImGuiCol_Header] = ImVec4();
    style.Colors[ImGuiCol_HeaderHovered] = contrast;
    style.Colors[ImGuiCol_HeaderActive] = main;
    style.Colors[ImGuiCol_Separator] = background1;
    style.Colors[ImGuiCol_SeparatorHovered] = contrast;
    style.Colors[ImGuiCol_SeparatorActive] = main;
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4();
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4();
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4();
    style.Colors[ImGuiCol_TabHovered] = main;
    style.Colors[ImGuiCol_Tab] = background0;
    style.Colors[ImGuiCol_TabSelected] = main;
    style.Colors[ImGuiCol_TabSelectedOverline] = ImVec4();
    style.Colors[ImGuiCol_TabDimmed] = background0;
    style.Colors[ImGuiCol_TabDimmedSelected] = contrast;
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4();
    style.Colors[ImGuiCol_DockingPreview] = contrast;
    style.Colors[ImGuiCol_DockingEmptyBg] = background0;
    style.Colors[ImGuiCol_PlotLines] = main;
    style.Colors[ImGuiCol_PlotLinesHovered] = white0;
    style.Colors[ImGuiCol_PlotHistogram] = main;
    style.Colors[ImGuiCol_PlotHistogramHovered] = white0;
    style.Colors[ImGuiCol_TableHeaderBg] = background1;
    style.Colors[ImGuiCol_TableBorderStrong] = with_alpha(main, Alpha1);
    style.Colors[ImGuiCol_TableBorderLight] = with_alpha(main, Alpha1);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4();
    style.Colors[ImGuiCol_TableRowBgAlt] = with_alpha(background1, Alpha0);
    style.Colors[ImGuiCol_TextLink] = white0;
    style.Colors[ImGuiCol_TextSelectedBg] = with_alpha(white1, Alpha0);
    style.Colors[ImGuiCol_DragDropTarget] = with_alpha(white1, Alpha1);
    style.Colors[ImGuiCol_NavHighlight] = with_alpha(white1, Alpha1);
    style.Colors[ImGuiCol_NavWindowingHighlight] = with_alpha(white1, Alpha1);
    style.Colors[ImGuiCol_NavWindowingDimBg] = with_alpha(white1, Alpha0);
    style.Colors[ImGuiCol_ModalWindowDimBg] = with_alpha(white1, Alpha0);
}
} // rw
