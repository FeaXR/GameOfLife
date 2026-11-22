#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#include "ui.h"
#include "controls.h"
#include "gol.h"
#include "patterns.h"

#define MAX_VERTEX_BUFFER (512 * 1024)
#define MAX_ELEMENT_BUFFER (128 * 1024)
#define BUTTON_HEIGHT 28
#define CAMERA_REPEAT_DELAY 0.05 //Seconds between camera moves while an arrow button is held

static struct nk_glfw glfw;
static struct nk_context* ctx;
static double lastCameraMove;

void uiInit(struct GLFWwindow* window)
{
	struct nk_font_atlas* atlas;
	struct nk_font* font;

	ctx = nk_glfw3_init(&glfw, window, NK_GLFW3_INSTALL_CALLBACKS);

	nk_glfw3_font_stash_begin(&glfw, &atlas);
	font = nk_font_atlas_add_default(atlas, 16.0f, NULL);
	nk_glfw3_font_stash_end(&glfw);
	nk_style_set_font(ctx, &font->handle);
}

void uiShutdown(void)
{
	nk_glfw3_shutdown(&glfw);
}

void uiKeyCallback(struct GLFWwindow* window, int key, int scancode, int action, int mods)
{
	nk_glfw3_key_callback(window, key, scancode, action, mods);
}

void uiMouseButtonCallback(struct GLFWwindow* window, int button, int action, int mods)
{
	nk_glfw3_mouse_button_callback(window, button, action, mods);
}

void uiScrollCallback(struct GLFWwindow* window, double xoffset, double yoffset)
{
	nk_gflw3_scroll_callback(window, xoffset, yoffset);
}

bool uiWantsMouse(void)
{
	return nk_window_is_any_hovered(ctx) || nk_item_is_any_active(ctx);
}

//Show a tooltip if the mouse is over the given widget bounds
static void tooltip(struct nk_rect bounds, const char* text)
{
	if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
	{
		nk_tooltip(ctx, text);
	}
}

static int button(const char* label, const char* tip)
{
	struct nk_rect bounds = nk_widget_bounds(ctx);
	int pressed = nk_button_label(ctx, label);
	tooltip(bounds, tip);
	return pressed;
}

//Arrow button that keeps moving the camera while held down
static void cameraButton(enum nk_symbol_type symbol, int dx, int dy, const char* tip)
{
	struct nk_rect bounds = nk_widget_bounds(ctx);
	double now;

	nk_button_push_behavior(ctx, NK_BUTTON_REPEATER);
	if (nk_button_symbol(ctx, symbol))
	{
		now = glfwGetTime();
		if (now - lastCameraMove >= CAMERA_REPEAT_DELAY)
		{
			moveCamera(dx, dy);
			lastCameraMove = now;
		}
	}
	nk_button_pop_behavior(ctx);
	tooltip(bounds, tip);
}

static void colorButton(const char* name, float* channel, const char* tip)
{
	char label[16];
	snprintf(label, sizeof(label), "%s %d%%", name, (int)(*channel * 100.0f + 0.5f));
	if (button(label, tip))
	{
		cycleColor(channel);
	}
}

static void label(const char* text, float width)
{
	nk_layout_row_push(ctx, width);
	nk_label(ctx, text, NK_TEXT_CENTERED);
}

static void gap(void)
{
	nk_layout_row_push(ctx, 8);
	nk_spacing(ctx, 1);
}

//Button that can be greyed out
static int buttonIf(bool enabled, const char* label, const char* tip)
{
	int pressed;

	if (!enabled)
		nk_widget_disable_begin(ctx);
	pressed = button(label, tip);
	nk_widget_disable_end(ctx);
	return pressed && enabled;
}

//Dropdown of the predefined patterns, grouped by category
static void patternPicker(float width)
{
	struct nk_rect bounds;
	const char* category = NULL;
	char text[64];
	int i;

	nk_layout_row_push(ctx, width);
	bounds = nk_widget_bounds(ctx);

	if (nk_combo_begin_label(ctx, placingPattern >= 0 ? patterns[placingPattern].name : "Place a pattern...", nk_vec2(width + 60, 620)))
	{
		nk_layout_row_dynamic(ctx, 24, 1);

		for (i = 0; i < patternCount; i++)
		{
			if (category == NULL || strcmp(category, patterns[i].category) != 0)
			{
				category = patterns[i].category;
				nk_label_colored(ctx, category, NK_TEXT_LEFT, nk_rgb(140, 200, 140));
			}

			snprintf(text, sizeof(text), "   %s", patterns[i].name);
			if (nk_combo_item_label(ctx, text, NK_TEXT_LEFT))
			{
				startPlacing(i);
			}
		}
		nk_combo_end(ctx);
	}
	else
	{
		tooltip(bounds, "Pick a structure, then click on the grid to place it");
	}
}

void uiFrame(void)
{
	char text[64];
	int width, height;
	bool placing = placingPattern >= 0;

	nk_glfw3_new_frame(&glfw);
	glfwGetWindowSize(glfw.win, &width, &height);

	if (nk_begin(ctx, "Toolbar", nk_rect(0, 0, (float)width, TOOLBAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_begin(ctx, NK_STATIC, BUTTON_HEIGHT, 48);

		///Game speed
		nk_layout_row_push(ctx, 70);
		if (button(gameLoop ? "Pause" : "Play", "Pause / resume (Space, Esc)"))
		{
			togglePause();
		}

		nk_layout_row_push(ctx, 56);
		if (buttonIf(!gameLoop, "Step", "Advance one generation while paused (Enter)"))
		{
			stepGame();
		}

		gap();
		nk_layout_row_push(ctx, 64);
		if (button("Slower", "Slow down (Numpad -)"))
		{
			slowDown();
		}
		snprintf(text, sizeof(text), "%u ms", timePerTick);
		label(text, 60);
		nk_layout_row_push(ctx, 64);
		if (button("Faster", "Speed up (Numpad +)"))
		{
			speedUp();
		}

		///Cam movement
		gap();
		nk_layout_row_push(ctx, BUTTON_HEIGHT);
		cameraButton(NK_SYMBOL_TRIANGLE_LEFT, -1, 0, "Move left (Left arrow)");
		nk_layout_row_push(ctx, BUTTON_HEIGHT);
		cameraButton(NK_SYMBOL_TRIANGLE_UP, 0, 1, "Move up (Up arrow)");
		nk_layout_row_push(ctx, BUTTON_HEIGHT);
		cameraButton(NK_SYMBOL_TRIANGLE_DOWN, 0, -1, "Move down (Down arrow)");
		nk_layout_row_push(ctx, BUTTON_HEIGHT);
		cameraButton(NK_SYMBOL_TRIANGLE_RIGHT, 1, 0, "Move right (Right arrow)");
		snprintf(text, sizeof(text), "%lld, %lld", camPosX + camSizeX / 2, camPosY + camSizeY / 2);
		label(text, 110);

		///Zoom
		nk_layout_row_push(ctx, BUTTON_HEIGHT);
		if (buttonIf(canZoomOut(), "-", "Zoom out (-, mouse wheel)"))
		{
			zoomOut();
		}
		snprintf(text, sizeof(text), "%d%%", zoomPercent());
		label(text, 48);
		nk_layout_row_push(ctx, BUTTON_HEIGHT);
		if (buttonIf(canZoomIn(), "+", "Zoom in (=, mouse wheel)"))
		{
			zoomIn();
		}

		///Rendering
		gap();
		nk_layout_row_push(ctx, 66);
		colorButton("R", &red, "Change red (R)");
		nk_layout_row_push(ctx, 66);
		colorButton("G", &green, "Change green (G)");
		nk_layout_row_push(ctx, 66);
		colorButton("B", &blue, "Change blue (B)");
		nk_layout_row_push(ctx, BUTTON_HEIGHT);
		nk_button_color(ctx, nk_rgb_f(red, green, blue));

		nk_layout_row_push(ctx, 84);
		if (button(showHalo ? "Halo: On" : "Halo: Off", "Toggle the halo effect (H)"))
		{
			toggleHalo();
		}
		nk_layout_row_push(ctx, 84);
		if (button(showGrid ? "Grid: On" : "Grid: Off", "Toggle the grid lines (L)"))
		{
			toggleGrid();
		}

		///Editing
		gap();
		nk_layout_row_push(ctx, 56);
		if (button("Clear", "Remove every cell. Click cells to flip them, drag to draw"))
		{
			clearBoard();
		}
		patternPicker(220);
		nk_layout_row_push(ctx, 64);
		if (buttonIf(placing, "Rotate", "Rotate the pattern clockwise (Q)"))
		{
			rotatePlacing();
		}
		nk_layout_row_push(ctx, 48);
		if (buttonIf(placing, "Flip", "Mirror the pattern (F)"))
		{
			flipPlacing();
		}
		nk_layout_row_push(ctx, 60);
		if (buttonIf(placing, "Done", "Stop placing (right click, Esc)"))
		{
			stopPlacing();
		}

		nk_layout_row_end(ctx);
	}
	nk_end(ctx);
}

void uiRender(void)
{
	nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}
