#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "gol.h"

long long camPosY = -(camSizeY / 2);
long long camPosX = -(camSizeX / 2);

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
	if (c == NULL)
	{
		return;
	}

	if (c->next != NULL)
	{
		deleteCell(c->next);
	}
	free(c);
}

place* createPlace(long long x, long long y, bool isALive)
{
	place* p = (place*)malloc(sizeof(place));
	p->x = x;
	p->y = y;
	p->isALive = isALive;
	p->neighbourCount = 0;
	p->next = NULL;
	return p;
}

void deletePlace(place* p)
{
	if (p == NULL)
	{
		return;
	}

	if (p->next != NULL)
	{
		deletePlace(p->next);
	}
	free(p);
}

place* addPlaceToPlaceList(long long x, long long y, place* plc)
{
	place* tmp = plc;

	while (1)
	{
		if (tmp->x == x && tmp->y == y)
		{
			tmp->neighbourCount++;
			return plc;
		}

		if (tmp->next == NULL)
		{
			break;
		}

		tmp = tmp->next;
	}
	tmp->next = createPlace(x, y, false);
	tmp->next->neighbourCount = 1;
	return plc;
}

place* addCellToPlaceList(long long x, long long y, place* plc)
{
	place* tmp = plc;

	while (1)
	{
		if (tmp->x == x && tmp->y == y)
		{
			tmp->isALive = true;
			return plc;
		}

		if (tmp->next == NULL)
		{
			break;
		}

		tmp = tmp->next;
	}
	tmp->next = createPlace(x, y, false);
	tmp->next->isALive = true;
	return plc;
}

void deletePositionOnScreen(positionOnScreen* p)
{
	if (p == NULL)
	{
		return;
	}

	if (p->next != NULL)
	{
		deletePositionOnScreen(p->next);
	}
	free(p);
}

cell* getCellsFromPlaceList(place* p)
{
	place* tmp = p;
	cell* c;

	if (tmp->isALive)
	{
		if (tmp->neighbourCount == 2 || tmp->neighbourCount == 3)
		{
			c = createCell(tmp->x, tmp->y);

			if (tmp->next != NULL)
			{
				c->next = getCellsFromPlaceList(tmp->next);

				if (c->next == NULL || !c->next->valid)
				{
					free(c->next);
					c->next = NULL;
				}
			}
			else
			{
				c->next = NULL;
			}
			return c;
		}

		if (tmp->next != NULL)
		{
			return getCellsFromPlaceList(tmp->next);
		}
		return NULL;

	}
	else
	{
		if (tmp->neighbourCount == 3)
		{
			c = createCell(tmp->x, tmp->y);

			if (tmp->next != NULL)
			{
				c->next = getCellsFromPlaceList(tmp->next);

				if (c->next == NULL || !c->next->valid)
				{
					free(c->next);
					c->next = NULL;
				}
			}
			else
			{
				c->next = NULL;
			}
			return c;
		}

		if (tmp->next != NULL)
		{
			return getCellsFromPlaceList(tmp->next);
		}
		return NULL;
	}
}

positionOnScreen* addToPositionListIfDoesntExist(positionOnScreen* places, int x, int y, unsigned int* count)
{
	positionOnScreen* tmp = places;

	if (tmp == NULL)
	{
		tmp = (positionOnScreen*)malloc(sizeof(positionOnScreen));
		tmp->x = x;
		tmp->y = y;
		tmp->next = NULL;
		(*count)++;
		return tmp;
	}

	while (1)
	{
		if (tmp->x == x && tmp->y == y)
		{
			return places;
		}

		if (tmp->next == NULL)
		{
			break;
		}
		tmp = tmp->next;
	}
	tmp->next = (positionOnScreen*)malloc(sizeof(positionOnScreen));
	tmp = tmp->next;
	tmp->x = x;
	tmp->y = y;
	tmp->next = NULL;
	(*count)++;
	return places;
}

positionOnScreen* getCellPositions(cell* cl)
{
	int x, y;
	cell* tmpCl = cl;
	positionOnScreen* p = NULL;
	positionOnScreen* tmpP = NULL;

	while (1)
	{
		if (!tmpCl->valid)
		{
			return NULL;
		}

		if (camPosX <= tmpCl->x && (camPosX + camSizeX) > tmpCl->x)
		{
			if (camPosY <= tmpCl->y && (camPosY + camSizeY) > tmpCl->y)
			{
				x = tmpCl->x - camPosX;
				y = tmpCl->y - camPosY;

				if (p == NULL)
				{
					p = (positionOnScreen*)malloc(sizeof(positionOnScreen));
					p->x = x;
					p->y = y;
					p->next = NULL;
					tmpP = p;
				}
				else
				{
					tmpP->next = (positionOnScreen*)malloc(sizeof(positionOnScreen));
					tmpP = tmpP->next;
					tmpP->x = x;
					tmpP->y = y;
					tmpP->next = NULL;
				}
			}
		}

		if (tmpCl->next == NULL)
		{
			break;
		}
		tmpCl = tmpCl->next;
	}
	return p;
}

positionOnScreen* getHaloPositions(unsigned int* count, positionOnScreen* places)
{
	positionOnScreen* p = NULL;
	positionOnScreen* tmp = places;

	if (places == NULL)
	{
		return NULL;
	}

	while (1)
	{
		p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y, count);

		switch (tmp->x)
		{

		case 0:
			switch (tmp->y)
			{
			case 0:
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y, count);
				break;

			case camSizeY - 1:
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y, count);
				break;

			default:
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y, count);
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y - 1, count);
				break;
			}
			break;

		case camSizeX - 1:
			switch (tmp->y)
			{
			case 0:
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y, count);
				break;

			case camSizeY - 1:
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y, count);
				break;

			default:
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y, count);
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y - 1, count);
				break;
			}
			break;

		default:

			switch (tmp->y)
			{
			case 0:
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y, count);
				break;

			case camSizeY - 1:
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y, count);
				break;

			default:
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y + 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y, count);
				p = addToPositionListIfDoesntExist(p, tmp->x - 1, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x, tmp->y - 1, count);
				p = addToPositionListIfDoesntExist(p, tmp->x + 1, tmp->y - 1, count);
				break;
			}
			break;
		}

		if (tmp->next == NULL)
		{
			break;
		}
		tmp = tmp->next;
	}
	return p;
}

cell* parseFile(const char* filePath)
{
	FILE* file = fopen(filePath, "r");
	char line[MAX_LINE_LENGTH] = { 0 };
	unsigned int line_count = 0;
	int i;
	long long x, y;
	bool minus;

	cell* c = NULL;
	cell* tmp = NULL;

	if (!file)
	{
		perror(filePath);
		return NULL;
	}

	/* Get each line until there are none left */
	while (fgets(line, MAX_LINE_LENGTH, file))
	{
		x = y = i = 0;
		minus = false;

		if (line[i] == '-')
		{
			minus = true;
			i++;
		}

		while (line[i] != ';')
		{
			x *= 10;
			x += line[i] - '0';
			i++;
		}
		x = minus ? -x : x;
		i += 2;
		minus = false;

		if (line[i] == '-')
		{
			minus = true;
			i++;
		}

		while (line[i] != '\n' && line[i] != '\0' && line[i] != '\\')
		{
			y *= 10;
			y += line[i] - '0';
			i++;
		}
		y = minus ? -y : y;

		if (c == NULL)
		{
			c = (cell*)malloc(sizeof(cell));
			c->valid = true;
			c->x = x;
			c->y = y;
			c->next = NULL;
			tmp = c;
			continue;
		}

		tmp->next = (cell*)malloc(sizeof(cell));
		tmp = tmp->next;
		tmp->valid = true;
		tmp->x = x;
		tmp->y = y;
		tmp->next = NULL;
	}
	return c;
}

cell* gameTick(cell* c)
{
	cell* tmp = c;
	place* plc = createPlace(c->x, c->y, true);
	place* tmpPlc;
	//tmp->next = c->next;

	while (1)
	{
		switch (tmp->x)
		{
		case LONGLONG_MIN:
		{
			switch (tmp->y)
			{
			case LONGLONG_MIN:
				addPlaceToPlaceList(tmp->x, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y, plc);
				break;

			case LONGLONG_MAX:
				addPlaceToPlaceList(tmp->x, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y, plc);
				break;

			default:
				addPlaceToPlaceList(tmp->x, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x, tmp->y - 1, plc);
				break;
			}
			break;
		}

		case LONGLONG_MAX:
		{
			switch (tmp->y)
			{
			case LONGLONG_MIN:
				addPlaceToPlaceList(tmp->x, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y, plc);
				break;

			case LONGLONG_MAX:
				addPlaceToPlaceList(tmp->x, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y, plc);
				break;

			default:
				addPlaceToPlaceList(tmp->x, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x, tmp->y - 1, plc);
				break;
			}
			break;
		}

		default:
		{
			switch (tmp->y)
			{
			case LONGLONG_MIN:
				addPlaceToPlaceList(tmp->x - 1, tmp->y, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y, plc);
				break;

			case LONGLONG_MAX:
				addPlaceToPlaceList(tmp->x - 1, tmp->y, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y, plc);
				break;

			default:
				addPlaceToPlaceList(tmp->x - 1, tmp->y, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x, tmp->y + 1, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y + 1, plc);

				addPlaceToPlaceList(tmp->x + 1, tmp->y, plc);
				addPlaceToPlaceList(tmp->x + 1, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x, tmp->y - 1, plc);
				addPlaceToPlaceList(tmp->x - 1, tmp->y - 1, plc);
				break;
			}
			break;
		}
		}

		if (tmp->next == NULL)
		{
			break;
		}
		tmp = tmp->next;
	}

	tmp = c;

	tmpPlc = plc;

	while (1)
	{
		addCellToPlaceList(tmp->x, tmp->y, plc);

		if (tmp->next == NULL)
		{
			break;
		}

		tmp = tmp->next;
	}
	//deleteCell(c);

	tmp = getCellsFromPlaceList(plc);
	deletePlace(plc);
	return tmp;
}

void drawToConsole(cell cl)
{
	char scr[camSizeX * camSizeY];
	int i, j, x, y;
	system("cls");

	for (i = 0; i < camSizeX * camSizeY; i++)
	{
		scr[i] = ' ';
	}

	while (1)
	{
		if (!cl.valid)
		{
			return;
		}

		if (camPosX < cl.x && (camPosX + camSizeX) > cl.x)
		{
			if (camPosY < cl.y && (camPosY + camSizeY) > cl.y)
			{
				x = cl.x - camPosX;
				y = camSizeY - (cl.y - camPosY);
				scr[y * camSizeX + x] = 'X';
			}
		}

		if (cl.next == NULL)
		{
			break;
		}
		cl = *cl.next;
	}

	for (i = 0; i < camSizeY; i++)
	{
		for (j = 0; j < camSizeX; j++)
		{
			printf("%c", scr[i * camSizeX + j]);
		}
		printf("\n");
	}
}