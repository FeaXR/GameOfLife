#pragma once
#define MAX_LINE_LENGTH  10000
#define camSizeX 90
#define camSizeY 45
#define LONGLONG_MIN -9223372036854775807
#define LONGLONG_MAX 9223372036854775807

typedef struct cell cell;
typedef struct place place;
typedef struct positionOnScreen positionOnScreen;

long long camPosX;
long long camPosY;

struct cell
{
	long long x;
	long long y;
	cell* next;
	bool valid;
};

struct place
{
	long long x;
	long long y;
	unsigned int neighbourCount;
	bool isALive;
	place* next;
};

struct positionOnScreen
{
	unsigned int x;
	unsigned int y;
	positionOnScreen* next;
};

cell* createCell(long long x, long long y);
place* createPlace(long long x, long long y, bool isALive);
place* addPlaceToPlaceList(long long x, long long y, place* plc);
place* addCellToPlaceList(long long x, long long y, place* plc);
void deleteCell(cell* c);
void deletePlace(place* p);
void deletePositionOnScreen(positionOnScreen* p);
cell* getCellsFromPlaceList(place* p);
cell* gameTick(cell* c);
void drawToConsole(cell cl);
cell* parseFile(const char* filePath);
positionOnScreen* getCellPositions(cell* cl);
positionOnScreen* addToPositionListIfDoesntExist(positionOnScreen* places, int x, int y, unsigned int* count);
positionOnScreen* getHaloPositions(unsigned int* count, positionOnScreen* places);