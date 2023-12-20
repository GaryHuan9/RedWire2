#include "Interface/Application.hpp"
#include "Interface/Components.hpp"
#include "Functional/Board.hpp"

#include "SFML/System.hpp"
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "imgui-SFML.h"
#include "imgui.h"

namespace rw
{

static void configure_spacing(ImGuiStyle& style);
static void configure_colors(ImGuiStyle& style);

Application::Application()
{
	bool success = true;

	sf::VideoMode video_mode(1920, 1080);
	sf::ContextSettings settings;
	settings.antialiasingLevel = 16;

	window = std::make_unique<sf::RenderWindow>(video_mode, "RedWire2", sf::Style::Default, settings);
	window->setVerticalSyncEnabled(true);
	success |= ImGui::SFML::Init(*window, false);

	auto& style = ImGui::GetStyle();
	configure_spacing(style);
	configure_colors(style);

	auto& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.Fonts->AddFontFromFileTTF("ext/JetBrainsMono/JetBrainsMono-Bold.ttf", 16.0f);
	success |= ImGui::SFML::UpdateFontTexture();
	assert(success);

	make_component<LayerView>();
	make_component<Controller>();

#ifndef NDEBUG
	make_component<Debugger>();
#endif
}

Application::~Application()
{
	ImGui::SFML::Shutdown();
}

void Application::run()
{
	sf::Clock clock;
	Timer timer;

	for (auto& component : components) component->initialize();

	while (true)
	{
		sf::Event event{};
		while (window->pollEvent(event)) process_event(event);
		if (!window->isOpen()) break;

		sf::Time time = clock.restart();
		timer.update(time.asMicroseconds());
		ImGui::SFML::Update(*window, time);
		window->clear(sf::Color::Black);

		for (auto& component : components) component->update(timer);

		//		ImGui::ShowDemoWindow();
		ImGui::SFML::Render(*window);
		window->display();
	}
}

bool Application::handle_mouse() const
{
	return not ImGui::GetIO().WantCaptureMouse && window->hasFocus();
}

bool Application::handle_keyboard() const
{
	return not ImGui::GetIO().WantCaptureKeyboard && window->hasFocus();
}

void Application::process_event(const sf::Event& event)
{
	ImGui::SFML::ProcessEvent(*window, event);

	auto distribute = [this](const sf::Event& event)
	{
		for (auto& component : components) component->input_event(event);
	};

	switch (event.type)
	{
		case sf::Event::Closed:
		{
			window->close();
			break;
		}
		case sf::Event::Resized:
		{
			sf::Vector2f size(sf::Vector2u(event.size.width, event.size.height));
			window->setView(sf::View(size / 2.0f, size));
			distribute(event);
			break;
		}
		case sf::Event::MouseMoved:
		case sf::Event::MouseWheelScrolled:
		case sf::Event::MouseButtonPressed:
		case sf::Event::MouseButtonReleased:
		{
			if (not handle_mouse()) break;
			distribute(event);
		}
		case sf::Event::KeyPressed:
		case sf::Event::KeyReleased:
		{
			if (not handle_keyboard()) break;
			distribute(event);
		}
		default: break;
	}
}

Component::Component(Application& application) : application(application), window(*application.get_window()) {}

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
	const ImVec4 main = ImVec4(0.07450981f, 0.93333334f, 0.34117648, 1.0f);
	const ImVec4 white1 = ImVec4(0.98039216f, 0.9843137f, 1.0f, 1.0f);
	const ImVec4 white0 = ImVec4(0.627098f, 0.9644314f, 0.7430588, 1.0f);
	const ImVec4 background0 = ImVec4(0.078431375f, 0.08235294f, 0.09019608, 1.0f);
	const ImVec4 background1 = ImVec4(0.13725491f, 0.15294118f, 0.18039216, 1.0f);
	const ImVec4 contrast = ImVec4(0.21568628f, 0.23137255f, 0.24705882, 1.0f);

	auto with_alpha = [](const ImVec4& value, float alpha) { return ImVec4(value.x, value.y, value.z, alpha); };

	style.Colors[ImGuiCol_Text] = white1;
	style.Colors[ImGuiCol_TextDisabled] = white0;
	style.Colors[ImGuiCol_WindowBg] = background0;
	style.Colors[ImGuiCol_PopupBg] = background0;
	style.Colors[ImGuiCol_Border] = with_alpha(main, Alpha1);
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
	style.Colors[ImGuiCol_Tab] = background0;
	style.Colors[ImGuiCol_TabHovered] = main;
	style.Colors[ImGuiCol_TabActive] = main;
	style.Colors[ImGuiCol_TabUnfocused] = background0;
	style.Colors[ImGuiCol_TabUnfocusedActive] = contrast;
	style.Colors[ImGuiCol_DockingPreview] = contrast;
	style.Colors[ImGuiCol_DockingEmptyBg] = background0;
	style.Colors[ImGuiCol_PlotLines] = main;
	style.Colors[ImGuiCol_PlotLinesHovered] = white0;
	style.Colors[ImGuiCol_PlotHistogram] = main;
	style.Colors[ImGuiCol_PlotHistogramHovered] = white0;
	style.Colors[ImGuiCol_TableHeaderBg] = background1;
	style.Colors[ImGuiCol_TableBorderStrong] = with_alpha(main, Alpha1);
	style.Colors[ImGuiCol_TableBorderLight] = with_alpha(main, Alpha1);
	style.Colors[ImGuiCol_TableRowBgAlt] = with_alpha(background1, Alpha0);
	style.Colors[ImGuiCol_TextSelectedBg] = with_alpha(white1, Alpha0);
	style.Colors[ImGuiCol_DragDropTarget] = with_alpha(white1, Alpha1);
	style.Colors[ImGuiCol_NavHighlight] = with_alpha(white1, Alpha1);
	style.Colors[ImGuiCol_NavWindowingHighlight] = with_alpha(white1, Alpha1);
	style.Colors[ImGuiCol_NavWindowingDimBg] = with_alpha(white1, Alpha0);
	style.Colors[ImGuiCol_ModalWindowDimBg] = with_alpha(white1, Alpha0);
}

}
