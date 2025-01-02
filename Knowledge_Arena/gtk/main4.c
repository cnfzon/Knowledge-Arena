#include <gtk/gtk.h>

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* box;
    GtkWidget* label_timer;
    GtkWidget* label_score;
    GtkWidget* label_question;
    GtkWidget* button1, * button2, * button3, * button4;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Quiz App");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);

    // 主垂直容器
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_window_set_child(GTK_WINDOW(window), box);

    // 時間倒數
    label_timer = gtk_label_new("\u6642\u9593\u5012\u6578");
    gtk_box_append(GTK_BOX(box), label_timer);

    // 答對題數
    label_score = gtk_label_new("\u7b54\u5c0d\u984c\u6578: 0");
    gtk_box_append(GTK_BOX(box), label_score);

    // 問題區域
    label_question = gtk_label_new("question");
    gtk_box_append(GTK_BOX(box), label_question);

    // 選項按鈕
    button1 = gtk_button_new_with_label("1");
    button2 = gtk_button_new_with_label("2");
    button3 = gtk_button_new_with_label("3");
    button4 = gtk_button_new_with_label("4");

    gtk_box_append(GTK_BOX(box), button1);
    gtk_box_append(GTK_BOX(box), button2);
    gtk_box_append(GTK_BOX(box), button3);
    gtk_box_append(GTK_BOX(box), button4);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char** argv) {
    GtkApplication* app;
    int status;

    app = gtk_application_new("com.example.quizgame", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}