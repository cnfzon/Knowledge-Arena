
#include <gtk/gtk.h>

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* box;
    GtkWidget* label;
    GtkWidget* button1, * button2, * button3, * button4;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "show");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_window_set_child(GTK_WINDOW(window), box);

    label = gtk_label_new("question");
    gtk_box_append(GTK_BOX(box), label);

    button1 = gtk_button_new_with_label("A");
    gtk_box_append(GTK_BOX(box), button1);

    button2 = gtk_button_new_with_label("B");
    gtk_box_append(GTK_BOX(box), button2);

    button3 = gtk_button_new_with_label("C");
    gtk_box_append(GTK_BOX(box), button3);

    button4 = gtk_button_new_with_label("D");
    gtk_box_append(GTK_BOX(box), button4);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char** argv) {
    GtkApplication* app;
    int status;

    app = gtk_application_new("com.example.quiz", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
