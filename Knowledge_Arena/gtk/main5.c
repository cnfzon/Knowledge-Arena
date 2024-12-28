#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#define MAX_QUESTIONS 50
#define MAX_LENGTH 256

static GtkWidget* label_timer;
static GtkWidget* label_question;
static GtkWidget* label_score;
static GtkWidget* button1, * button2, * button3, * button4;
static int time_left = 15;
static int current_question = 0;
static int score = 0;

typedef struct {
    char question[MAX_LENGTH];
    char options[4][MAX_LENGTH];
    int answer;
} QuizQuestion;

static QuizQuestion questions[MAX_QUESTIONS];
static int total_questions = 0;

// 提前宣告函式
static void show_next_question(void);

static gboolean update_timer(gpointer user_data) {
    char buffer[32];
    g_snprintf(buffer, sizeof(buffer), "\u6642\u9593\u5012\u6578: %d", time_left);
    gtk_label_set_text(GTK_LABEL(label_timer), buffer);

    if (time_left > 0) {
        time_left--;
        return G_SOURCE_CONTINUE;
    }
    else {
        gtk_label_set_text(GTK_LABEL(label_timer), "\u6642\u9593\u5012\u6578: 0");
        gtk_label_set_text(GTK_LABEL(label_question), "Time's up! Moving to next question.");
        current_question++;
        show_next_question();
        return G_SOURCE_REMOVE;
    }
}

static void load_questions() {
    FILE* file = fopen("formatted_full_questions.txt", "r");
    if (!file) {
        g_print("Failed to open questions file\n");
        gtk_label_set_text(GTK_LABEL(label_question), "Failed to load questions.");
        return;
    }

    while (fscanf(file, "%255[^?]?%*c", questions[total_questions].question) != EOF) {
        for (int i = 0; i < 4; i++) {
            if (fscanf(file, "%255[^\n]\n", questions[total_questions].options[i]) == EOF) {
                fclose(file);
                return;
            }
        }
        if (fscanf(file, "Answer: %d\n", &questions[total_questions].answer) == EOF) {
            fclose(file);
            return;
        }
        total_questions++;
        if (total_questions >= MAX_QUESTIONS) break;
    }
    fclose(file);

    if (total_questions == 0) {
        gtk_label_set_text(GTK_LABEL(label_question), "No questions available.");
    }
}

static void show_next_question() {
    if (total_questions == 0 || current_question >= total_questions) {
        gtk_label_set_text(GTK_LABEL(label_question), "No questions loaded or Quiz Completed!\n");
        return;
    }

    gtk_label_set_text(GTK_LABEL(label_question), questions[current_question].question);
    gtk_button_set_label(GTK_BUTTON(button1), questions[current_question].options[0]);
    gtk_button_set_label(GTK_BUTTON(button2), questions[current_question].options[1]);
    gtk_button_set_label(GTK_BUTTON(button3), questions[current_question].options[2]);
    gtk_button_set_label(GTK_BUTTON(button4), questions[current_question].options[3]);

    time_left = 15; // Reset timer for next question
}

static void check_answer(int chosen) {
    if (total_questions == 0 || current_question >= total_questions) return;

    if (chosen == questions[current_question].answer) {
        score++;
        char score_buffer[32];
        g_snprintf(score_buffer, sizeof(score_buffer), "\u7b54\u5c0d\u984c\u6578: %d", score);
        gtk_label_set_text(GTK_LABEL(label_score), score_buffer);
    }
    current_question++;
    show_next_question();
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* box;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Quiz App");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_window_set_child(GTK_WINDOW(window), box);

    label_timer = gtk_label_new("\u6642\u9593\u5012\u6578: 15");
    gtk_box_append(GTK_BOX(box), label_timer);

    label_score = gtk_label_new("\u7b54\u5c0d\u984c\u6578: 0");
    gtk_box_append(GTK_BOX(box), label_score);

    label_question = gtk_label_new("Loading Questions...");
    gtk_box_append(GTK_BOX(box), label_question);

    button1 = gtk_button_new_with_label("1");
    g_signal_connect_swapped(button1, "clicked", G_CALLBACK(check_answer), GINT_TO_POINTER(1));

    button2 = gtk_button_new_with_label("2");
    g_signal_connect_swapped(button2, "clicked", G_CALLBACK(check_answer), GINT_TO_POINTER(2));

    button3 = gtk_button_new_with_label("3");
    g_signal_connect_swapped(button3, "clicked", G_CALLBACK(check_answer), GINT_TO_POINTER(3));

    button4 = gtk_button_new_with_label("4");
    g_signal_connect_swapped(button4, "clicked", G_CALLBACK(check_answer), GINT_TO_POINTER(4));

    gtk_box_append(GTK_BOX(box), button1);
    gtk_box_append(GTK_BOX(box), button2);
    gtk_box_append(GTK_BOX(box), button3);
    gtk_box_append(GTK_BOX(box), button4);

    load_questions();
    show_next_question();

    g_timeout_add_seconds(1, update_timer, NULL);
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