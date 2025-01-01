#include <locale.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define MAX_QUESTIONS       50
#define MAX_LENGTH          256
#define QUESTIONS_PER_ROUND 5     // 每回合題數
// 關主答對題數在 3~5 之間隨機
// 玩家要 >= host_target_correct 才算過關

// === 全域變數 ===
static GtkWidget* label_timer;       // 顯示倒數時間
static GtkWidget* label_question;    // 顯示題目
static GtkWidget* label_round_score; // 顯示「回合分數：玩家 X / 關主 Y」
static GtkWidget* progress_bar;
static GtkWidget* button1;
static GtkWidget* button2;
static GtkWidget* button3;
static GtkWidget* button4;
static GtkWidget* button_continue;
static GtkWidget* button_exit;

static int round_correct_player = 0; // 玩家本回合答對題數
static int host_target_correct = 3; // 關主本回合目標答對題數(3~5隨機)

static int time_left = 15;
static int current_question = 0;    // 目前題目索引
static int total_questions = 0;    // 題庫總題數
static gboolean user_answered = FALSE;

typedef struct {
    char question[MAX_LENGTH];
    char options[4][MAX_LENGTH];
    int  answer;  // 正確答案(1~4)
} QuizQuestion;

static QuizQuestion questions[MAX_QUESTIONS];

// === 函式宣告 ===
static void     apply_css(void);
static void     load_questions(void);
static void     show_next_question(void);
static gboolean update_timer(gpointer user_data);
static gboolean update_progress(gpointer user_data);
static void     randomize_questions(void);
static void     fclose_safe(FILE* file);
static void     check_answer(int chosen);
static void     check_round_status(void);
static void     continue_game(void);
static void     exit_game(void);
static void     start_new_round(void); // 幫助隨機產生關主目標分數

// === 幫助函式：隨機產生關主答對題數(3~5) ===
static void start_new_round() {
    // 重置玩家分數
    round_correct_player = 0;
    // 亂數決定關主目標答對
    // (rand() % 3) 可能是 0,1,2 -> +3 => 3,4,5
    host_target_correct = 3 + (rand() % 3);

    // 更新 UI 分數顯示 => 回合分數：玩家 0 / 關主 host_target_correct
    char score_buf[64];
    snprintf(score_buf, sizeof(score_buf),
        "回合分數：玩家 %d ",
        round_correct_player);
    gtk_label_set_text(GTK_LABEL(label_round_score), score_buf);
}

// === Timer & Progress ===
static gboolean update_timer(gpointer user_data) {
    // 更新倒數顯示
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Time Left: %d", time_left);
    gtk_label_set_text(GTK_LABEL(label_timer), buffer);

    if (time_left > 0) {
        time_left--;
        return G_SOURCE_CONTINUE;
    }
    else {
        // 時間到 => 跳下一題
        gtk_label_set_text(GTK_LABEL(label_timer), "Time's up! Next question...");
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
static void load_questions() {
    FILE* file = fopen("formatted_full_questions.txt", "r");
    if (!file) {
        g_print("Error: Failed to open questions file.\n");
        gtk_label_set_text(GTK_LABEL(label_question), "Failed to load questions!");
        return;
    }

    while (total_questions < MAX_QUESTIONS) {
        // 用 "?%*c" 分割題目
        if (fscanf(file, "%255[^?]?%*c", questions[total_questions].question) != 1) {
            break;  // 讀不到 => EOF
        }

        gboolean ok = TRUE;
        for (int i = 0; i < 4; i++) {
            char tmpLine[MAX_LENGTH] = { 0 };
            if (!fgets(tmpLine, sizeof(tmpLine), file)) {
                ok = FALSE;
                break;
            }
            tmpLine[strcspn(tmpLine, "\r\n")] = '\0';  // 移除換行
            strncpy(questions[total_questions].options[i], tmpLine, MAX_LENGTH - 1);
        }
        if (!ok) break;

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
        randomize_questions();
    }
}

// === 隨機打亂題目 ===
static void randomize_questions() {
    srand((unsigned)time(NULL));
    for (int i = 0; i < total_questions; i++) {
        int j = rand() % total_questions;
        QuizQuestion tmp = questions[i];
        questions[i] = questions[j];
        questions[j] = tmp;
    }
}

static void fclose_safe(FILE* file) {
    if (file) {
        fclose(file);
    }
}

// === 顯示下一題 ===
static void show_next_question() {
    if (total_questions == 0 || current_question >= total_questions) {
        gtk_label_set_text(GTK_LABEL(label_question), "No more questions!");
        check_round_status();
        return;
    }

    // 重置 => 玩家未答、time_left=15
    user_answered = FALSE;
    time_left = 15;

    // 顯示題目: (x/5) ...
    int in_round_index = (current_question % QUESTIONS_PER_ROUND) + 1;
    char markup[512];
    snprintf(markup, sizeof(markup),
        "<span weight='bold' font='24'>(%d/%d) %s</span>",
        in_round_index, QUESTIONS_PER_ROUND,
        questions[current_question].question);

    gtk_label_set_use_markup(GTK_LABEL(label_question), TRUE);
    gtk_label_set_markup(GTK_LABEL(label_question), markup);

    // 更新選項按鈕
    gtk_button_set_label(GTK_BUTTON(button1), questions[current_question].options[0]);
    gtk_button_set_label(GTK_BUTTON(button2), questions[current_question].options[1]);
    gtk_button_set_label(GTK_BUTTON(button3), questions[current_question].options[2]);
    gtk_button_set_label(GTK_BUTTON(button4), questions[current_question].options[3]);
}

// === 玩家檢查答案 ===
static void check_answer(int chosen) {
    if (current_question >= total_questions) return;
    if (user_answered) return; // 已答

    user_answered = TRUE;

    // 玩家是否答對
    if (chosen == questions[current_question].answer) {
        round_correct_player++;
    }

    // 更新回合分數 => 「回合分數：玩家 X / 關主 Y」
    char score_buf[64];
    snprintf(score_buf, sizeof(score_buf),
        "回合分數：玩家 %d ",
        round_correct_player);
    gtk_label_set_text(GTK_LABEL(label_round_score), score_buf);

    current_question++;
    show_next_question();
    check_round_status();
}

// === 檢查回合狀態(5題) ===
static void check_round_status() {
    int answered_in_round = current_question % QUESTIONS_PER_ROUND;

    // 回合結束 or 題庫沒題
    if (answered_in_round == 0 || current_question >= total_questions) {
        // 隱藏四個選項按鈕
        gtk_widget_set_visible(button1, FALSE);
        gtk_widget_set_visible(button2, FALSE);
        gtk_widget_set_visible(button3, FALSE);
        gtk_widget_set_visible(button4, FALSE);

        // 判斷玩家是否 >= host_target_correct
        if (round_correct_player >= host_target_correct) {
            char msg[64];
            snprintf(msg, sizeof(msg),
                "回合結束：玩家過關！（玩家：%d / 關主：%d）",
                round_correct_player, host_target_correct);
            gtk_label_set_text(GTK_LABEL(label_question), msg);
        }
        else {
            char msg[64];
            snprintf(msg, sizeof(msg),
                "回合結束：闖關失敗！（玩家：%d / 關主：%d）",
                round_correct_player, host_target_correct);
            gtk_label_set_text(GTK_LABEL(label_question), msg);
        }

        // 顯示「繼續 / 退出」按鈕
        gtk_widget_set_visible(button_continue, TRUE);
        gtk_widget_set_visible(button_exit, TRUE);
    }
}

// === 按「繼續遊戲」 => 進入下一回合 ===
static void continue_game(void) {
    // 開始一個新回合 => 重新亂數決定 host_target_correct，玩家分數歸零
    start_new_round();

    // 隱藏「繼續 / 退出」
    gtk_widget_set_visible(button_continue, FALSE);
    gtk_widget_set_visible(button_exit, FALSE);

    // 顯示四個選項按鈕
    gtk_widget_set_visible(button1, TRUE);
    gtk_widget_set_visible(button2, TRUE);
    gtk_widget_set_visible(button3, TRUE);
    gtk_widget_set_visible(button4, TRUE);

    // 若題庫還有題 => 繼續
    if (current_question < total_questions) {
        show_next_question();
    }
    else {
        gtk_label_set_text(GTK_LABEL(label_question),
            "所有題目都完成了！");
    }
}

// === 退出遊戲 ===
static void exit_game(void) {
    GApplication* app = g_application_get_default();
    if (app) {
        g_application_quit(app);
    }
}

// === 套用簡易 CSS ===
static void apply_css(void) {
    static const char* css_data =
        "window {"
        "    background-color: #f0f0f0;"
        "}"
        "label {"
        "    color: #333333;"
        "    font-size: 16px;"
        "}"
        "button {"
        "    color: #222222;"
        "    background-color: #ddd;"
        "    font-size: 16px;"
        "    padding: 6px 12px;"
        "}";

    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, css_data);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);
}

// === GUI 初始化 ===
static void activate(GtkApplication* app, gpointer user_data) {
    // 套用 CSS
    apply_css();

    // 主視窗
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Quiz App - Random Host Score [3~5]");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // 最上層: Time Left & Progress
    label_timer = gtk_label_new("Time Left: 15");
    gtk_box_append(GTK_BOX(vbox), label_timer);

    progress_bar = gtk_progress_bar_new();
    gtk_box_append(GTK_BOX(vbox), progress_bar);

    // 題目
    label_question = gtk_label_new("Loading...");
    gtk_label_set_justify(GTK_LABEL(label_question), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(vbox), label_question);

    // 回合分數 =>「玩家 X / 關主 Y」
    label_round_score = gtk_label_new("回合分數：玩家 0 / 關主 3");
    gtk_box_append(GTK_BOX(vbox), label_round_score);

    // 選項按鈕 (水平排)
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(vbox), button_box);

    button1 = gtk_button_new_with_label("Option 1");
    gtk_box_append(GTK_BOX(button_box), button1);
    g_signal_connect_swapped(button1, "clicked",
        G_CALLBACK(check_answer), GINT_TO_POINTER(1));

    button2 = gtk_button_new_with_label("Option 2");
    gtk_box_append(GTK_BOX(button_box), button2);
    g_signal_connect_swapped(button2, "clicked",
        G_CALLBACK(check_answer), GINT_TO_POINTER(2));

    button3 = gtk_button_new_with_label("Option 3");
    gtk_box_append(GTK_BOX(button_box), button3);
    g_signal_connect_swapped(button3, "clicked",
        G_CALLBACK(check_answer), GINT_TO_POINTER(3));

    button4 = gtk_button_new_with_label("Option 4");
    gtk_box_append(GTK_BOX(button_box), button4);
    g_signal_connect_swapped(button4, "clicked",
        G_CALLBACK(check_answer), GINT_TO_POINTER(4));

    // 「繼續遊戲 / 退出遊戲」
    button_continue = gtk_button_new_with_label("繼續遊戲");
    gtk_widget_set_visible(button_continue, FALSE);
    gtk_box_append(GTK_BOX(vbox), button_continue);
    g_signal_connect(button_continue, "clicked", G_CALLBACK(continue_game), NULL);

    button_exit = gtk_button_new_with_label("退出遊戲");
    gtk_widget_set_visible(button_exit, FALSE);
    gtk_box_append(GTK_BOX(vbox), button_exit);
    g_signal_connect(button_exit, "clicked", G_CALLBACK(exit_game), NULL);

    // 載入題庫
    load_questions();

    // **第一回合** => 先設定亂數的 host_target_correct
    start_new_round();

    // 顯示第一題
    show_next_question();

    // 啟動計時器 & 進度條 (每秒)
    g_timeout_add_seconds(1, update_timer, NULL);
    g_timeout_add_seconds(1, update_progress, NULL);

    gtk_window_present(GTK_WINDOW(window));
}

// === 主程式入口 ===
int main(int argc, char** argv) {
    setlocale(LC_ALL, "");

    GtkApplication* app;
    app = gtk_application_new("com.example.quizgame", G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
