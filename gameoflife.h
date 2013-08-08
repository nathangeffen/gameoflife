/*
 * (C) Nathan Geffen 2013 under GPL version 3.0. This is free software.
 * See the file called COPYING for the license.
 *
 * # Definitions for Game of Life.
 *
 * This is an implementation of Game of Life using threads. It has been written
 * for teaching purposes only. The code and algorithms are far from optimal.
 *
 * Both the plain text and GUI versions draw a small subset of the 10 million
 * cell grid. The initialization functions, except for the random initializer,
 * clear the entire grid and set a small visible subset of the grid to a
 * specified pattern.
 */


#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#define GRID_DIM 10000 /* The game is played on a 10,000 x 10,000 grid. */
#define INIT_LOW_X -1000
#define INIT_LOW_Y -1000
#define INIT_HIGH_X 1000
#define INIT_HIGH_Y 1000
#define DRAW_LOW_X -12
#define DRAW_LOW_Y -12
#define DRAW_HIGH_X 12
#define DRAW_HIGH_Y 12
#define MAX_THREADS 10

/* This is the main data structure that keeps track of the game */

/* population: the grid itself. 1 is alive. 0 is dead. */
/* lowest_x: The lowest (or lowest less 1) x value that contains a live cell. */
/* lowest_y: The lowest (or lowest less 1) y value that contains a live cell. */
/* highest_x: The highest (or highest+1) x value that contains a live cell. */
/* highest_y: The highest (or highest+1) y value that contains a live cell. */
/* population_size: Number of live cells. */

struct grid {
	int population[GRID_DIM][GRID_DIM];

	int lowest_x;
	int lowest_y;
	int highest_x;
	int highest_y;
	unsigned population_size;
};


/* Get alive/dead state of grid->population[x][y] */
int grid_at(const struct grid *grid, const int x, const int y);

/* Set alive/dead state of grid->population[x][y] */
void grid_set(struct grid *grid, const int x, const int y, const int value);


/* The main loop, unthreaded version. */
struct grid *iterate_generations(struct grid *grid, const int generations);

/* The main loop, threaded version. */
struct grid *iterate_generations_t(struct grid *grid,
				   const int generations,
				   const int num_threads);

/* Initializes all cells to be randomly alive or dead. */
void initial_random(struct grid *grid,
		    const int lowest_x, const int lowest_y,
		    const int highest_x, const int highest_y);

/* Initializes an R-pentomino pattern on visible portion of the grid. */
void initial_r_pentomino(struct grid *grid,
			 const int lowest_x, const int lowest_y,
			 const int highest_x, const int highest_y);

/* Initializes a Gosper Gun on visible portion of the grid. */
void initial_gosper(struct grid *grid,
		    const int lowest_x, const int lowest_y,
		    const int highest_x, const int highest_y);

/* Kills every cell. */
void grid_clear(struct grid *grid);

#endif /* GAMEOFLIFE_H */
