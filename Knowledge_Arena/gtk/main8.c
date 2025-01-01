#include <locale.h>       // 為使用 setlocale
#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>       // srand, rand

#define MAX_QUESTIONS      50
#define MAX_LENGTH         256

#define QUESTIONS_PER_ROUND 5   // 每回合題數
#define PASS_THRESHOLD      3   // 過關需要的正確題數

// === 全域變數 ===
static GtkWidget* label_timer;
static GtkWidget* label_question;
static GtkWidget* label_score;
static GtkWidget* progress_bar;
static GtkWidget* button1;
static GtkWidget* button2;
static GtkWidget* button3;
static GtkWidget* button4;

// 「繼續 / 退出」按鈕
static GtkWidget* button_continue;
static GtkWidget* button_exit;

// 顯示回合答對題數、贏的場次
static GtkWidget* label_round;
static GtkWidget* label_wins;

static int time_left = 15;
static int current_question = 0;
static int score = 0;     // 每回合積分
static int total_questions = 0;
static int round_correct = 0;     // 本回合答對數
static int total_wins = 0;     // 總贏的回合

static gboolean round_end = FALSE; // 是否進入回合結束

// === 題目結構 ===
typedef struct {
    char question[MAX_LENGTH];
    char options[4][MAX_LENGTH];
    int  answer; // 正確答案(1~4)
} QuizQuestion;

static QuizQuestion questions[MAX_QUESTIONS];

// === 函式宣告 ===
static void     load_questions(void);
static void     show_next_question(void);
static gboolean update_timer(gpointer user_data);
static gboolean update_progress(gpointer user_data);
static void     randomize_questions(void);
static void     fclose_safe(FILE* file);
static void     mark_correct_answers(void);
static void     restart_quiz(void);
static void     check_answer(int chosen);
static void     check_round_status(void);
static void     continue_game(void);
static void     exit_game(void);

// === Timer & Progress ===
static gboolean update_timer(gpointer user_data) {
    char buffer[64];
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

// === 讀取題庫 ===
// 格式示例 (UTF-8 編碼):
// 第一題?  
// 選項1
// 選項2
// 選項3
// 選項4
// 3        (正確答案 1~4)
//
// 第二題?
// (依此類推)
static void load_questions() {
    FILE* file = fopen("formatted_full_questions.txt", "r");
    if (!file) {
        g_print("Error: Failed to open questions file.\n");
        gtk_label_set_text(GTK_LABEL(label_question), "Failed to load questions!");
        return;
    }

    while (total_questions < MAX_QUESTIONS) {
        // 讀取問題 (遇到 '?' 為止)
        if (fscanf(file, "%255[^?]?%*c", questions[total_questions].question) != 1) {
            break; // 檔案結束或格式不符
        }

        // 讀取四個選項
        gboolean readOK = TRUE;
        for (int i = 0; i < 4; i++) {
            char tmpLine[MAX_LENGTH] = { 0 };
            if (!fgets(tmpLine, sizeof(tmpLine), file)) {
                readOK = FALSE;
                break;
            }
            // 移除換行符
            tmpLine[strcspn(tmpLine, "\r\n")] = '\0';
            strncpy(questions[total_questions].options[i], tmpLine, MAX_LENGTH - 1);
        }
        if (!readOK) break;

        // 讀取正確答案
        int ans;
        if (fscanf(file, "%d\n", &ans) != 1) {
            break;
        }
        questions[total_questions].answer = ans;

        total_questions++;
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

// === 標記正確答案（可選功能） ===
static void mark_correct_answers() {
    for (int i = 0; i < total_questions; i++) {
        int correct_index = questions[i].answer - 1;
        const char* note = " (Correct Answer)";
        if (strlen(questions[i].options[correct_index]) + strlen(note) < MAX_LENGTH - 1) {
            strncat(questions[i].options[correct_index], note,
                (MAX_LENGTH - 1) - strlen(questions[i].options[correct_index]));
        }
    }
}

// === 隨機打亂題目 ===
static void randomize_questions() {
    srand((unsigned)time(NULL));
    for (int i = 0; i < total_questions; i++) {
        int j = rand() % total_questions;
        QuizQuestion temp = questions[i];
        questions[i] = questions[j];
        questions[j] = temp;
    }
}

// === 安全關閉檔案 ===
static void fclose_safe(FILE* file) {
    if (file) {
        fclose(file);
    }
}

// === 顯示下一題 ===
static void show_next_question() {
    if (total_questions == 0 || current_question >= total_questions) {
        gtk_label_set_text(GTK_LABEL(label_question), "Quiz Completed! Congratulations!");
        return;
    }

    // 重設倒數時間
    time_left = 15;

    // 題目使用 Pango Markup，粗體+放大
    gtk_label_set_use_markup(GTK_LABEL(label_question), TRUE);
    char markup[512];
    snprintf(markup, sizeof(markup),
        "<span weight='bold' font='20'>%s</span>",
        questions[current_question].question);
    gtk_label_set_markup(GTK_LABEL(label_question), markup);

    // 顯示四個選項
    gtk_button_set_label(GTK_BUTTON(button1), questions[current_question].options[0]);
    gtk_button_set_label(GTK_BUTTON(button2), questions[current_question].options[1]);
    gtk_button_set_label(GTK_BUTTON(button3), questions[current_question].options[2]);
    gtk_button_set_label(GTK_BUTTON(button4), questions[current_question].options[3]);
}

// === 檢查答案 ===
static void check_answer(int chosen) {
    if (total_questions == 0 || current_question >= total_questions) {
        return;
    }

    if (chosen == questions[current_question].answer) {
        score++;
        round_correct++;

        char buf[64];
        snprintf(buf, sizeof(buf), "Correct Answers: %d", score);
        gtk_label_set_text(GTK_LABEL(label_score), buf);
    }

    current_question++;
    show_next_question();
    check_round_status();
}

// === 檢查回合狀態 (每 5 題為一回合) ===
static void check_round_status() {
    int questions_answered_in_round = current_question % QUESTIONS_PER_ROUND;

    // 若剛好 5 題 or 題庫用完
    if (questions_answered_in_round == 0 || current_question >= total_questions) {
        // 題庫不足 5 題也算完一回合
        if (current_question >= total_questions) {
            questions_answered_in_round = QUESTIONS_PER_ROUND;
        }

        round_end = TRUE;

        // 隱藏四個按鈕
        gtk_widget_set_visible(button1, FALSE);
        gtk_widget_set_visible(button2, FALSE);
        gtk_widget_set_visible(button3, FALSE);
        gtk_widget_set_visible(button4, FALSE);

        // 在此顯示回合結果
        // 若 round_correct >= PASS_THRESHOLD (3)，表示過關
        if (round_correct >= PASS_THRESHOLD) {
            // 顯示「關主答對：X 題」
            // (依需求可直接顯示於 label_question 或另做 label)
            char msg[64];
            snprintf(msg, sizeof(msg), "關主答對：%d 題", round_correct);
            gtk_label_set_text(GTK_LABEL(label_question), msg);

            // 增加「贏的場次」
            total_wins++;
            char wins_buf[64];
            snprintf(wins_buf, sizeof(wins_buf), "贏的場次：%d", total_wins);
            gtk_label_set_text(GTK_LABEL(label_wins), wins_buf);
        }
        else {
            gtk_label_set_text(GTK_LABEL(label_question), "闖關失敗！");
        }

        // 在 label_round 顯示本回合題數
        char round_buf[64];
        snprintf(round_buf, sizeof(round_buf), "關主本回合答對：%d / %d",
            round_correct, QUESTIONS_PER_ROUND);
        gtk_label_set_text(GTK_LABEL(label_round), round_buf);

        // 每回合結束 -> 重置分數
        score = 0;
        gtk_label_set_text(GTK_LABEL(label_score), "Correct Answers: 0");

        // 顯示「繼續 / 退出」按鈕
        gtk_widget_set_visible(button_continue, TRUE);
        gtk_widget_set_visible(button_exit, TRUE);
    }
}

// === 點擊「繼續遊戲」 ===
static void continue_game(void) {
    round_correct = 0;
    round_end = FALSE;

    // 隱藏「繼續 / 退出」按鈕
    gtk_widget_set_visible(button_continue, FALSE);
    gtk_widget_set_visible(button_exit, FALSE);

    // 顯示四個按鈕
    gtk_widget_set_visible(button1, TRUE);
    gtk_widget_set_visible(button2, TRUE);
    gtk_widget_set_visible(button3, TRUE);
    gtk_widget_set_visible(button4, TRUE);

    // 若題目尚有剩餘，繼續出題
    if (current_question < total_questions) {
        show_next_question();
    }
    else {
        gtk_label_set_text(GTK_LABEL(label_question), "所有題目都答完了！");
    }
}

// === 點擊「退出遊戲」 ===
static void exit_game(void) {
    GApplication* app = g_application_get_default();
    if (app) {
        g_application_quit(app);
    }
}

// === 重新開始 (若有需要) ===
static void restart_quiz() {
    current_question = 0;
    score = 0;
    round_correct = 0;
    time_left = 15;
    // total_wins 不一定要重置，視需求決定

    randomize_questions();
    gtk_label_set_text(GTK_LABEL(label_score), "Correct Answers: 0");
    gtk_label_set_text(GTK_LABEL(label_round), "關主本回合答對：0 / 5");
    show_next_question();
}

// === GUI 初始化 ===
static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* box;

    // 建立主視窗
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Quiz App (UTF-8)");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_window_set_child(GTK_WINDOW(window), box);

    // Timer Label
    label_timer = gtk_label_new("Time Left: 15");
    gtk_box_append(GTK_BOX(box), label_timer);

    // Score Label
    label_score = gtk_label_new("Correct Answers: 0");
    gtk_box_append(GTK_BOX(box), label_score);

    // Question Label
    label_question = gtk_label_new("");
    // 若要中文預先顯示，也要確保 UTF-8
    // gtk_label_set_text(GTK_LABEL(label_question), "載入中...");
    gtk_box_append(GTK_BOX(box), label_question);

    // Progress Bar
    progress_bar = gtk_progress_bar_new();
    gtk_box_append(GTK_BOX(box), progress_bar);

    // 顯示本回合答對數
    label_round = gtk_label_new("關主本回合答對：0 / 5");
    gtk_box_append(GTK_BOX(box), label_round);

    // 顯示贏的場次
    label_wins = gtk_label_new("贏的場次：0");
    gtk_box_append(GTK_BOX(box), label_wins);

    // 四個選項按鈕 (左對齊)
    button1 = gtk_button_new_with_label("1");
    g_signal_connect_swapped(button1, "clicked", G_CALLBACK(check_answer), GINT_TO_POINTER(1));
    gtk_widget_set_halign(button1, GTK_ALIGN_START);

    button2 = gtk_button_new_with_label("2");
    g_signal_connect_swapped(button2, "clicked", G_CALLBACK(check_answer), GINT_TO_POINTER(2));
    gtk_widget_set_halign(button2, GTK_ALIGN_START);

    button3 = gtk_button_new_with_label("3");
    g_signal_connect_swapped(button3, "clicked", G_CALLBACK(check_answer), GINT_TO_POINTER(3));
    gtk_widget_set_halign(button3, GTK_ALIGN_START);

    button4 = gtk_button_new_with_label("4");
    g_signal_connect_swapped(button4, "clicked", G_CALLBACK(check_answer), GINT_TO_POINTER(4));
    gtk_widget_set_halign(button4, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(box), button1);
    gtk_box_append(GTK_BOX(box), button2);
    gtk_box_append(GTK_BOX(box), button3);
    gtk_box_append(GTK_BOX(box), button4);

    // 「繼續 / 退出」按鈕 (先隱藏)
    button_continue = gtk_button_new_with_label("繼續遊戲");
    gtk_widget_set_visible(button_continue, FALSE);
    gtk_box_append(GTK_BOX(box), button_continue);
    g_signal_connect(button_continue, "clicked", G_CALLBACK(continue_game), NULL);

    button_exit = gtk_button_new_with_label("退出遊戲");
    gtk_widget_set_visible(button_exit, FALSE);
    gtk_box_append(GTK_BOX(box), button_exit);
    g_signal_connect(button_exit, "clicked", G_CALLBACK(exit_game), NULL);

    // 讀題庫
    load_questions();
    show_next_question();

    // 啟動計時器 (每秒)
    g_timeout_add_seconds(1, update_timer, NULL);
    g_timeout_add_seconds(1, update_progress, NULL);

    gtk_window_present(GTK_WINDOW(window));
}

// === 主程式入口 ===
int main(int argc, char** argv) {
    // **先設定 locale** => 讓中文能正確處理 (若系統支援 zh_TW.utf8)
    setlocale(LC_ALL, "");

    GtkApplication* app;
    int status;

    app = gtk_application_new("com.example.quizgame", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
