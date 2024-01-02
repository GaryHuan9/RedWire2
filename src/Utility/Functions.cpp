#include "Utility/Functions.hpp"

#include "GL/glew.h"
#include "imgui.h"

namespace rw
{

void throw_any_gl_error()
{
	GLenum error;

	while (true)
	{
		error = glGetError();
		if (error == GL_NO_ERROR) return;
		std::string string_error(reinterpret_cast<const char*>(glewGetErrorString(error)));
		throw std::runtime_error("An OpenGL error occurred: " + string_error + " (" + std::to_string(error) + ").");
	}
}

bool imgui_begin(const char* label)
{
	if (ImGui::Begin(label, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) return true;

	ImGui::End();
	return false;
}

bool imgui_tooltip(const char* text)
{
	static constexpr auto Flags = ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_Stationary;
	if (not ImGui::IsItemHovered(Flags)) return false;
	if (not ImGui::BeginTooltip()) return false;

	float width = ImGui::GetIO().DisplaySize.x * 0.18f;
	ImGui::PushTextWrapPos(width);
	ImGui::TextUnformatted(text);
	ImGui::PopTextWrapPos();

	ImGui::EndTooltip();
	return true;
}

}
