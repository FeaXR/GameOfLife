#pragma once
#include <stdbool.h>

// Game state shared between the keyboard handler and the toolbar (defined in main.c)
extern bool gameLoop; //true while the game is running
extern bool showHalo;
extern unsigned int timePerTick; //ms between game ticks
extern float red, green, blue;

// Actions, bound to both keys and toolbar buttons (implemented in main.c)
void moveCamera(int dx, int dy); //dx, dy in {-1, 0, 1}
void togglePause(void);
void stepGame(void); //Advance one generation, only while paused
void slowDown(void);
void speedUp(void);
void cycleColor(float* channel); //Step a color channel by 25%, wrapping to 0
void toggleHalo(void);
