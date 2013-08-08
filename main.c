/*
 * (C) Nathan Geffen 2013 under GPL version 3.0. This is free software.
 * See the file called COPYING for the license.
 *
 * # Plain text drawing version of Game of Life
 *
 * This code will only be compiled if you substitute GUI=main on the command line
 * when calling make. Like so:
 *
 * make GUI=main.c
 *
 * It's faster and more robust than the GUI version, but it's less pretty.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "gameoflife.h"

void draw_grid(struct grid *grid,
	       const int from_x, const int from_y,
	       const int to_x, const int to_y)
{
	int x, y;
	for (y = from_y; y <= to_y; ++y) {
		for (x = from_x; x <= to_x; ++x)
			if (grid_at(grid, x, y))
				printf("*");
			else
				printf(" ");
		printf("\n");
	}
}

struct grid  *run_life(struct grid *grid, int step_size, int num_threads)
{
	unsigned i = 0;
	time_t t = 0;
	char input[10];

	while (strcmp(input, "q\n")) {
		draw_grid(grid, DRAW_LOW_X, DRAW_LOW_Y, DRAW_HIGH_X, DRAW_HIGH_Y);
		printf("Generations: %u. ", i);
		printf("Area of grid: (%d %d) - (%d %d). ",
		       grid->lowest_x, grid->lowest_y,
		       grid->highest_x, grid->highest_y);
		printf("Step size: %d\n", step_size);
		printf("Population size: %u. ", grid->population_size);
		printf("Time taken: %ld seconds\n", t);
		printf("Enter a step size or simply press enter to play. "
		       "Type 'q' and enter to quit.\n");
		strncpy(input, fgets(input, 10, stdin), 10);
		if ( strcmp(input, "q\n") ) {
			if (strcmp(input, "\n")) {
				step_size = atoi(input);
				if (step_size <= 1) step_size = 1;
			}
			t = time(NULL);
			if (num_threads == 1)
				grid = iterate_generations(grid, step_size);
			else
				grid = iterate_generations_t(grid,
							     step_size,
							     num_threads);
			t = time(NULL) - t;
			i += step_size;
		}
	}
	return grid;
}

void usage_message(const char *prog_name)
{
	fprintf(stderr, "Usage: %s -i rand|pent \n", prog_name);
}


int process_arguments(int argc, char *argv[], struct grid *grid,
		       int *step_size, int *num_threads)
{
	int opt, initialized = 0;
	*step_size = 1;
	*num_threads = 1;

	while ( (opt = getopt(argc, argv, "t:s:i:h") ) != -1) {
		switch (opt) {
		case 't':
			*num_threads = atoi(optarg);
			break;
		case 's':
			*step_size = atoi(optarg);
			break;
		case 'i':
			if (!strcmp(optarg, "rand"))
				break;
			if (!strcmp(optarg, "pent")) {
				initialized = 1;
				initial_r_pentomino(grid, -15, -15, 15, 15);
				break;
			}
		case 'h':
		default:
			fprintf(stderr, "%s [-t threads] [-s stepsize] "
				"[-i rand|pent]\n", argv[0]);
			return 0;
		}
	}
	if (!initialized)
		initial_random(grid, INIT_LOW_X, INIT_LOW_Y,
			       INIT_HIGH_X, INIT_HIGH_Y);
	return 1;
}

int main(int argc, char *argv[])
{
	int step_size, num_threads;
	struct grid *grid;

	grid = (struct grid *) malloc(sizeof(*grid));
	if (!grid) {
		fprintf(stderr, "Out of memory.");
		return 1;
	}
	if (!process_arguments(argc, argv, grid, &step_size, &num_threads))
		return 1;
	printf("Number of threads: %d\n", num_threads);
	printf("Default step size: %d\n", step_size);
	grid = run_life(grid, step_size, num_threads);
	free(grid);

	return 0;
}
