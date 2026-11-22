#pragma once
#include <stdbool.h>

// Game state shared between the keyboard/mouse handlers and the toolbar (defined in main.c)
extern bool gameLoop; //true while the game is running
extern bool showHalo;
extern bool showGrid;
extern unsigned int timePerTick; //ms between game ticks
extern float red, green, blue;
extern int placingPattern; //Index into patterns[], -1 if not placing

// Actions, bound to keys, the mouse and toolbar buttons (implemented in main.c)
void moveCamera(int dx, int dy); //dx, dy in {-1, 0, 1}
void zoomIn(void);
void zoomOut(void);
bool canZoomIn(void);
bool canZoomOut(void);
int zoomPercent(void);
void togglePause(void);
void stepGame(void); //Advance one generation, only while paused
void slowDown(void);
void speedUp(void);
void cycleColor(float* channel); //Step a color channel by 25%, wrapping to 0
void toggleHalo(void);
void toggleGrid(void);
void clearBoard(void);

// Placing predefined patterns with the mouse
void startPlacing(int pattern);
void stopPlacing(void);
void rotatePlacing(void); //Clockwise
void flipPlacing(void); //Mirror horizontally
