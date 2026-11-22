#pragma once
#include <stdbool.h>
#include <limits.h>

#define MAX_LINE_LENGTH  10000
#define LONGLONG_MIN LLONG_MIN
#define LONGLONG_MAX LLONG_MAX

typedef struct cell cell;

//Bottom left corner of the camera, and the number of visible cells (changes with zoom)
extern long long camPosX;
extern long long camPosY;
extern int camSizeX;
extern int camSizeY;

struct cell
{
	long long x;
	long long y;
	cell* next;
	bool valid;
};

cell* createCell(long long x, long long y);
void deleteCell(cell* c); //Deletes the whole list
cell* gameTick(cell* c); //Returns the next generation as a new list
cell* parseFile(const char* filePath);

bool isCellAlive(cell* c, long long x, long long y);
cell* setCell(cell* c, long long x, long long y, bool alive); //Returns the new head of the list
unsigned int countCells(cell* c);
