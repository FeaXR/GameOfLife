#pragma once
// If function X return false, break
#define ASSERT(x) if(!(x)) __debugbreak();


//Clear errors before function X, and check if after X any errors emerged
#define GLCall(x) GLClearError(); \
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))