#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_QUESTIONS 50
#define MAX_LENGTH 256

static GtkWidget* label_timer;
static GtkWidget* label_question;
static GtkWidget* label_score;
static GtkWidget* progress_bar;
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

// Function declarations
static void show_next_question(void);
static gboolean update_timer(gpointer user_data);
static gboolean update_progress(gpointer user_data);
static void randomize_questions();
static void fclose_safe(FILE* file);
static void mark_correct_answers();
static void restart_quiz();

static gboolean update_timer(gpointer user_data) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Time Left: %d", time_left);
    gtk_label_set_text(GTK_LABEL(label_timer), buffer);

    if (time_left > 0) {
        time_left--;
        return G_SOURCE_CONTINUE;
    }
    else {
        gtk_label_set_text(GTK_LABEL(label_timer), "Time's up! Moving to next question...");
        current_question++;
        show_next_question();
        return G_SOURCE_REMOVE;
    }
}

static gboolean update_progress(gpointer user_data) {
    double fraction = (double)time_left / 15.0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), fraction);
    return G_SOURCE_CONTINUE;
}

// 讀取題庫檔案
static void load_questions() {
    FILE* file = fopen("formatted_full_questions.txt", "r");
    if (!file) {
        g_print("Error: Failed to open questions file.\n");
        gtk_label_set_text(GTK_LABEL(label_question), "Failed to load questions!");
        return;
    }

    while (fscanf(file, "%255[^?]?%*c", questions[total_questions].question) != EOF) {
        for (int i = 0; i < 4; i++) {
            if (fscanf(file, "%255[^\n]\n", questions[total_questions].options[i]) == EOF) {
                fclose_safe(file);
                g_print("Error parsing options for question %d\n", total_questions + 1);
                return;
            }
        }
        if (fscanf(file, "%d\n", &questions[total_questions].answer) == EOF) {
            fclose_safe(file);
            g_print("Error parsing answer for question %d\n", total_questions + 1);
            return;
        }
        total_questions++;
        if (total_questions >= MAX_QUESTIONS) break;
    }

    fclose_safe(file);

    if (total_questions == 0) {
        gtk_label_set_text(GTK_LABEL(label_question), "No questions available!");
    }
    else {
        mark_correct_answers();
        randomize_questions();
    }
}

// 標記正確答案
static void mark_correct_answers() {
    for (int i = 0; i < total_questions; i++) {
        int correct_index = questions[i].answer - 1;  // 將答案轉換為索引
        strcat(questions[i].options[correct_index], " (Correct Answer)");
    }
}

// 隨機打亂題目順序
static void randomize_questions() {
    srand((unsigned)time(NULL));
    for (int i = 0; i < total_questions; i++) {
        int j = rand() % total_questions;
        QuizQuestion temp = questions[i];
        questions[i] = questions[j];
        questions[j] = temp;
    }
}

// 安全關閉檔案
static void fclose_safe(FILE* file) {
    if (file != NULL) {
        fclose(file);
    }
}

// 顯示下一題，標記選項
static void show_next_question() {
    if (total_questions == 0 || current_question >= total_questions) {
        gtk_label_set_text(GTK_LABEL(label_question), "Quiz Completed! Congratulations!");
        return;
    }

    gtk_label_set_text(GTK_LABEL(label_question), questions[current_question].question);
    gtk_button_set_label(GTK_BUTTON(button1), questions[current_question].options[0]);
    gtk_button_set_label(GTK_BUTTON(button2), questions[current_question].options[1]);
    gtk_button_set_label(GTK_BUTTON(button3), questions[current_question].options[2]);
    gtk_button_set_label(GTK_BUTTON(button4), questions[current_question].options[3]);

    time_left = 15; // 重設時間
}

static void check_answer(int chosen) {
    if (total_questions == 0 || current_question >= total_questions) return;

    if (chosen == questions[current_question].answer) {
        score++;
        char score_buffer[32];
        snprintf(score_buffer, sizeof(score_buffer), "Correct Answers: %d", score);
        gtk_label_set_text(GTK_LABEL(label_score), score_buffer);
    }
    current_question++;
    show_next_question();
}

static void restart_quiz() {
    current_question = 0;
    score = 0;
    time_left = 15;
    randomize_questions();
    gtk_label_set_text(GTK_LABEL(label_score), "Correct Answers: 0");
    show_next_question();
}

// GUI 初始化
static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;  // 使用正確的變數名稱
    GtkWidget* box;

    window = gtk_application_window_new(app);  // 正確初始化窗口
    gtk_window_set_title(GTK_WINDOW(window), "Quiz App");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_window_set_child(GTK_WINDOW(window), box);

    // 其他元件初始化
    label_timer = gtk_label_new("Time Left: 15");
    gtk_box_append(GTK_BOX(box), label_timer);

    label_score = gtk_label_new("Correct Answers: 0");
    gtk_box_append(GTK_BOX(box), label_score);

    label_question = gtk_label_new("Loading questions...");
    gtk_box_append(GTK_BOX(box), label_question);

    progress_bar = gtk_progress_bar_new();
    gtk_box_append(GTK_BOX(box), progress_bar);

    // 問題選項按鈕
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
    g_timeout_add_seconds(1, update_progress, NULL);
    gtk_window_present(GTK_WINDOW(window));
}

// 主程式入口
int main(int argc, char** argv) {
    GtkApplication* app;
    int status;

    app = gtk_application_new("com.example.quizgame", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
