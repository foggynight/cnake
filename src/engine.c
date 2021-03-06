#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "engine.h"
#include "screen.h"

struct PlayerNode {
	int y_pos;
	int x_pos;
	struct PlayerNode *next;
};

struct Food {
	int y_pos;
	int x_pos;
	char symbol;
};

static char grid[GRID_HEIGHT][GRID_WIDTH];

static struct PlayerNode *head;
static char direction = 'd';

static struct Food *food_array[FOOD_MAX];
static size_t food_array_ptr = 0;
static unsigned int food_timer = 0;

static unsigned long score = 0;
static unsigned int cnake_length = 1;
static unsigned int score_timer = 0;

static void grid_clear(void);
static void create_food(int number);
static void delete_food(int y_pos, int x_pos);

static int is_solid(int y_pos, int x_pos);
static int is_snake(int y_pos, int x_pos);
static int is_food(int y_pos, int x_pos);

void engine_setup(void)
{
	grid_clear();
	screen_setup();

	srand(clock());

	head = malloc(sizeof(struct PlayerNode));
	head->y_pos = 0;
	head->x_pos = 0;
	head->next = NULL;

	create_food(FOOD_START);
}

int engine_step(void)
{
	int ch = getch();
	if (ch == 'w' || ch == 'd' || ch == 's' || ch == 'a') {
		if (head->next == NULL) {
			direction = ch;
		}
		else {
			switch (direction) {
				case 'w' : if (ch != 's') direction = ch; break;
				case 'a' : if (ch != 'd') direction = ch; break;
				case 's' : if (ch != 'w') direction = ch; break;
				case 'd' : if (ch != 'a') direction = ch; break;
			}
		}
	}

	int y_next = head->y_pos;
	int x_next = head->x_pos;

	switch (direction) {
		case 'w' : y_next -= 1; break;
		case 'a' : x_next -= 1; break;
		case 's' : y_next += 1; break;
		case 'd' : x_next += 1; break;
	}

	/* If the next element is wall or cnake, gameover. */
	if (is_solid(y_next, x_next) || is_snake(y_next, x_next)) {
		return 0;
	}

	/* Next cell becomes new head, with the old head as it's next node. */
	{
		struct PlayerNode *temp = malloc(sizeof(struct PlayerNode));
		temp->y_pos = y_next;
		temp->x_pos = x_next;
		temp->next = head;

		head = temp;
		++cnake_length;
	}

	/* If next element was a piece of food, delete the food. */
	if (is_food(y_next, x_next)) {
		delete_food(y_next, x_next);
	}
	/* Else delete the last node of the cnake. */
	else {
		struct PlayerNode *current = head;
		while (current->next->next != NULL) {
			current = current->next;
		}
		free(current->next);
		current->next = NULL;
		--cnake_length;
	}

	/* Update cnake cells. */
	{
		grid_clear();

		switch (direction) {
			case 'w': grid[head->y_pos][head->x_pos] = 'A'; break;
			case 'a': grid[head->y_pos][head->x_pos] = '<'; break;
			case 's': grid[head->y_pos][head->x_pos] = 'V'; break;
			case 'd': grid[head->y_pos][head->x_pos] = '>'; break;
		}

		struct PlayerNode *temp = head->next;
		while (temp != NULL) {
			grid[temp->y_pos][temp->x_pos] = '+';
			temp = temp->next;
		}
	}

	if (food_timer > TIME_FOOD * FRAME_RATE) {
		create_food(1);
		food_timer = -1;
	}

	/* Update food cells. */
	for (size_t i = 0; i < food_array_ptr; ++i) {
		struct Food *food_item = food_array[i];
		grid[food_item->y_pos][food_item->x_pos] = food_item->symbol;
	}

	if (score_timer > TIME_SCORE * FRAME_RATE) {
		score += cnake_length;
		score_timer = -1;
	}

	++food_timer;
	++score_timer;
	screen_update(grid, score);

	return 1;
}

unsigned long engine_kill(void)
{
	screen_kill();
	return score;
}

static void grid_clear(void)
{
	for (int i = 0; i < GRID_HEIGHT * GRID_WIDTH; ++i)
		grid[i / GRID_WIDTH][i % GRID_WIDTH] = ' ';
}

static void create_food(int number)
{
	for (int i = 0; food_array_ptr < FOOD_MAX && i < number; ++i) {
		int y, x;
		do {
			y = rand() % GRID_HEIGHT;
			x = rand() % GRID_WIDTH;
		} while (is_snake(y, x));

		struct Food *food_item = malloc(sizeof(struct Food));
		(*food_item).y_pos = y;
		(*food_item).x_pos = x;
		(*food_item).symbol = 'F';

		food_array[food_array_ptr++] = food_item;
	}
}

static void delete_food(int y_pos, int x_pos)
{
	size_t temp_ptr = 0;

	while (temp_ptr < food_array_ptr) {
		if (food_array[temp_ptr]->y_pos == y_pos
				&& food_array[temp_ptr]->x_pos == x_pos)
			break;
		++temp_ptr;
	}

	while (temp_ptr < food_array_ptr-1) {
		food_array[temp_ptr] = food_array[temp_ptr + 1];
		++temp_ptr;
	}

	--food_array_ptr;
}

static int is_solid(int y_pos, int x_pos)
{
	if (y_pos < 0 || y_pos >= GRID_HEIGHT || x_pos < 0 || x_pos >= GRID_WIDTH)
		return 1;
	else if (is_snake(y_pos, x_pos))
		return 1;

	return 0;
}

static int is_snake(int y_pos, int x_pos)
{
	switch (grid[y_pos][x_pos]) {
		case 'A':
		case '<':
		case 'V':
		case '>':
		case '+':
			return 1;
		default: return 0;
	}
}

static int is_food(int y_pos, int x_pos)
{
	for (size_t i = 0; i < food_array_ptr; ++i)
		if (food_array[i]->y_pos == y_pos && food_array[i]->x_pos == x_pos)
			return 1;

	return 0;
}
