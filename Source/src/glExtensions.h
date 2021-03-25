#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdbool.h>

static void GLClearError() //Clear all errors
{
	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line) // Print error
{
	GLenum error;

	while (error = glGetError())
	{
		printf("[OpenGL Error] (");
		printf("%d): ", error);
		printf("%s ", function);
		printf("%s ", file);
		printf("%d\n", line);
		return false;
	}
	return true;
}