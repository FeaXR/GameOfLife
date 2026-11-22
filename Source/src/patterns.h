#pragma once

typedef struct Pattern Pattern;

struct Pattern
{
	const char* category;
	const char* name;
	int width;
	int height;
	const char* rows; //Rows from top to bottom separated by '\n', 'O' is a live cell
};

extern const Pattern patterns[];
extern const int patternCount;
