#define _CRT_SECURE_NO_WARNINGS
#define MAX_LINE_LENGTH  10000
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "shader.h"

ShaderProgramSource ParseShader(const char* filePath)
{
	ShaderType type = NONE;
	ShaderProgramSource src;
	FILE* file;

	char line[MAX_LINE_LENGTH] = { 0 };
	unsigned int line_count = 0;
	char* new_str;
	int tmp;
	char* readBuffer[2];
	readBuffer[0] = (char*)malloc(1);
	readBuffer[1] = (char*)malloc(1);
	*readBuffer[0] = '\0';
	*readBuffer[1] = '\0';

	src.FragmentSource = "";
	src.VertexSource = "";

	/* Open file */
	file = fopen(filePath, "r");

	if (!file)
	{
		perror(filePath);
		return src;
	}

	/* Get each line until there are none left */
	while (fgets(line, MAX_LINE_LENGTH, file))
	{
		if (strstr(line, "#shader") != NULL)
		{
			if (strstr(line, "vertex") != NULL)
			{
				type = VERTEX;
				continue;
			}

			if (strstr(line, "fragment") != NULL)
			{
				type = FRAGMENT;
				continue;
			}
			printf("Unsused shader found!\n");
			type = NONE;
			continue;
		}

		tmp = strlen(readBuffer[type]) + strlen(line);

		if ((new_str = malloc(tmp + 1)) != NULL)
		{
			new_str[0] = '\0';
			strcat(new_str, readBuffer[type]);
			strcat(new_str, line);
			free(readBuffer[type]);
			readBuffer[type] = (char*)malloc(tmp + 1);
			strcpy(readBuffer[type] == 0 ? " " : readBuffer[type], new_str);
			free(new_str);
			continue;
		}
		printf("Malloc failed!\n");
	}

	/* Close file */
	if (fclose(file))
	{
		return src;
		perror(filePath);
	}
	src.VertexSource = readBuffer[0];
	src.FragmentSource = readBuffer[1];
	return src;
}

unsigned int CompileShader(unsigned int type, const char* source)
{
	unsigned int id = glCreateShader(type);
	const char* src = source;

	int result;
	int length;
	char* message;

	glShaderSource(id, 1, &src, NULL);
	glCompileShader(id);

	glGetShaderiv(id, GL_COMPILE_STATUS, &result);

	if (result == GL_FALSE)
	{
		length = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		message = (char*)malloc(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);

		printf("Failed to compile ");
		printf((type == GL_VERTEX_SHADER) ? "vertex" : "fragment");
		printf(" shader! \n");
		printf(message == 0 ? " " : message);
		free(message);
		glDeleteShader(id);
		return 0;
	}
	return id;
}

unsigned int CreateShader(const char* vertexShader, const char* fragmentShader)
{
	unsigned int program = glCreateProgram();
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}