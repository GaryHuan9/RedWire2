#include "Utility/Functions.hpp"

#include <GL/glew.h>

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

}
