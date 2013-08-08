/*
 * (C) Nathan Geffen 2013 under GPL version 3.0. This is free software.
 * See the file called COPYING for the license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "gameoflife.h"

pthread_mutex_t mutex;

struct thread_data {
	struct grid *old_grid;
	struct grid *new_grid;
	int thread;
	int thread_size;
};

int grid_at(const struct grid *grid, const int x, const int y)
{
	if (x >= GRID_DIM / 2 || x < -GRID_DIM / 2 ||
	    y >= GRID_DIM / 2 || y < -GRID_DIM / 2)
		fprintf(stderr, "Out of bounds: %d %d", x, y);
	else
		return grid->population[GRID_DIM / 2 + x][GRID_DIM / 2 + y];
	return 0;
}

void grid_set(struct grid *grid,
	      int          x,
	      int          y,
	      int          value)
{
	if (x >= GRID_DIM / 2 || x < -GRID_DIM / 2 ||
	    y >= GRID_DIM / 2 || y < -GRID_DIM / 2)
		fprintf(stderr, "Out of bounds: %d %d", x, y);
	else {
		if (value !=
		    grid->population[GRID_DIM / 2 + x][GRID_DIM / 2 + y]) {
			grid->population[GRID_DIM / 2 + x][GRID_DIM / 2 + y] =
				value;
			pthread_mutex_lock (&mutex);
			if (value) {
				++grid->population_size;
				if (x <= grid->lowest_x)
					grid->lowest_x = x - 1;
				else if (x >= grid->highest_x)
					grid->highest_x = x + 1;
				if (y <= grid->lowest_y)
					grid->lowest_y = y - 1;
				else if (y >= grid->highest_y)
					grid->highest_y = y + 1;
			} else {
				--grid->population_size;
			}
			pthread_mutex_unlock(&mutex);
		}
	}
}

static int process_cell(struct grid *grid, const int x, const int y,
			struct grid *new_grid)
{
	unsigned neighbors =
		grid_at(grid, x - 1, y - 1) +
		grid_at(grid, x - 1, y) +
		grid_at(grid, x - 1, y + 1) +
		grid_at(grid, x, y - 1) +
		grid_at(grid, x, y + 1) +
		grid_at(grid, x + 1, y - 1) +
		grid_at(grid, x + 1, y) +
		grid_at(grid, x + 1, y + 1);

	if (grid_at(grid, x, y)) {
		if (neighbors < 2 || neighbors > 3)
			grid_set(new_grid, x, y, 0);
		else
			grid_set(new_grid, x, y, 1);
	} else {
		if (neighbors == 3)
			grid_set(new_grid, x, y, 1);
		else
			grid_set(new_grid, x, y, 0);
	}
	return grid_at(new_grid, x, y);
}


struct grid *iterate_generations(struct grid *grid, const int generations)
{
	int x, y, i;
	struct grid *new_grid;
	int lowest_x, highest_x;
	int lowest_y, highest_y;

	for (i = 0; i < generations; ++i) {
		lowest_x = grid->lowest_x - 1;
		highest_x = grid->highest_x + 1;
		lowest_y = grid->lowest_y - 1;
		highest_y = grid->highest_y + 1;

		new_grid = (struct grid *) malloc(sizeof(*new_grid));
		if (!new_grid) {
			fprintf(stderr, "Out of memory\n");
			return NULL;
		}
		new_grid->population_size = 0;
		for (x = lowest_x; x <= highest_x; ++x)
			for (y = lowest_y; y <= highest_y; ++y)
				process_cell(grid, x, y, new_grid);

		free(grid);
		grid = new_grid;
	}
	return grid;
}


static void *process_cell_section(void *data)
{
	struct thread_data *thread_data = (struct thread_data *) data;
	struct grid *old_grid = thread_data->old_grid;
	struct grid *new_grid = thread_data->new_grid;
	int thread_size = thread_data->thread_size;
	int t = thread_data->thread;
	int lowest_x, highest_x, lowest_y, highest_y, start, end;
	int x, y;

	lowest_x = old_grid->lowest_x - 1;
	highest_x = old_grid->highest_x + 1;
	lowest_y = old_grid->lowest_y - 1;
	highest_y = old_grid->highest_y + 1;
	start = lowest_x + t * thread_size;
	end = start + thread_size;
	end = end > highest_x + 1 ? highest_x + 1: end;
	for (x = start; x < end; ++x)
		for (y = lowest_y; y <= highest_y; ++y)
			process_cell(old_grid, x, y, new_grid);
	pthread_exit(NULL);
}


struct grid *iterate_generations_t(struct grid *grid,
				   const int generations, int num_threads)
{
	int i;
	struct grid *new_grid;
	int lowest_x, highest_x;
	pthread_t threads[MAX_THREADS];
	struct thread_data thread_data[MAX_THREADS];
	void *status;
	int t, thread_size, rc;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	for (i = 0; i < generations; ++i) {
		lowest_x = grid->lowest_x - 1;
		highest_x = grid->highest_x + 1;
		new_grid = (struct grid *) malloc(sizeof(*new_grid));
		if (!new_grid) {
			fprintf(stderr, "Out of memory\n");
			return NULL;
		}
		new_grid->population_size = 0;
		thread_size = (highest_x + 1 - lowest_x) / num_threads;
		for (t = 0; t <= num_threads; ++t) {
			thread_data[t].old_grid = grid;
			thread_data[t].new_grid = new_grid;
			thread_data[t].thread = t;
			thread_data[t].thread_size = thread_size;
			rc = pthread_create(&threads[t], &attr,
					    process_cell_section,
					    (void *) &thread_data[t]);
			if (rc) {
				fprintf(stderr, "Can't create thread %d\n", rc);
				return NULL;
			}
		}
		pthread_attr_destroy(&attr);
		for(t = 0; t <= num_threads; ++t) {
			rc = pthread_join(threads[t], &status);
			if (rc) {
				fprintf(stderr,
					"Error joining thread is %d\n", rc);
				return NULL;
			}
		}
		free(grid);
		grid = new_grid;
	}
	return grid;
}

void initial_random(struct grid *grid, const int lowest_x, const int lowest_y,
		    const int highest_x, const int highest_y)
{
	int x, y;
	grid->lowest_x = lowest_x;
	grid->lowest_y = lowest_y;
	grid->highest_x = highest_x;
	grid->highest_y = highest_y;
	grid->population_size = 0;

	for (x = lowest_x; x <= highest_x; ++x)
		for (y = lowest_y; y <= highest_y; ++y)
			if (rand() <= RAND_MAX / 2)
				grid_set(grid, x , y, 1);
			else
				grid_set(grid, x , y, 0);
}

void initial_r_pentomino(struct grid *grid, const int lowest_x, const int lowest_y,
			 const int highest_x, const int highest_y)
{
	grid->lowest_x = lowest_x;
	grid->lowest_y = lowest_y;
	grid->highest_x = highest_x;
	grid->highest_y = highest_y;

	grid_set(grid, 10, 10, 1);
	grid_set(grid, 11, 10, 1);
	grid_set(grid, 11, 11, 1);
	grid_set(grid, 11, -1+10, 1);
	grid_set(grid, 12, -1+10, 1);
}

void initial_gosper(struct grid *grid, const int lowest_x, const int lowest_y,
			 const int highest_x, const int highest_y)
{
	grid->lowest_x = lowest_x;
	grid->lowest_y = lowest_y;
	grid->highest_x = highest_x;
	grid->highest_y = highest_y;

	grid_set(grid, 2, 6, 1);
	grid_set(grid, 3, 6, 1);
	grid_set(grid, 2, 7, 1);
	grid_set(grid, 3, 7, 1);
	grid_set(grid, 14, 4, 1);
	grid_set(grid, 15, 4, 1);
	grid_set(grid, 13, 5, 1);
	grid_set(grid, 17, 5, 1);
	grid_set(grid, 12, 6, 1);
	grid_set(grid, 12, 7, 1);
	grid_set(grid, 12, 8, 1);
	grid_set(grid, 13, 9, 1);
	grid_set(grid, 14, 10, 1);
	grid_set(grid, 15, 10, 1);
	grid_set(grid, 15, 10, 1);
	grid_set(grid, 16, 7, 1);
	grid_set(grid, 18, 6, 1);
	grid_set(grid, 18, 7, 1);
	grid_set(grid, 19, 7, 1);
	grid_set(grid, 18, 8, 1);
	grid_set(grid, 17, 9, 1);
	grid_set(grid, 22, 4, 1);
	grid_set(grid, 23, 4, 1);
	grid_set(grid, 22, 5, 1);
	grid_set(grid, 23, 5, 1);
	grid_set(grid, 22, 6, 1);
	grid_set(grid, 23, 6, 1);
	grid_set(grid, 24, 3, 1);
	grid_set(grid, 24, 7, 1);
	grid_set(grid, 26, 2, 1);
	grid_set(grid, 26, 3, 1);
	grid_set(grid, 26, 7, 1);
	grid_set(grid, 26, 8, 1);
	grid_set(grid, 36, 4, 1);
	grid_set(grid, 36, 5, 1);
	grid_set(grid, 37, 4, 1);
	grid_set(grid, 37, 5, 1);
}



void grid_clear(struct grid *grid)
{
	int x, y;
	for (x = 0; x < GRID_DIM; ++x)
		for (y = 0; y < GRID_DIM; ++y)
			grid->population[x][y] = 0;
	grid->lowest_x = 0;
	grid->lowest_y = 0;
	grid->highest_x = 0;
	grid->highest_y = 0;
	grid->population_size = 0;
}
