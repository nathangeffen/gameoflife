#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gtk/gtk.h>

#include "gameoflife.h"

#define CELL_WIDTH 5
#define CELL_BORDER 1
#define MAX_PIXEL_X 1200
#define MAX_PIXEL_Y 600
#define CELL_X_DIM 200
#define CELL_Y_DIM 100
#define pos_cell(x, y) (x) * CELL_Y_DIM + (y)
#define pos_cell_y(cell_index) cell_index % CELL_Y_DIM
#define pos_cell_x(cell_index) \
	(cell_index - pos_cell_y(cell_index)) / CELL_Y_DIM
#define pixel_x(cell_x) cell_x * (CELL_WIDTH + CELL_BORDER)
#define pixel_y(cell_y) pixel_x(cell_y)

struct gui_grid {
        cairo_surface_t *surface;
	GtkWidget *da;
        GtkWidget *start_button;
        GtkWidget *iterations_val_label;
        GtkWidget *statusbar;
        GtkWidget *step_entry;
        GtkWidget *num_threads_entry;
        GtkWidget *initial_combo;
        GtkWidget *cell_widgets[CELL_X_DIM * CELL_Y_DIM];
        int cell[CELL_X_DIM * CELL_Y_DIM];
        struct grid *grid;
        unsigned step_size;
        int num_threads;
        int initialized;
        int in_progress;
        unsigned iterations;
        time_t secs;
};

static struct gui_grid *gui_grid;

static
void update_statusbar()
{
        gchar *str;
        gchar *time_str;


        time_str = g_strdup_printf("| Seconds: %ld",
                                   gui_grid->secs);
        str = g_strdup_printf("Iterations: %u | "
                              "Population: %u | "
                              "From: %d %d to %d %d "
                              "%s",
                              gui_grid->iterations,
                              gui_grid->grid->population_size,
                              gui_grid->grid->lowest_x,
                              gui_grid->grid->lowest_y,
                              gui_grid->grid->highest_x,
                              gui_grid->grid->highest_y,
                              time_str);
        gtk_statusbar_push(GTK_STATUSBAR(gui_grid->statusbar),
                           gtk_statusbar_get_context_id(
                                   GTK_STATUSBAR(gui_grid->statusbar),
                                   str), str);
        g_free(str);
        g_free(time_str);

}

static
void set_cell(int cell_x,
	      int cell_y,
	      int on_off,
	      int red,
	      int green,
	      int blue)
{
	cairo_t *cr;
	int index = pos_cell(cell_x, cell_y);

	cr = cairo_create (gui_grid->surface);
	if (gui_grid->cell[index] != on_off) {
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgb (cr, red, green, blue);
		gui_grid->cell[index] = on_off;
		cairo_rectangle (cr, pixel_x(cell_x) + CELL_BORDER,
				 pixel_y(cell_y) + CELL_BORDER ,
				 CELL_WIDTH - 1, CELL_WIDTH - 1);
		cairo_fill (cr);
		gtk_widget_queue_draw_area (gui_grid->da,
					    pixel_x(cell_x) + CELL_BORDER,
					    pixel_y(cell_y) + CELL_BORDER,
					    CELL_WIDTH - 1, CELL_WIDTH - 1);

	}
}


static
void cell_on(int cell_x,
	     int cell_y)
{
	set_cell(cell_x, cell_y, 1, 1, 0, 0);
}

static
void cell_off(int cell_x,
	      int cell_y)
{
	set_cell(cell_x, cell_y, 0, 1, 1, 1);
}

static
int get_cell_x(gdouble x) {
	return x < MAX_PIXEL_X ?
		(int) (x) / (CELL_WIDTH + CELL_BORDER) : CELL_X_DIM - 1;
}

static
int get_cell_y(gdouble y) {
	return y < MAX_PIXEL_Y ?
		(int) (y) / (CELL_WIDTH + CELL_BORDER) : CELL_Y_DIM - 1;
}

static
gboolean draw_cb (GtkWidget *widget,
		  cairo_t   *cr,
		  gpointer   data)
{
	cairo_set_source_surface (cr, gui_grid->surface, 0, 0);
	cairo_paint (cr);

	return FALSE;
}

static
void iterate_over_grid()
{
        int cell_value, x, y;
        size_t index;

        for (y = 0; y < CELL_Y_DIM; ++y)
                for (x = 0; x < CELL_X_DIM; ++x) {
                        index = pos_cell(x, y);
                        cell_value = grid_at(gui_grid->grid, x, y);
                        if (cell_value != gui_grid->cell[index]) {
                                if (cell_value)
                                        cell_on(x, y);
                                else
                                        cell_off(x, y);
                        }
                }
}


static void
clear_surface (void)
{
	int i;
	cairo_t *cr;

	cr = cairo_create (gui_grid->surface);

	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_paint (cr);

	cairo_set_source_rgb (cr, 0, 0, 1);
	cairo_set_line_width (cr, 0.1);


	for (i = 0; i <= CELL_Y_DIM; ++i) {
		cairo_move_to (cr, 0, (CELL_WIDTH + CELL_BORDER) * i);
		cairo_line_to (cr, MAX_PIXEL_X, (CELL_WIDTH + CELL_BORDER) * i);
		cairo_stroke (cr);
	}
	for (i = 0; i <= CELL_X_DIM; ++i) {
		cairo_move_to (cr, (CELL_WIDTH + CELL_BORDER) * i, 0);
		cairo_line_to (cr, (CELL_WIDTH + CELL_BORDER) * i, MAX_PIXEL_Y);
		cairo_stroke (cr);
	}
	cairo_destroy (cr);
}


static gboolean
configure_event_cb (GtkWidget         *widget,
                    GdkEventConfigure *event,
                    gpointer           data)
{
	if (gui_grid->surface)
		cairo_surface_destroy (gui_grid->surface);
	gui_grid->surface = gdk_window_create_similar_surface(
		gtk_widget_get_window (widget),
		CAIRO_CONTENT_COLOR,
		gtk_widget_get_allocated_width (widget),
		gtk_widget_get_allocated_height (widget));
	clear_surface ();
	memset(gui_grid->cell, 0, CELL_X_DIM * CELL_Y_DIM * sizeof(int) );
	iterate_over_grid();
	return TRUE;
}

static void
draw_brush (GtkWidget *widget,
	    gdouble    pix_x,
	    gdouble    pix_y)
{
	int cell_x, cell_y, index;

	cell_x = get_cell_x(pix_x);
	cell_y = get_cell_y(pix_y);
	index = pos_cell(cell_x, cell_y);
	pix_x = cell_x * (CELL_WIDTH + CELL_BORDER);
	pix_y = cell_y * (CELL_WIDTH + CELL_BORDER);
	if (gui_grid->cell[index])
		cell_off(cell_x, cell_y);
	else
		cell_on(cell_x, cell_y);
	grid_set(gui_grid->grid, cell_x, cell_y, gui_grid->cell[index]);
	update_statusbar();
}

static gboolean
motion_notify_event_cb (GtkWidget      *widget,
                        GdkEventMotion *event,
                        gpointer        data)
{
	if (gui_grid->surface == NULL)
		return FALSE;

	printf("Pixel x: %.2f y: %.2f; ", event->x, event->y);
	printf("Cell: x: %d, y: %d\n",
	       get_cell_x(event->x), get_cell_y(event->y));

	if (event->state & GDK_BUTTON1_MASK)
		draw_brush (widget, event->x, event->y);

	return TRUE;
}

static gboolean
button_press_event_cb (GtkWidget      *widget,
		       GdkEventButton *event,
		       gpointer        data)
{
	if (gui_grid->surface == NULL)
		return FALSE;

	if (event->button == GDK_BUTTON_PRIMARY)
	{
		draw_brush (widget, event->x, event->y);
	}
	else if (event->button == GDK_BUTTON_SECONDARY)
	{
		clear_surface ();
		gtk_widget_queue_draw (widget);
	}
	return TRUE;
}

static
void initialize_from_gui()
{
        gui_grid->grid->lowest_x = 0;
        gui_grid->grid->lowest_y = 0;
        gui_grid->grid->highest_x = CELL_X_DIM;
        gui_grid->grid->highest_y = CELL_Y_DIM;
}


gboolean update_cb(gpointer data)
{
        time_t start, stop;

        struct gui_grid *gui_grid = (struct gui_grid *) data;
        if (gui_grid->in_progress == 0) return FALSE;
        gui_grid->iterations += gui_grid->step_size;
        start = time(NULL);
        if (gui_grid->num_threads == 1)
                gui_grid->grid = iterate_generations(gui_grid->grid,
                                                     gui_grid->step_size);
        else
                gui_grid->grid =
                        iterate_generations_t(gui_grid->grid,
                                              gui_grid->step_size,
                                              gui_grid->num_threads);
        stop = time(NULL);
        gui_grid->secs = stop - start;
        iterate_over_grid();
        update_statusbar();
        return TRUE;
}

static
void clear_cb(GtkWidget *widget, gpointer data)
{
        int x, y;
        struct gui_grid *gui_grid = (struct gui_grid *) data;
        for (y = 0; y < CELL_Y_DIM; ++y)
                for (x = 0; x < CELL_X_DIM; ++x)
                        cell_off(x, y);
        grid_clear(gui_grid->grid);
        gui_grid->initialized = 0;
        gui_grid->iterations = 0;
        gui_grid->secs = 0;
        update_statusbar();
}

static
void setup_r_pentomino(struct gui_grid *gui_grid)
{
        initial_r_pentomino(gui_grid->grid,  -15, -15, 15, 15);
        iterate_over_grid();
        update_statusbar();
}

static
void setup_random(struct gui_grid *gui_grid, int low_x, int low_y,
                  int high_x, int high_y)
{
        initial_random(gui_grid->grid, low_x, low_y,
                       high_x, high_y);
        iterate_over_grid();
        update_statusbar();
}

static
void setup_gosper(struct gui_grid *gui_grid)
{
        initial_gosper(gui_grid->grid, 0, 0, 30, 30);
        iterate_over_grid();
        update_statusbar();
}

static
void initialize_gui_grid_cb(GtkWidget *widget, gpointer data)
{
        struct gui_grid *gui_grid = (struct gui_grid *) data;

        gchar *selection = gtk_combo_box_text_get_active_text(
                GTK_COMBO_BOX_TEXT(gui_grid->initial_combo));

        clear_cb(NULL, gui_grid);
        if (!strcmp(selection, "R-pentomino"))
                setup_r_pentomino(gui_grid);
        else if (!strcmp(selection, "Random screen"))
                setup_random(gui_grid, 0, 0, CELL_X_DIM, CELL_Y_DIM);
        else if (!strcmp(selection, "Random large"))
                setup_random(gui_grid,  INIT_LOW_X, INIT_LOW_Y,
                             INIT_HIGH_X, INIT_HIGH_Y);
        else if (!strcmp(selection, "Gosper"))
                setup_gosper(gui_grid);
        gui_grid->initialized = 1;
        g_free(selection);
}

static
void prepare_execute(GtkWidget *widget)
{
        if (!gui_grid->initialized) {
                initialize_from_gui();
                gui_grid->initialized = 1;
        }

        gui_grid->step_size = atoi(
                gtk_entry_get_text(GTK_ENTRY(gui_grid->step_entry)));
        if (gui_grid->step_size == 0) gui_grid->step_size = 1;
        gui_grid->num_threads = atoi(
                gtk_entry_get_text(GTK_ENTRY(gui_grid->num_threads_entry)));
        if (gui_grid->num_threads < 1)
                gui_grid->num_threads = 1;
        else if (gui_grid->num_threads > MAX_THREADS - 1)
                gui_grid->num_threads = MAX_THREADS - 1;
}

static
void step_cb(GtkWidget *widget, gpointer data)
{
        struct gui_grid *gui_grid = (struct gui_grid *) data;

        if (gui_grid->in_progress) return;
        prepare_execute(widget);
        gui_grid->in_progress = 1;
        update_cb(gui_grid);
        gui_grid->in_progress = 0;
}

static
void start_cb(GtkWidget *widget, gpointer data)
{
        struct gui_grid *gui_grid = (struct gui_grid *) data;

        if (gui_grid->in_progress) {
                gui_grid->in_progress = 0;
                gtk_button_set_label(GTK_BUTTON(widget), "Run");
        } else {
                gui_grid->in_progress = 1;
                gtk_button_set_label(GTK_BUTTON(widget), "Stop");
                prepare_execute(widget);
                g_timeout_add(100, (GSourceFunc) update_cb, gui_grid);
        }
}

static
void setup_grid(GtkWidget *box,
                struct gui_grid *gui_grid)
{
	GtkWidget *frame;
	GtkWidget *da;

	da = gtk_drawing_area_new ();
	g_signal_connect (da, "draw",
			  G_CALLBACK (draw_cb), NULL);
	g_signal_connect (da,"configure-event",
			  G_CALLBACK (configure_event_cb), NULL);
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
        gtk_container_add(GTK_CONTAINER(box), frame);

	gtk_widget_set_size_request (da, MAX_PIXEL_X, MAX_PIXEL_Y);
	gtk_container_add (GTK_CONTAINER (frame), da);

	g_signal_connect (da, "motion-notify-event",
			  G_CALLBACK (motion_notify_event_cb), NULL);
	g_signal_connect (da, "button-press-event",
			  G_CALLBACK (button_press_event_cb), NULL);

	gtk_widget_set_events (da, gtk_widget_get_events (da)
			       | GDK_BUTTON_PRESS_MASK
			       | GDK_POINTER_MOTION_MASK);
	gui_grid->da = da;

}

static
void setup_window(GtkWidget *window, struct gui_grid *gui_grid)
{
        GtkWidget *table;
        GtkWidget *top_box, *bottom_box;
        GtkWidget *start_button, *step_button, *clear_button;
        GtkWidget *initial_combo;

        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size(GTK_WINDOW(window), 1400, 700);
        gtk_window_set_title(GTK_WINDOW(window), "Game of Life");

        table = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
        gtk_container_add(GTK_CONTAINER(window), table);
        top_box = gtk_box_new (TRUE, 0);
        gtk_box_pack_start(GTK_BOX(table),
			   top_box, TRUE, TRUE, 0);
        bottom_box = gtk_box_new (FALSE, 10);
        gtk_box_pack_start(GTK_BOX(table),
			   bottom_box, FALSE, FALSE, 0);

        start_button = gtk_button_new_with_label("Run");
        gtk_box_pack_start (GTK_BOX (bottom_box), start_button,
                            TRUE, 5, 2);
        gtk_widget_show (start_button);
        g_signal_connect(G_OBJECT(start_button), "clicked",
                         G_CALLBACK(start_cb), gui_grid);

        step_button =  gtk_button_new_with_label("Step");
        gtk_box_pack_start (GTK_BOX (bottom_box), step_button,
                            TRUE, 10, 5);
        gtk_widget_show (step_button);
        g_signal_connect(G_OBJECT(step_button), "clicked",
                         G_CALLBACK(step_cb), gui_grid);
        clear_button = gtk_button_new_with_label("Clear");
        gtk_box_pack_start (GTK_BOX (bottom_box), clear_button,
                            TRUE, 10, 5);
        gtk_widget_show (clear_button);
        g_signal_connect(G_OBJECT(clear_button), "clicked",
                         G_CALLBACK(clear_cb), gui_grid);
        gtk_box_pack_start (GTK_BOX (bottom_box),
                            gtk_label_new("Step size:"), TRUE, 0, 0);
        gui_grid->step_entry = gtk_entry_new_with_buffer(
                gtk_entry_buffer_new("1", 1));
        gtk_box_pack_start (GTK_BOX (bottom_box), gui_grid->step_entry,
                            TRUE, 0, 0);
        gui_grid->num_threads_entry = gtk_entry_new_with_buffer(
                gtk_entry_buffer_new("1", 1));
        gtk_box_pack_start (GTK_BOX (bottom_box),
                            gtk_label_new("Threads:"), TRUE, 0, 0);
        gtk_box_pack_start (GTK_BOX (bottom_box), gui_grid->num_threads_entry,
                            TRUE, 0, 0);


        gtk_box_pack_start (GTK_BOX (bottom_box),
                            gtk_label_new("Initial position:"), TRUE, 0, 0);
        initial_combo = gtk_combo_box_text_new();
        gui_grid->initial_combo = initial_combo;
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(initial_combo),
                                       "Random screen");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(initial_combo),
                                       "Random large");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(initial_combo),
                                       "R-pentomino");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(initial_combo),
                                       "Gosper");
        g_signal_connect(G_OBJECT(initial_combo), "changed",
                         G_CALLBACK(initialize_gui_grid_cb), gui_grid);

        gtk_box_pack_start (GTK_BOX (bottom_box), initial_combo,
                            TRUE, 0, 0);

        gui_grid->statusbar = gtk_statusbar_new();
        gtk_box_pack_start(GTK_BOX(table), gui_grid->statusbar,
			   FALSE, FALSE, 25);
        setup_grid(top_box, gui_grid);
        g_signal_connect_swapped(G_OBJECT(window), "destroy",
                                 G_CALLBACK(gtk_main_quit), NULL);
        update_statusbar();
        gtk_widget_show_all(window);
}

int main( int argc, char *argv[])
{
        GtkWidget *window;
        struct grid *grid;

        gtk_init(&argc, &argv);
        gui_grid = (struct gui_grid *) malloc(sizeof(struct gui_grid));
        if (!gui_grid) {
                fprintf(stderr, "Out of memory for gui.");
                return 1;
        }
        grid = (struct grid *) malloc(sizeof(struct grid));
        if (!grid) {
                fprintf(stderr, "Out of memory for grid.");
                return 1;
        }
	gui_grid->surface = NULL;
        gui_grid->grid = grid;
        gui_grid->initialized = 0;
        gui_grid->in_progress = 0;
        gui_grid->step_size = 1;
        gui_grid->iterations = 0;
        gui_grid->secs = 0;
        gui_grid->num_threads = 1;
        grid_clear(gui_grid->grid);
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        setup_window(window, gui_grid);
        gtk_widget_show_all(window);
        gtk_main();
        free(gui_grid->grid);
        free(gui_grid);
        return 0;
}
