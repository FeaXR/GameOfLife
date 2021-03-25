#define _CRT_SECURE_NO_WARNINGS

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <time.h>

#include "shader.h"
#include "macros.h"
#include "glExtensions.h"
#include "gol.h"

unsigned int timePerTick = 100;
const unsigned int sqrSize = 21;
const float gapSize = 1.0f;
const float holeSize = 1.0f;
const unsigned int xCount = camSizeX;
const unsigned int yCount = camSizeY;
const unsigned int scrollSpeed = 3;
const float haloFactor = 0.2f;
const float haloHaloFactor = 0.1f;
bool debug = false;
bool gameLoop = true;
bool showHalo = true;
clock_t lastTick = 0;
clock_t lastFrame = 0;
cell* board;
cell* tmpBoard;

float red = 0.0f; //Variable of the color red
float green = 1.0f; //Variable of the color green
float blue = 0.0f; //Variable of the color blue

int initWindow(GLFWwindow** window);
float* generateVertexPositions();
unsigned int* generateIndicesFromPositions(unsigned int* indexCount, positionOnScreen* p);
unsigned int* generateHaloIndicesFromPositions(unsigned int* indexCount, positionOnScreen* p);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void drawToWindow(GLFWwindow* window, unsigned int* buffer, unsigned int vao, unsigned int ibo, unsigned int shader, int u_ColorLocation, int u_holeColorLocation, int i_sqrSizeLocation, int f_gapSizeLocation, int f_holeSizeLocation, float* positions);
int main(int argc, char* argv[]);

int initWindow(GLFWwindow** window)
{
	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	/* Create a windowed mode window and its OpenGL context */
	*window = glfwCreateWindow(sqrSize * xCount, sqrSize * yCount, "Program", NULL, NULL);

	if (!(*window))
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(*window);

	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK)
	{
		printf("Error!");
		return -1;
	}

	printf("OpenGL version : ");
	printf(glGetString(GL_VERSION));
	printf("\n");
	return 0;
}

//Generate verticies
float* generateVertexPositions()
{
	float yPos = -1.0f;
	float xPos = -1.0f;
	float xDist = 2.0f / xCount;
	float yDist = 2.0f / yCount;
	unsigned int i, j;
	float* positions;

	positions = (float*)malloc((xCount + 1) * (yCount + 1) * sizeof(float) * 2);

	for (i = 0; i <= yCount; i++)
	{
		xPos = -1.0f;

		for (j = 0; j <= xCount; j++)
		{
			positions[((xCount + 1) * i + j) * 2] = xPos;
			positions[((xCount + 1) * i + j) * 2 + 1] = yPos;

			if (debug)
			{
				printf("%f, %f\n", xPos, yPos);
			}
			xPos += xDist;
		}
		yPos += yDist;
	}
	return positions;
}

//generate List of indices to draw from square positions
unsigned int* generateIndicesFromPositions(unsigned int* indexCount, positionOnScreen* p)
{
	positionOnScreen* tmpPos = p;
	unsigned int i;
	unsigned int* indexes = NULL;
	unsigned int* tmp;
	*indexCount = 0;

	if (p == NULL)
	{
		return NULL;
	}

	while (1)
	{
		tmp = (unsigned int*)malloc(((*indexCount) + 6) * sizeof(unsigned int));

		for (i = 0; i < (*indexCount); i++)
		{
			tmp[i] = indexes[i];
		}
		//Expanding array and copying the data into it
		free(indexes);
		indexes = (unsigned int*)malloc(((*indexCount + 6)) * sizeof(unsigned int));

		for (i = 0; i < (*indexCount); i++)
		{
			indexes[i] = tmp[i];
		}
		free(tmp);

		//First triangle
		indexes[(*indexCount)] = ((xCount + 1) * tmpPos->y) + tmpPos->x;
		indexes[(*indexCount) + 1] = ((xCount + 1) * tmpPos->y) + tmpPos->x + 1;
		indexes[(*indexCount) + 2] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x + 1;
		//Second triangle
		indexes[(*indexCount) + 3] = ((xCount + 1) * tmpPos->y) + tmpPos->x;
		indexes[(*indexCount) + 4] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x;
		indexes[(*indexCount) + 5] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x + 1;
		//Updating count
		(*indexCount) += 6;

		if (tmpPos->next == NULL)
		{
			break;
		}
		tmpPos = tmpPos->next;
	}
	return indexes;
}

//generate List of halo indices to draw from square positions to draw around existing squares
unsigned int* generateHaloIndicesFromPositions(unsigned int* indexCount, positionOnScreen* p)
{
	positionOnScreen* tmpPos = p;
	positionOnScreen* tmpPos2 = p;
	unsigned int i;
	unsigned int* indexes = NULL;
	unsigned int* tmpIndices;
	unsigned int neighbourcount;
	bool upperNeighBour, rightNeighBour, downNeighBour, leftNeighBour;

	*indexCount = 0;

	if (p == NULL)
	{
		return NULL;
	}

	while (1)
	{
		if (debug)
		{
			printf("%d, %d\n", tmpPos2->x, tmpPos2->y);
		}

		if (tmpPos2->next == NULL)
		{
			break;
		}
		tmpPos2 = tmpPos2->next;
	}

	while (1)
	{
		neighbourcount = 0;
		upperNeighBour = rightNeighBour = downNeighBour = leftNeighBour = false;
		tmpPos2 = p;

		while (1)
		{
			if (tmpPos2->x == tmpPos->x)
			{
				if (tmpPos2->y == tmpPos->y - 1)
				{
					neighbourcount++;
					downNeighBour = true;
				}
				else if (tmpPos2->y == tmpPos->y + 1)
				{
					neighbourcount++;
					upperNeighBour = true;
				}
			}
			else if (tmpPos2->y == tmpPos->y)
			{
				if (tmpPos2->x == tmpPos->x - 1)
				{
					neighbourcount++;
					leftNeighBour = true;
				}
				else if (tmpPos2->x == tmpPos->x + 1)
				{
					neighbourcount++;
					rightNeighBour = true;
				}
			}

			if (tmpPos2->next == NULL)
			{
				break;
			}
			tmpPos2 = tmpPos2->next;
		}

		if (neighbourcount > 2)
		{
			tmpIndices = (unsigned int*)malloc(((*indexCount) + 6) * sizeof(unsigned int));

			for (i = 0; i < (*indexCount); i++)
			{
				tmpIndices[i] = indexes[i];
			}
			//Expanding array and copying the data into it
			free(indexes);
			indexes = (unsigned int*)malloc(((*indexCount + 6)) * sizeof(unsigned int));

			for (i = 0; i < (*indexCount); i++)
			{
				indexes[i] = tmpIndices[i];
			}
			free(tmpIndices);

			//First triangle
			indexes[(*indexCount)] = ((xCount + 1) * tmpPos->y) + tmpPos->x;
			indexes[(*indexCount) + 1] = ((xCount + 1) * tmpPos->y) + tmpPos->x + 1;
			indexes[(*indexCount) + 2] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x + 1;
			//Second triangle
			indexes[(*indexCount) + 3] = ((xCount + 1) * tmpPos->y) + tmpPos->x;
			indexes[(*indexCount) + 4] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x;
			indexes[(*indexCount) + 5] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x + 1;
			//Updating count
			(*indexCount) += 6;
		}
		else if (neighbourcount == 2)
		{
			if (leftNeighBour && downNeighBour)
			{
				tmpIndices = (unsigned int*)malloc(((*indexCount) + 3) * sizeof(unsigned int));

				for (i = 0; i < (*indexCount); i++)
				{
					tmpIndices[i] = indexes[i];
				}
				//Expanding array and copying the data into it
				free(indexes);
				indexes = (unsigned int*)malloc(((*indexCount + 3)) * sizeof(unsigned int));

				for (i = 0; i < (*indexCount); i++)
				{
					indexes[i] = tmpIndices[i];
				}
				free(tmpIndices);

				indexes[(*indexCount)] = ((xCount + 1) * tmpPos->y) + tmpPos->x;
				indexes[(*indexCount) + 1] = ((xCount + 1) * tmpPos->y) + tmpPos->x + 1;
				indexes[(*indexCount) + 2] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x;
				//Updating count
				(*indexCount) += 3;
			}
			else if (downNeighBour && rightNeighBour)
			{
				tmpIndices = (unsigned int*)malloc(((*indexCount) + 3) * sizeof(unsigned int));

				for (i = 0; i < (*indexCount); i++)
				{
					tmpIndices[i] = indexes[i];
				}
				//Expanding array and copying the data into it
				free(indexes);
				indexes = (unsigned int*)malloc(((*indexCount + 3)) * sizeof(unsigned int));

				for (i = 0; i < (*indexCount); i++)
				{
					indexes[i] = tmpIndices[i];
				}
				free(tmpIndices);

				indexes[(*indexCount)] = ((xCount + 1) * tmpPos->y) + tmpPos->x;
				indexes[(*indexCount) + 1] = ((xCount + 1) * tmpPos->y) + tmpPos->x + 1;
				indexes[(*indexCount) + 2] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x + 1;
				//Updating count
				(*indexCount) += 3;
			}
			else if (rightNeighBour && upperNeighBour)
			{
				tmpIndices = (unsigned int*)malloc(((*indexCount) + 3) * sizeof(unsigned int));

				for (i = 0; i < (*indexCount); i++)
				{
					tmpIndices[i] = indexes[i];
				}
				//Expanding array and copying the data into it
				free(indexes);
				indexes = (unsigned int*)malloc(((*indexCount + 3)) * sizeof(unsigned int));

				for (i = 0; i < (*indexCount); i++)
				{
					indexes[i] = tmpIndices[i];
				}
				free(tmpIndices);

				indexes[(*indexCount)] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x;
				indexes[(*indexCount) + 1] = ((xCount + 1) * tmpPos->y) + tmpPos->x + 1;
				indexes[(*indexCount) + 2] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x + 1;
				//Updating count
				(*indexCount) += 3;
			}
			else if (upperNeighBour && leftNeighBour)
			{
				tmpIndices = (unsigned int*)malloc(((*indexCount) + 3) * sizeof(unsigned int));

				for (i = 0; i < (*indexCount); i++)
				{
					tmpIndices[i] = indexes[i];
				}
				//Expanding array and copying the data into it
				free(indexes);
				indexes = (unsigned int*)malloc(((*indexCount + 3)) * sizeof(unsigned int));

				for (i = 0; i < (*indexCount); i++)
				{
					indexes[i] = tmpIndices[i];
				}
				free(tmpIndices);

				indexes[(*indexCount)] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x;
				indexes[(*indexCount) + 1] = ((xCount + 1) * tmpPos->y) + tmpPos->x;
				indexes[(*indexCount) + 2] = ((xCount + 1) * (tmpPos->y + 1)) + tmpPos->x + 1;
				//Updating count
				(*indexCount) += 3;
			}
		}

		if (tmpPos->next == NULL)
		{
			break;
		}
		tmpPos = tmpPos->next;
	}
	return indexes;
}

//Handle key presses
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action)
	{
		switch (key)
		{
			///Cam movement
		case GLFW_KEY_RIGHT: //Right
			if (camPosX < (LONGLONG_MAX - scrollSpeed))
				camPosX += scrollSpeed;
			break;
		case GLFW_KEY_LEFT: //Left
			if ((camPosX + camSizeX) > (LONGLONG_MIN) +scrollSpeed)
				camPosX -= scrollSpeed;
			break;
		case GLFW_KEY_DOWN: //Down
			if (camPosY > (LONGLONG_MIN)+scrollSpeed)
				camPosY -= scrollSpeed;

			break;
		case GLFW_KEY_UP: //Up
			if ((camPosY + camSizeY) < (LONGLONG_MAX - scrollSpeed))
				camPosY += scrollSpeed;
			break;

			///Game speed
		case GLFW_KEY_ESCAPE: //Pause/Play
		case GLFW_KEY_SPACE: 
			gameLoop = !gameLoop;
			break;
		case GLFW_KEY_ENTER: //Single step if paused
			if (!gameLoop)
			{
				board = gameTick(board);
				tmpBoard = board;
				lastTick = clock();
			}
			break;
		case GLFW_KEY_KP_SUBTRACT: //Slow down
			if (timePerTick < 300)
				timePerTick += 10;
			break;
		case GLFW_KEY_KP_ADD: //Speed up
			if (timePerTick > 0)
				timePerTick -= 10;
			break;

			///Rendering
		case GLFW_KEY_R: //Red
			red += 0.25f;
			red = red > 1.0f ? 0 : red;
			break;
		case GLFW_KEY_G: //Green
			green += 0.25f;
			green = green > 1.0f ? 0 : green;
			break;
		case GLFW_KEY_B: //Blue
			blue += 0.25f;
			blue = blue > 1.0f ? 0 : blue;
			break;
		case GLFW_KEY_H: //Halo
			showHalo = !showHalo;
			break;
		default:
			break;
		}
	}

	if (debug)
	{
		system("cls");
		printf("%lld, %lld", (camPosX + camSizeX / 2), (camPosY + camSizeY / 2));
	}
}

void drawToWindow(GLFWwindow* window, unsigned int* buffer, unsigned int vao,
	unsigned int ibo, unsigned int shader, int u_ColorLocation, int u_holeColorLocation,
	int u_sqrSizeLocation, int u_gapSizeLocation, int u_holeSizeLocation, float* positions)
{
	unsigned int* cellIndices; //Array of cell verticies to draw
	unsigned int* haloIndices; //Array of halo verticies to draw
	unsigned int* haloHaloIndices; //Array of halo halo verticies to draw

	positionOnScreen* cellPos;
	positionOnScreen* haloPos;
	positionOnScreen* haloHaloPos;

	int cellIndexCount = 0;
	int haloIndexCount = 0;
	int haloHaloIndexCount = 0;

	// Clear screen
	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	//Bind Everything
	GLCall(glUseProgram(shader));
	GLCall(glBindVertexArray(vao));

	cellPos = getCellPositions(board);
	cellIndices = generateIndicesFromPositions(&cellIndexCount, cellPos);

	if (showHalo)
	{
		haloPos = getHaloPositions(&haloIndexCount, cellPos);
		haloIndices = generateHaloIndicesFromPositions(&haloIndexCount, haloPos);

		haloHaloPos = getHaloPositions(&haloHaloIndexCount, haloPos);
		haloHaloIndices = generateHaloIndicesFromPositions(&haloHaloIndexCount, haloHaloPos);

		///Drawing the halo halo
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
		GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, haloHaloIndexCount * sizeof(unsigned int), &haloHaloIndices[0], GL_STATIC_DRAW)); //Setting the layout of the buffer

		//Setting shader
		GLCall(glUniform4f(u_ColorLocation, red * showHalo * haloHaloFactor, green * showHalo * haloHaloFactor, blue * showHalo * haloHaloFactor, 0.5f));
		GLCall(glUniform4f(u_holeColorLocation, 0.2f, 0.2f, 0.2f, 0.5f));
		//GLCall(glUniform1f(f_holeSizeLocation, holeSize));

		//Draw the triangles
		GLCall(glDrawElements(GL_TRIANGLES, haloHaloIndexCount, GL_UNSIGNED_INT, NULL));


		///Drawing the halo
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
		GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, haloIndexCount * sizeof(unsigned int), &haloIndices[0], GL_STATIC_DRAW)); //Setting the layout of the buffer

		//Setting shader
		GLCall(glUniform4f(u_ColorLocation, red * showHalo * haloFactor, green * showHalo * haloFactor, blue * showHalo * haloFactor, 0.7f));
		//GLCall(glUniform1f(f_holeSizeLocation, holeSize));

		//Draw the triangles
		GLCall(glDrawElements(GL_TRIANGLES, haloIndexCount, GL_UNSIGNED_INT, NULL));

		//freeing everything
		free(haloIndices);
		deletePositionOnScreen(haloPos);
		free(haloHaloIndices);
		deletePositionOnScreen(haloHaloPos);
	}

	///Drawing the cells
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, cellIndexCount * sizeof(unsigned int), &cellIndices[0], GL_STATIC_DRAW)); //Setting the layout of the buffer

	//Setting shader
	GLCall(glUniform4f(u_ColorLocation, red, green, blue, 1.0f));
	//GLCall(glUniform1f(f_holeSizeLocation, 0));

	//Draw the triangles
	GLCall(glDrawElements(GL_TRIANGLES, cellIndexCount, GL_UNSIGNED_INT, NULL));

	/* Swap front and back buffers */
	GLCall(glfwSwapBuffers(window));

	/* Poll for and process events */
	GLCall(glfwPollEvents());

	//freeing everything
	free(cellIndices);
	deletePositionOnScreen(cellPos);

	//red += 0.07;
	//green -= 0.1;
	//blue += 0.12;

	//if (red > 1.0f) red -= 1.0f;
	//if (green < 01.0f) green += 1.0f;
	//if (blue > 1.0f) blue -= 1.0f;
}

int main(int argc, char* argv[])
{
	GLFWwindow* window; //Window

	unsigned int buffer; //Vertex Bufffer
	unsigned int vao; // Vertex Array
	unsigned int ibo; //Index Buffer

	ShaderProgramSource shaderSource; //Source of the shaders
	unsigned int shaderId; //Id of shader in GPU

	int u_ColorLocation; // u_Color uniform location
	int u_holeColorLocation; // u_holeColor uniform location
	int u_sqrSizeLocation; // u_sqrSize location
	int u_gapSizeLocation; // u_gapSize location
	int u_holeSizeLocation; // u_gapSize location

	float* vertexPositions; //vertex coordinates
	long elapsedTime; //Time since last game tick

	switch (argc)
	{
	case 1:
		board = parseFile("res/init/cells.init");
		break;
	case 3:
		debug = (argv[2][0] == '0');
		printf("%d\n", debug);
	case 2:
		board = parseFile(argv[1]);
	default:
		break;
	}
	tmpBoard = board;

	if (initWindow(&window) == -1)
	{
		return -1;
	}
	glfwSetKeyCallback(window, key_callback);

	vertexPositions = generateVertexPositions();

	GLCall(glGenVertexArrays(1, &vao));
	GLCall(glBindVertexArray(vao));

	//Creating Vertex buffer
	GLCall(glGenBuffers(1, &buffer));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, (xCount + 1) * (yCount + 1) * 2 * sizeof(float), vertexPositions, GL_STATIC_DRAW));

	free(vertexPositions);
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0)); //Setting the layout of the buffer

	//Creating the Index Buffer
	GLCall(glGenBuffers(1, &ibo));

	//Getting the source code of the shader
	shaderSource = ParseShader("res/shaders/basic.shader");

	//Compiling and running shader
	shaderId = CreateShader(shaderSource.VertexSource, shaderSource.FragmentSource);
	GLCall(glUseProgram(shaderId));
	free(shaderSource.FragmentSource);
	free(shaderSource.VertexSource);

	//Getting the location of the uniforms
	GLCall(u_ColorLocation = glGetUniformLocation(shaderId, "u_Color"));
	ASSERT(u_ColorLocation != -1);

	GLCall(u_holeColorLocation = glGetUniformLocation(shaderId, "u_holeColor"));
	ASSERT(u_holeColorLocation != -1);

	GLCall(u_sqrSizeLocation = glGetUniformLocation(shaderId, "u_sqrSize"));
	ASSERT(u_sqrSizeLocation != -1);

	GLCall(u_gapSizeLocation = glGetUniformLocation(shaderId, "u_gapSize"));
	ASSERT(u_gapSizeLocation != -1);

	GLCall(u_holeSizeLocation = glGetUniformLocation(shaderId, "u_holeSize"));
	ASSERT(u_holeSizeLocation != -1);

	//Setting uniforms
	GLCall(glUniform1i(u_sqrSizeLocation, sqrSize));
	GLCall(glUniform1f(u_gapSizeLocation, gapSize));
	GLCall(glUniform1f(u_holeSizeLocation, 0));

	//Unbind everything. By binding 0, the binding will be reset
	GLCall(glBindVertexArray(0));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glUseProgram(0));

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(window))
	{
		drawToWindow(window, &buffer, vao, ibo, shaderId, u_ColorLocation, u_holeColorLocation, u_sqrSizeLocation, u_gapSizeLocation, u_holeSizeLocation, vertexPositions);
		elapsedTime = clock() - lastTick;

		if (tmpBoard == board)
		{
			tmpBoard = gameTick(board);
		}

		if (gameLoop && (elapsedTime > timePerTick || elapsedTime < 0 /*Can be negative if the program runs long enough and the value wraps around*/))
		{	
			deleteCell(board);
			board = tmpBoard;
			lastTick = clock();
		}

		if (debug)
		{
			//printf("%d ms\n", clock() - lastFrame);
			printf("%f fps, %d ms \n", 1000.0 / (clock() - lastFrame), (clock() - lastFrame));
			lastFrame = clock();
		}
	}

	//Clean up after running
	deleteCell(board);
	GLCall(glDeleteProgram(shaderId));

	glfwTerminate();
	_CrtDumpMemoryLeaks();
	return 0;
}