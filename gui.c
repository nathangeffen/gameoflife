#include <gtk/gtk.h>

gint count = 0;
char buf[5];

void increase(GtkWidget *widget, gpointer label)
{
	count++;

	sprintf(buf, "%d", count);
	gtk_label_set_text(GTK_LABEL(label), buf);
}

void increase_edit(GtkWidget *widget, gpointer entry)
{
	count += 10;

	sprintf(buf, "%d", count);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
}


void decrease(GtkWidget *widget, gpointer label)
{
	count--;

	sprintf(buf, "%d", count);
	gtk_label_set_text(GTK_LABEL(label), buf);
}

void decrease_edit(GtkWidget *widget, gpointer entry)
{
	count -= 10;

	sprintf(buf, "%d", count);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
}



int main(int argc, char** argv) {

	GtkWidget *edit;
	GtkWidget *label;
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *plus;
	GtkWidget *minus;
	GtkWidget *image;
	GtkWidget *grid;
	GtkWidget *inner_grid;

	unsigned x, y;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);
	gtk_window_set_title(GTK_WINDOW(window), "+-");

	frame = gtk_fixed_new();
	/* gtk_container_add(GTK_CONTAINER(window), frame); */
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), grid);

	plus = gtk_button_new_with_label("+");
	gtk_widget_set_size_request(plus, 80, 35);
	gtk_grid_attach(GTK_GRID(grid), plus, 0, 0, 1, 1);

	minus = gtk_button_new_with_label("-");
	gtk_widget_set_size_request(minus, 80, 35);
	gtk_grid_attach(GTK_GRID(grid), minus, 1, 0, 1, 1);

	label = gtk_label_new("0");
	gtk_grid_attach(GTK_GRID(grid), label, 2, 1, 1, 1);

	edit = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(edit), "0");
	gtk_grid_attach(GTK_GRID(grid), edit, 2, 0, 1, 1);

	image = gtk_image_new_from_file ("red_dot.png");
	gtk_grid_attach(GTK_GRID(grid), image, 3, 0, 1, 1);

	inner_grid = gtk_grid_new();
	gtk_grid_attach(GTK_GRID(grid), inner_grid, 3, 3, 10, 10);
	gtk_grid_set_column_spacing(GTK_GRID(inner_grid), 5);

	for (x = 0; x < 5; ++x) {
		image = gtk_image_new_from_file ("red_dot.png");
		gtk_grid_attach(GTK_GRID(inner_grid), image, x, 0, 1, 1);
	}

	gtk_widget_show_all(window);

	g_signal_connect(window, "destroy",
			 G_CALLBACK (gtk_main_quit), NULL);

	g_signal_connect(plus, "clicked",
			 G_CALLBACK(increase), label);

	g_signal_connect(plus, "clicked",
			 G_CALLBACK(increase_edit), edit);

	g_signal_connect(minus, "clicked",
			 G_CALLBACK(decrease), label);

	g_signal_connect(minus, "clicked",
			 G_CALLBACK(decrease_edit), edit);

	g_signal_connect(label, "move-cursor",
			 G_CALLBACK(increase), label);

	gtk_main();

	return 0;
}
