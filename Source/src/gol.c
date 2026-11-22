#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "gol.h"

int camSizeX = 90;
int camSizeY = 45;
long long camPosY = -(45 / 2);
long long camPosX = -(90 / 2);

cell* createCell(long long x, long long y)
{
	cell* c = (cell*)malloc(sizeof(cell));
	c->x = x;
	c->y = y;
	c->next = NULL;
	c->valid = true;
	return c;
}

void deleteCell(cell* c)
{
	cell* next;

	while (c != NULL)
	{
		next = c->next;
		free(c);
		c = next;
	}
}

unsigned int countCells(cell* c)
{
	unsigned int count = 0;

	for (; c != NULL; c = c->next)
	{
		count++;
	}
	return count;
}

bool isCellAlive(cell* c, long long x, long long y)
{
	for (; c != NULL; c = c->next)
	{
		if (c->x == x && c->y == y)
		{
			return true;
		}
	}
	return false;
}

cell* setCell(cell* c, long long x, long long y, bool alive)
{
	cell* prev = NULL;
	cell* tmp;

	for (tmp = c; tmp != NULL; prev = tmp, tmp = tmp->next)
	{
		if (tmp->x == x && tmp->y == y)
		{
			if (alive)
			{
				return c; //Already alive
			}

			//Unlink and free
			if (prev == NULL)
			{
				c = tmp->next;
			}
			else
			{
				prev->next = tmp->next;
			}
			free(tmp);
			return c;
		}
	}

	if (alive)
	{
		tmp = createCell(x, y);
		tmp->next = c;
		return tmp;
	}
	return c;
}

///Next generation, using a hash map of every place that is alive or next to a living cell

typedef struct place place;

struct place
{
	long long x;
	long long y;
	unsigned char neighbourCount;
	bool isALive;
	bool used;
};

static place* findPlace(place* places, size_t mask, long long x, long long y)
{
	unsigned long long h = (unsigned long long)x * 0x9E3779B97F4A7C15ULL;
	size_t i;

	h ^= (unsigned long long)y * 0xC2B2AE3D27D4EB4FULL;
	h ^= h >> 29;
	i = (size_t)h & mask;

	//Linear probing
	while (places[i].used && (places[i].x != x || places[i].y != y))
	{
		i = (i + 1) & mask;
	}

	if (!places[i].used)
	{
		places[i].used = true;
		places[i].x = x;
		places[i].y = y;
	}
	return &places[i];
}

cell* gameTick(cell* c)
{
	cell* tmp;
	cell* head = NULL;
	cell* tail = NULL;
	place* places;
	size_t capacity = 64;
	size_t i;
	unsigned int count = countCells(c);
	int dx, dy;

	if (count == 0)
	{
		return NULL;
	}

	//Every cell touches at most 9 places, keep the table at most half full
	while (capacity < (size_t)count * 18)
	{
		capacity *= 2;
	}
	places = (place*)calloc(capacity, sizeof(place));

	for (tmp = c; tmp != NULL; tmp = tmp->next)
	{
		findPlace(places, capacity - 1, tmp->x, tmp->y)->isALive = true;

		for (dx = -1; dx <= 1; dx++)
		{
			//The edges of the world
			if ((dx < 0 && tmp->x == LONGLONG_MIN) || (dx > 0 && tmp->x == LONGLONG_MAX))
				continue;

			for (dy = -1; dy <= 1; dy++)
			{
				if ((dx == 0 && dy == 0) ||
					(dy < 0 && tmp->y == LONGLONG_MIN) || (dy > 0 && tmp->y == LONGLONG_MAX))
					continue;

				findPlace(places, capacity - 1, tmp->x + dx, tmp->y + dy)->neighbourCount++;
			}
		}
	}

	for (i = 0; i < capacity; i++)
	{
		if (!places[i].used)
			continue;

		if (places[i].neighbourCount == 3 || (places[i].isALive && places[i].neighbourCount == 2))
		{
			tmp = createCell(places[i].x, places[i].y);

			if (tail == NULL)
			{
				head = tmp;
			}
			else
			{
				tail->next = tmp;
			}
			tail = tmp;
		}
	}
	free(places);
	return head;
}

cell* parseFile(const char* filePath)
{
	FILE* file = fopen(filePath, "r");
	char line[MAX_LINE_LENGTH] = { 0 };
	long long x, y;

	cell* c = NULL;
	cell* tmp = NULL;

	if (!file)
	{
		perror(filePath);
		return NULL;
	}

	/* Get each line until there are none left. Format: {x}; {y}\\Comment */
	while (fgets(line, MAX_LINE_LENGTH, file))
	{
		if (sscanf(line, " %lld ; %lld", &x, &y) != 2)
		{
			continue; // Skip empty or malformed lines
		}

		//Ignore duplicates so every cell is in the list once
		if (isCellAlive(c, x, y))
		{
			continue;
		}

		if (c == NULL)
		{
			c = createCell(x, y);
			tmp = c;
			continue;
		}

		tmp->next = createCell(x, y);
		tmp = tmp->next;
	}
	fclose(file);
	return c;
}
