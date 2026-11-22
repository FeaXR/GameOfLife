#pragma once

// Toolbar drawn with Nuklear at the top of the window
#define TOOLBAR_HEIGHT 40 //In window units, the grid is drawn below it

struct GLFWwindow;

void uiInit(struct GLFWwindow* window);
void uiFrame(void); //Process input and build the toolbar, call once per frame before drawing
void uiRender(void); //Draw the toolbar, call after drawing the grid
void uiShutdown(void);

//Forward key events to the UI (it needs them for its own input handling)
void uiKeyCallback(struct GLFWwindow* window, int key, int scancode, int action, int mods);
