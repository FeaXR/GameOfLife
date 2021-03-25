#pragma once
#ifndef _shader_h
#define _shader_h

typedef struct ShaderProgramSource ShaderProgramSource;
typedef enum ShaderType ShaderType;

struct ShaderProgramSource
{
	char* VertexSource;
	char* FragmentSource;
};

enum ShaderType
{
	NONE = -1,
	VERTEX = 0,
	FRAGMENT = 1
};

ShaderProgramSource ParseShader(const char* filePath);
unsigned int CompileShader(unsigned int type, const char* source);
unsigned int CreateShader(const char* vertexShader, const char* fragmentShader);

#endif