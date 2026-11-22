#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _WIN32
#define _DEFAULT_SOURCE //readlink, chdir
#endif

#if defined(_MSC_VER) && !defined(NDEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shader.h"
#include "macros.h"
#include "glExtensions.h"
#include "gol.h"
#include "controls.h"
#include "patterns.h"
#include "ui.h"

//Size of the grid area in window units. Every zoom level divides both exactly
#define GRID_WIDTH 1890
#define GRID_HEIGHT 945

//Cell sizes in window units, from zoomed out to zoomed in
static const unsigned int zoomLevels[] = { 5, 7, 9, 15, 21, 27, 35, 45, 63 };
#define ZOOM_LEVEL_COUNT ((int)(sizeof(zoomLevels) / sizeof(zoomLevels[0])))
#define DEFAULT_ZOOM 4

unsigned int timePerTick = 100;
unsigned int sqrSize = 21; //Size of a cell in window units, depends on zoom
int zoomLevel = DEFAULT_ZOOM;
const float holeSize = 1.0f;
const float haloFactor = 0.2f;
const float haloHaloFactor = 0.1f;
const float gridBrightness = 0.07f;
const float ghostBrightness = 0.8f;
bool debug = false;
bool gameLoop = true;
bool showHalo = true;
bool showGrid = false;
double lastTick = 0.0; //Time of last game tick, in seconds (glfwGetTime)
double lastFrame = 0.0; //Time of last frame, in seconds (glfwGetTime)
cell* board;
cell* tmpBoard; //Precomputed next generation, equal to board if not computed yet

float red = 0.0f; //Variable of the color red
float green = 1.0f; //Variable of the color green
float blue = 0.0f; //Variable of the color blue

//Pattern being placed with the mouse
int placingPattern = -1;
static int* ghostCells = NULL; //x, y pairs, y pointing up, relative to the bottom left
static int ghostCount = 0;
static int ghostWidth = 0;
static int ghostHeight = 0;

//Drawing cells by dragging the mouse
static bool painting = false;
static bool paintState = false;
static long long lastPaintX, lastPaintY;

//Panning the camera by dragging with the middle mouse button
static bool panning = false;
static double panLastX, panLastY; //Cursor position at the last pan update
static double panRestX, panRestY; //Dragged distance not yet turned into whole cells, in window units
static GLFWcursor* panCursor = NULL;

//OpenGL objects and per zoom level data
static struct
{
	GLFWwindow* window;
	unsigned int vao;
	unsigned int vbo; //Corners of every visible cell
	unsigned int ibo; //Indices for whatever is being drawn
	unsigned int gridIbo; //Indices for every visible cell, for the grid overlay
	unsigned int gridIndexCount;
	unsigned int shader;
	int uColor, uHoleColor, uSqrSize, uGapSize, uHoleSize;
	bool geometryDirty; //Zoom changed, the buffers need to be rebuilt

	//One byte per visible cell
	unsigned char* cellGrid;
	unsigned char* haloGrid;
	unsigned char* haloHaloGrid;
} gfx;

typedef struct
{
	unsigned int* data;
	unsigned int count;
	unsigned int capacity;
} IndexList;

int initWindow(GLFWwindow** window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void drawToWindow(GLFWwindow* window);
int main(int argc, char* argv[]);

//If res/ is not in the working directory, switch to the executable's directory
static void enterResourceDir(void)
{
	char path[4096];
	char* slash;
	FILE* f = fopen("res/shaders/basic.shader", "r");

	if (f)
	{
		fclose(f);
		return;
	}

#ifdef _WIN32
	DWORD len = GetModuleFileNameA(NULL, path, sizeof(path));
	if (len == 0 || len >= sizeof(path))
		return;
	slash = strrchr(path, '\\');
#else
	ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (len <= 0)
		return;
	path[len] = '\0';
	slash = strrchr(path, '/');
#endif

	if (slash)
	{
		*slash = '\0';
		if (chdir(path) != 0)
			perror(path);
	}
}

int initWindow(GLFWwindow** window)
{
	GLenum glewErr;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //The grid has a fixed size

	/* Create a windowed mode window and its OpenGL context */
	*window = glfwCreateWindow(GRID_WIDTH, GRID_HEIGHT + TOOLBAR_HEIGHT, "Game of Life", NULL, NULL);

	if (!(*window))
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(*window);

	glfwSwapInterval(1);

	glewExperimental = GL_TRUE; //Needed for core profile contexts
	glewErr = glewInit();
	glGetError(); //glewInit can leave a harmless GL_INVALID_ENUM behind

#ifdef GLEW_ERROR_NO_GLX_DISPLAY
	//A GLX-only GLEW reports this on Wayland (EGL context), but the GL functions are still loaded
	if (glewErr == GLEW_ERROR_NO_GLX_DISPLAY)
	{
		glewErr = GLEW_OK;
	}
#endif

	if (glewErr != GLEW_OK)
	{
		printf("Failed to initialize GLEW: %s\n", (const char*)glewGetErrorString(glewErr));
		return -1;
	}

	printf("OpenGL version : %s\n", (const char*)glGetString(GL_VERSION));
	return 0;
}

///Building the geometry

static void pushIndex(IndexList* list, unsigned int index)
{
	if (list->count == list->capacity)
	{
		list->capacity = list->capacity ? list->capacity * 2 : 1024;
		list->data = (unsigned int*)realloc(list->data, list->capacity * sizeof(unsigned int));
	}
	list->data[list->count++] = index;
}

//Index of the vertex at the bottom left corner of the given screen cell
static unsigned int vertexIndex(int x, int y)
{
	return (unsigned int)((camSizeX + 1) * y + x);
}

static void pushSquare(IndexList* list, int x, int y)
{
	//First triangle
	pushIndex(list, vertexIndex(x, y));
	pushIndex(list, vertexIndex(x + 1, y));
	pushIndex(list, vertexIndex(x + 1, y + 1));
	//Second triangle
	pushIndex(list, vertexIndex(x, y));
	pushIndex(list, vertexIndex(x, y + 1));
	pushIndex(list, vertexIndex(x + 1, y + 1));
}

static void pushTriangle(IndexList* list, unsigned int a, unsigned int b, unsigned int c)
{
	pushIndex(list, a);
	pushIndex(list, b);
	pushIndex(list, c);
}

//Generate verticies for the corners of every visible cell
static float* generateVertexPositions(void)
{
	int i, j;
	float* positions = (float*)malloc((size_t)(camSizeX + 1) * (camSizeY + 1) * sizeof(float) * 2);

	for (i = 0; i <= camSizeY; i++)
	{
		for (j = 0; j <= camSizeX; j++)
		{
			positions[((camSizeX + 1) * i + j) * 2] = -1.0f + 2.0f * j / camSizeX;
			positions[((camSizeX + 1) * i + j) * 2 + 1] = -1.0f + 2.0f * i / camSizeY;
		}
	}
	return positions;
}

//Rebuild everything that depends on the number of visible cells
static void updateGeometry(void)
{
	float* positions;
	IndexList grid = { 0 };
	int x, y;
	size_t cells = (size_t)camSizeX * camSizeY;

	if (!gfx.geometryDirty)
		return;
	gfx.geometryDirty = false;

	positions = generateVertexPositions();
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, gfx.vbo));
	GLCall(glBufferData(GL_ARRAY_BUFFER, (camSizeX + 1) * (camSizeY + 1) * 2 * sizeof(float), positions, GL_STATIC_DRAW));
	free(positions);

	for (y = 0; y < camSizeY; y++)
	{
		for (x = 0; x < camSizeX; x++)
		{
			pushSquare(&grid, x, y);
		}
	}
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx.gridIbo));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, grid.count * sizeof(unsigned int), grid.data, GL_STATIC_DRAW));
	gfx.gridIndexCount = grid.count;
	free(grid.data);

	free(gfx.cellGrid);
	free(gfx.haloGrid);
	free(gfx.haloHaloGrid);
	gfx.cellGrid = (unsigned char*)malloc(cells);
	gfx.haloGrid = (unsigned char*)malloc(cells);
	gfx.haloHaloGrid = (unsigned char*)malloc(cells);
}

//Mark the visible living cells
static void markCells(unsigned char* grid)
{
	cell* c;

	memset(grid, 0, (size_t)camSizeX * camSizeY);

	for (c = board; c != NULL; c = c->next)
	{
		if (camPosX <= c->x && (camPosX + camSizeX) > c->x && camPosY <= c->y && (camPosY + camSizeY) > c->y)
		{
			grid[(c->y - camPosY) * camSizeX + (c->x - camPosX)] = 1;
		}
	}
}

//Mark every place in or next to a marked place
static void dilate(const unsigned char* src, unsigned char* dst)
{
	int x, y, dx, dy;

	memset(dst, 0, (size_t)camSizeX * camSizeY);

	for (y = 0; y < camSizeY; y++)
	{
		for (x = 0; x < camSizeX; x++)
		{
			if (!src[y * camSizeX + x])
				continue;

			for (dy = -1; dy <= 1; dy++)
			{
				for (dx = -1; dx <= 1; dx++)
				{
					if (x + dx >= 0 && x + dx < camSizeX && y + dy >= 0 && y + dy < camSizeY)
					{
						dst[(y + dy) * camSizeX + x + dx] = 1;
					}
				}
			}
		}
	}
}

static void squareIndices(const unsigned char* grid, IndexList* list)
{
	int x, y;

	for (y = 0; y < camSizeY; y++)
	{
		for (x = 0; x < camSizeX; x++)
		{
			if (grid[y * camSizeX + x])
			{
				pushSquare(list, x, y);
			}
		}
	}
}

//Halo shape: full squares inside, triangles on the corners to round it off
static void haloIndices(const unsigned char* grid, IndexList* list)
{
	int x, y;
	bool up, right, down, left;
	unsigned int neighbourCount;

	for (y = 0; y < camSizeY; y++)
	{
		for (x = 0; x < camSizeX; x++)
		{
			if (!grid[y * camSizeX + x])
				continue;

			up = y + 1 < camSizeY && grid[(y + 1) * camSizeX + x];
			down = y > 0 && grid[(y - 1) * camSizeX + x];
			left = x > 0 && grid[y * camSizeX + x - 1];
			right = x + 1 < camSizeX && grid[y * camSizeX + x + 1];
			neighbourCount = up + down + left + right;

			if (neighbourCount > 2)
			{
				pushSquare(list, x, y);
			}
			else if (neighbourCount == 2)
			{
				if (left && down)
					pushTriangle(list, vertexIndex(x, y), vertexIndex(x + 1, y), vertexIndex(x, y + 1));
				else if (down && right)
					pushTriangle(list, vertexIndex(x, y), vertexIndex(x + 1, y), vertexIndex(x + 1, y + 1));
				else if (right && up)
					pushTriangle(list, vertexIndex(x, y + 1), vertexIndex(x + 1, y), vertexIndex(x + 1, y + 1));
				else if (up && left)
					pushTriangle(list, vertexIndex(x, y + 1), vertexIndex(x, y), vertexIndex(x + 1, y + 1));
			}
		}
	}
}

static void drawIndices(unsigned int ibo, const unsigned int* data, unsigned int count, float r, float g, float b, float a)
{
	if (count == 0)
		return;

	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));

	if (data != NULL)
	{
		GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STREAM_DRAW));
	}
	GLCall(glUniform4f(gfx.uColor, r, g, b, a));
	GLCall(glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, NULL));
}

///Editing the board. The precomputed next generation is thrown away on every change

static void editBegin(void)
{
	if (tmpBoard != board)
	{
		deleteCell(tmpBoard);
	}
	tmpBoard = NULL;
}

static void editEnd(void)
{
	tmpBoard = board;
}

//Screen cell (0, 0 is the bottom left of the grid) at the given window position
static bool windowToScreenCell(double mx, double my, int* col, int* row)
{
	double fromBottom = (double)(TOOLBAR_HEIGHT + GRID_HEIGHT) - my;

	if (mx < 0 || mx >= GRID_WIDTH || my < TOOLBAR_HEIGHT || fromBottom <= 0)
		return false;

	*col = (int)(mx / sqrSize);
	*row = (int)(fromBottom / sqrSize);

	if (*col >= camSizeX)
		*col = camSizeX - 1;
	if (*row >= camSizeY)
		*row = camSizeY - 1;
	return true;
}

//World cell under the mouse, false if the mouse is not over the grid
static bool mouseCell(long long* x, long long* y)
{
	double mx, my;
	int col, row;

	glfwGetCursorPos(gfx.window, &mx, &my);

	if (!windowToScreenCell(mx, my, &col, &row))
		return false;

	*x = camPosX + col;
	*y = camPosY + row;
	return true;
}

//Bottom left corner of the ghost when centered on the given cell
static void ghostOrigin(long long x, long long y, long long* ox, long long* oy)
{
	*ox = x - ghostWidth / 2;
	*oy = y - ghostHeight / 2;
}

static void placeGhost(long long x, long long y)
{
	long long ox, oy;
	int i;

	ghostOrigin(x, y, &ox, &oy);
	editBegin();

	for (i = 0; i < ghostCount; i++)
	{
		board = setCell(board, ox + ghostCells[i * 2], oy + ghostCells[i * 2 + 1], true);
	}
	editEnd();
}

///Actions shared by the keyboard, the mouse and the toolbar buttons

//Camera moves faster when zoomed out
static int scrollSpeed(void)
{
	return camSizeX / 30 > 1 ? camSizeX / 30 : 1;
}

void moveCamera(int dx, int dy)
{
	int speed = scrollSpeed();

	if (dx > 0 && camPosX < (LONGLONG_MAX - speed))
		camPosX += speed;
	if (dx < 0 && (camPosX + camSizeX) > (LONGLONG_MIN) + speed)
		camPosX -= speed;
	if (dy > 0 && (camPosY + camSizeY) < (LONGLONG_MAX - speed))
		camPosY += speed;
	if (dy < 0 && camPosY > (LONGLONG_MIN) + speed)
		camPosY -= speed;

	if (debug)
	{
		printf("%lld, %lld\n", (camPosX + camSizeX / 2), (camPosY + camSizeY / 2));
	}
}

//Zoom keeping the world point at the given screen position (in cells) in place
static void zoomAround(int direction, double anchorCol, double anchorRow)
{
	int newLevel = zoomLevel + direction;
	double pixelX, pixelY;

	if (newLevel < 0 || newLevel >= ZOOM_LEVEL_COUNT)
		return;

	pixelX = anchorCol * sqrSize;
	pixelY = anchorRow * sqrSize;

	zoomLevel = newLevel;
	sqrSize = zoomLevels[zoomLevel];
	camSizeX = GRID_WIDTH / sqrSize;
	camSizeY = GRID_HEIGHT / sqrSize;

	camPosX += (long long)floor(anchorCol - pixelX / sqrSize + 0.5);
	camPosY += (long long)floor(anchorRow - pixelY / sqrSize + 0.5);
	gfx.geometryDirty = true;
}

void zoomIn(void)
{
	zoomAround(1, camSizeX / 2.0, camSizeY / 2.0);
}

void zoomOut(void)
{
	zoomAround(-1, camSizeX / 2.0, camSizeY / 2.0);
}

int zoomPercent(void)
{
	return (int)(sqrSize * 100 / zoomLevels[DEFAULT_ZOOM]);
}

bool canZoomIn(void)
{
	return zoomLevel + 1 < ZOOM_LEVEL_COUNT;
}

bool canZoomOut(void)
{
	return zoomLevel > 0;
}

void togglePause(void)
{
	gameLoop = !gameLoop;
}

void stepGame(void)
{
	if (gameLoop)
		return;

	//Use the precomputed next generation if there is one
	if (tmpBoard == board)
	{
		tmpBoard = gameTick(board);
	}

	if (tmpBoard != board)
	{
		deleteCell(board);
		board = tmpBoard;
	}
	lastTick = glfwGetTime();
}

void slowDown(void)
{
	if (timePerTick < 300)
		timePerTick += 10;
}

void speedUp(void)
{
	if (timePerTick > 0)
		timePerTick -= 10;
}

void cycleColor(float* channel)
{
	*channel += 0.25f;
	*channel = *channel > 1.0f ? 0 : *channel;
}

void toggleHalo(void)
{
	showHalo = !showHalo;
}

void toggleGrid(void)
{
	showGrid = !showGrid;
}

void clearBoard(void)
{
	editBegin();
	deleteCell(board);
	board = NULL;
	editEnd();
}

void startPlacing(int pattern)
{
	const char* p;
	int x = 0, row = 0;

	if (pattern < 0 || pattern >= patternCount)
		return;

	placingPattern = pattern;
	ghostWidth = patterns[pattern].width;
	ghostHeight = patterns[pattern].height;
	ghostCount = 0;
	free(ghostCells);
	ghostCells = (int*)malloc(sizeof(int) * 2 * ghostWidth * ghostHeight);

	//Rows go from top to bottom, cell y coordinates go up
	for (p = patterns[pattern].rows; *p; p++)
	{
		if (*p == '\n')
		{
			row++;
			x = 0;
			continue;
		}

		if (*p == 'O')
		{
			ghostCells[ghostCount * 2] = x;
			ghostCells[ghostCount * 2 + 1] = ghostHeight - 1 - row;
			ghostCount++;
		}
		x++;
	}
}

void stopPlacing(void)
{
	placingPattern = -1;
	ghostCount = 0;
}

void rotatePlacing(void)
{
	int i, x;

	if (placingPattern < 0)
		return;

	//Clockwise
	for (i = 0; i < ghostCount; i++)
	{
		x = ghostCells[i * 2];
		ghostCells[i * 2] = ghostCells[i * 2 + 1];
		ghostCells[i * 2 + 1] = ghostWidth - 1 - x;
	}
	x = ghostWidth;
	ghostWidth = ghostHeight;
	ghostHeight = x;
}

void flipPlacing(void)
{
	int i;

	if (placingPattern < 0)
		return;

	for (i = 0; i < ghostCount; i++)
	{
		ghostCells[i * 2] = ghostWidth - 1 - ghostCells[i * 2];
	}
}

///Input

//Handle key presses
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	uiKeyCallback(window, key, scancode, action, mods);

	if (!action) //Press or repeat only
		return;

	switch (key)
	{
		///Cam movement
	case GLFW_KEY_RIGHT: moveCamera(1, 0); break;
	case GLFW_KEY_LEFT: moveCamera(-1, 0); break;
	case GLFW_KEY_DOWN: moveCamera(0, -1); break;
	case GLFW_KEY_UP: moveCamera(0, 1); break;
	case GLFW_KEY_EQUAL: zoomIn(); break;
	case GLFW_KEY_MINUS: zoomOut(); break;

		///Game speed
	case GLFW_KEY_ESCAPE: //Stop placing, or Pause/Play
		if (placingPattern >= 0)
			stopPlacing();
		else
			togglePause();
		break;
	case GLFW_KEY_SPACE:
		togglePause();
		break;
	case GLFW_KEY_ENTER: //Single step if paused
	case GLFW_KEY_KP_ENTER:
		stepGame();
		break;
	case GLFW_KEY_KP_SUBTRACT: slowDown(); break;
	case GLFW_KEY_KP_ADD: speedUp(); break;

		///Rendering
	case GLFW_KEY_R: cycleColor(&red); break;
	case GLFW_KEY_G: cycleColor(&green); break;
	case GLFW_KEY_B: cycleColor(&blue); break;
	case GLFW_KEY_H: toggleHalo(); break;
	case GLFW_KEY_L: toggleGrid(); break;

		///Placing patterns
	case GLFW_KEY_Q: rotatePlacing(); break;
	case GLFW_KEY_F: flipPlacing(); break;
	default:
		break;
	}
}

static void startPanning(void)
{
	glfwGetCursorPos(gfx.window, &panLastX, &panLastY);
	panRestX = 0;
	panRestY = 0;
	panning = true;
	glfwSetCursor(gfx.window, panCursor);
}

static void stopPanning(void)
{
	if (!panning)
		return;
	panning = false;
	glfwSetCursor(gfx.window, NULL);
}

//Move a camera coordinate by a number of cells, without running off the edge of the world
static long long shiftCamera(long long pos, long long by, int size)
{
	if (by > 0 && pos + size > LONGLONG_MAX - by)
		return LONGLONG_MAX - size;
	if (by < 0 && pos < LONGLONG_MIN - by)
		return LONGLONG_MIN;
	return pos + by;
}

//Move the camera so the world stays under the cursor while dragging with the middle button
static void continuePanning(void)
{
	double mx, my;
	long long cellsX, cellsY;

	if (!panning)
		return;

	if (glfwGetMouseButton(gfx.window, GLFW_MOUSE_BUTTON_MIDDLE) != GLFW_PRESS)
	{
		stopPanning();
		return;
	}

	glfwGetCursorPos(gfx.window, &mx, &my);
	panRestX += mx - panLastX;
	panRestY += my - panLastY;
	panLastX = mx;
	panLastY = my;

	//Only whole cells, the rest is kept for the next frame (and survives zooming mid-drag)
	cellsX = (long long)(panRestX / sqrSize);
	cellsY = (long long)(panRestY / sqrSize);
	panRestX -= (double)cellsX * sqrSize;
	panRestY -= (double)cellsY * sqrSize;

	//Dragging right shows what was to the left; window y points down, world y points up
	camPosX = shiftCamera(camPosX, -cellsX, camSizeX);
	camPosY = shiftCamera(camPosY, cellsY, camSizeY);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	long long x, y;

	uiMouseButtonCallback(window, button, action, mods);

	if (action == GLFW_RELEASE)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			painting = false;
		if (button == GLFW_MOUSE_BUTTON_MIDDLE)
			stopPanning();
		return;
	}

	if (uiWantsMouse())
		return;

	if (button == GLFW_MOUSE_BUTTON_RIGHT && placingPattern >= 0)
	{
		stopPlacing();
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (mouseCell(&x, &y))
			startPanning();
		return;
	}

	if (button != GLFW_MOUSE_BUTTON_LEFT || !mouseCell(&x, &y))
		return;

	if (placingPattern >= 0)
	{
		placeGhost(x, y);
		return;
	}

	//Flip the clicked cell, dragging sets more cells to the same state
	paintState = !isCellAlive(board, x, y);
	editBegin();
	board = setCell(board, x, y, paintState);
	editEnd();

	painting = true;
	lastPaintX = x;
	lastPaintY = y;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	double mx, my;
	int col, row;

	uiScrollCallback(window, xoffset, yoffset);

	if (uiWantsMouse() || yoffset == 0)
		return;

	glfwGetCursorPos(window, &mx, &my);

	if (!windowToScreenCell(mx, my, &col, &row))
		return;

	//Zoom around the point under the mouse
	zoomAround(yoffset > 0 ? 1 : -1, mx / sqrSize, ((TOOLBAR_HEIGHT + GRID_HEIGHT) - my) / sqrSize);
}

//Keep drawing while the mouse is dragged with the left button down
static void continuePainting(void)
{
	long long x, y;

	if (!painting)
		return;

	if (glfwGetMouseButton(gfx.window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
	{
		painting = false;
		return;
	}

	if (!mouseCell(&x, &y) || (x == lastPaintX && y == lastPaintY))
		return;

	editBegin();
	board = setCell(board, x, y, paintState);
	editEnd();
	lastPaintX = x;
	lastPaintY = y;
}

///Drawing

void drawToWindow(GLFWwindow* window)
{
	static IndexList cells, halo, haloHalo, ghost;
	int winWidth, winHeight, fbWidth, fbHeight;
	float scale; //Framebuffer pixels per window unit (HiDPI)
	long long mouseX, mouseY, ox, oy, x, y;
	int i;

	//Build the toolbar first, button presses can change the board
	uiFrame();
	continuePainting();
	continuePanning();

	glfwGetWindowSize(window, &winWidth, &winHeight);
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	scale = winWidth > 0 ? (float)fbWidth / (float)winWidth : 1.0f;

	// Clear screen
	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	//The grid is drawn at the bottom of the window, below the toolbar
	GLCall(glViewport(0, 0, fbWidth, (int)(GRID_HEIGHT * scale + 0.5f)));

	//Bind Everything
	GLCall(glUseProgram(gfx.shader));
	GLCall(glBindVertexArray(gfx.vao));
	updateGeometry(); //Needs the vertex array bound, it binds index buffers
	GLCall(glUniform1f(gfx.uSqrSize, sqrSize * scale));
	GLCall(glUniform1f(gfx.uGapSize, (sqrSize < 10 ? 0.5f : 1.0f) * scale)); //Thinner gaps when zoomed out
	GLCall(glUniform4f(gfx.uHoleColor, 0.2f, 0.2f, 0.2f, 0.5f));

	cells.count = halo.count = haloHalo.count = ghost.count = 0;

	///Drawing the grid
	if (showGrid)
	{
		drawIndices(gfx.gridIbo, NULL, gfx.gridIndexCount, gridBrightness, gridBrightness, gridBrightness, 1.0f);
	}

	markCells(gfx.cellGrid);

	if (showHalo)
	{
		dilate(gfx.cellGrid, gfx.haloGrid);
		dilate(gfx.haloGrid, gfx.haloHaloGrid);
		haloIndices(gfx.haloGrid, &halo);
		haloIndices(gfx.haloHaloGrid, &haloHalo);

		///Drawing the halo halo
		drawIndices(gfx.ibo, haloHalo.data, haloHalo.count, red * haloHaloFactor, green * haloHaloFactor, blue * haloHaloFactor, 0.5f);

		///Drawing the halo
		drawIndices(gfx.ibo, halo.data, halo.count, red * haloFactor, green * haloFactor, blue * haloFactor, 0.7f);
	}

	///Drawing the cells
	squareIndices(gfx.cellGrid, &cells);
	drawIndices(gfx.ibo, cells.data, cells.count, red, green, blue, 1.0f);

	///Drawing the pattern being placed under the mouse
	if (placingPattern >= 0 && !uiWantsMouse() && mouseCell(&mouseX, &mouseY))
	{
		ghostOrigin(mouseX, mouseY, &ox, &oy);

		for (i = 0; i < ghostCount; i++)
		{
			x = ox + ghostCells[i * 2] - camPosX;
			y = oy + ghostCells[i * 2 + 1] - camPosY;

			if (x >= 0 && x < camSizeX && y >= 0 && y < camSizeY)
			{
				pushSquare(&ghost, (int)x, (int)y);
			}
		}
		drawIndices(gfx.ibo, ghost.data, ghost.count, ghostBrightness, ghostBrightness, ghostBrightness, 1.0f);
	}

	//Draw the toolbar on top
	uiRender();

	/* Swap front and back buffers */
	GLCall(glfwSwapBuffers(window));

	/* Poll for and process events */
	GLCall(glfwPollEvents());
}

int main(int argc, char* argv[])
{
	ShaderProgramSource shaderSource; //Source of the shaders
	double elapsedTime; //Time since last game tick, in ms

	if (argc >= 3)
	{
		debug = (argv[2][0] == '0');
		printf("%d\n", debug);
	}

	//User supplied init files are relative to the original working directory
	if (argc >= 2)
	{
		board = parseFile(argv[1]);
	}
	enterResourceDir();

	if (argc < 2)
	{
		board = parseFile("res/init/cells.init");
	}
	tmpBoard = board;

	if (initWindow(&gfx.window) == -1)
	{
		return -1;
	}
	uiInit(gfx.window);
	panCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	//Replace the UI's callbacks, they get forwarded to it
	glfwSetKeyCallback(gfx.window, key_callback);
	glfwSetMouseButtonCallback(gfx.window, mouse_button_callback);
	glfwSetScrollCallback(gfx.window, scroll_callback);

	GLCall(glGenVertexArrays(1, &gfx.vao));
	GLCall(glBindVertexArray(gfx.vao));

	//Creating the buffers, they are filled in updateGeometry
	GLCall(glGenBuffers(1, &gfx.vbo));
	GLCall(glGenBuffers(1, &gfx.ibo));
	GLCall(glGenBuffers(1, &gfx.gridIbo));
	gfx.geometryDirty = true;

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, gfx.vbo));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0)); //Setting the layout of the buffer

	//Getting the source code of the shader
	shaderSource = ParseShader("res/shaders/basic.shader");

	if (shaderSource.VertexSource[0] == '\0' || shaderSource.FragmentSource[0] == '\0')
	{
		printf("Could not load res/shaders/basic.shader\n");
		glfwTerminate();
		return -1;
	}

	//Compiling and running shader
	gfx.shader = CreateShader(shaderSource.VertexSource, shaderSource.FragmentSource);
	GLCall(glUseProgram(gfx.shader));
	free(shaderSource.FragmentSource);
	free(shaderSource.VertexSource);

	//Getting the location of the uniforms
	GLCall(gfx.uColor = glGetUniformLocation(gfx.shader, "u_Color"));
	ASSERT(gfx.uColor != -1);

	GLCall(gfx.uHoleColor = glGetUniformLocation(gfx.shader, "u_holeColor"));
	ASSERT(gfx.uHoleColor != -1);

	GLCall(gfx.uSqrSize = glGetUniformLocation(gfx.shader, "u_sqrSize"));
	ASSERT(gfx.uSqrSize != -1);

	GLCall(gfx.uGapSize = glGetUniformLocation(gfx.shader, "u_gapSize"));
	ASSERT(gfx.uGapSize != -1);

	GLCall(gfx.uHoleSize = glGetUniformLocation(gfx.shader, "u_holeSize"));
	ASSERT(gfx.uHoleSize != -1);

	//Setting uniforms
	GLCall(glUniform1f(gfx.uHoleSize, 0));

	//Unbind everything. By binding 0, the binding will be reset
	GLCall(glBindVertexArray(0));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glUseProgram(0));

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(gfx.window))
	{
		drawToWindow(gfx.window);
		elapsedTime = (glfwGetTime() - lastTick) * 1000.0;

		if (tmpBoard == board)
		{
			tmpBoard = gameTick(board);
		}

		if (gameLoop && elapsedTime > timePerTick)
		{
			if (tmpBoard != board)
			{
				deleteCell(board);
				board = tmpBoard;
			}
			lastTick = glfwGetTime();
		}

		if (debug)
		{
			double frameTime = glfwGetTime() - lastFrame;
			printf("%f fps, %f ms \n", 1.0 / frameTime, frameTime * 1000.0);
			lastFrame = glfwGetTime();
		}
	}

	//Clean up after running
	if (tmpBoard != board)
	{
		deleteCell(tmpBoard);
	}
	deleteCell(board);
	free(ghostCells);
	free(gfx.cellGrid);
	free(gfx.haloGrid);
	free(gfx.haloHaloGrid);
	GLCall(glDeleteProgram(gfx.shader));

	uiShutdown();
	glfwTerminate();
#if defined(_MSC_VER) && !defined(NDEBUG)
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
