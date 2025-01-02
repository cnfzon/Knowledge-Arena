#include <locale.h>       // ���ϥ� setlocale
#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>       // srand, rand

#define MAX_QUESTIONS      50
#define MAX_LENGTH         256

#define QUESTIONS_PER_ROUND 5   // �C�^�X�D��
#define PASS_THRESHOLD      3   // �L���ݭn�����T�D��

// === �����ܼ� ===
static GtkWidget* label_timer;
static GtkWidget* label_question;
static GtkWidget* label_score;
static GtkWidget* progress_bar;
static GtkWidget* button1;
static GtkWidget* button2;
static GtkWidget* button3;
static GtkWidget* button4;

// �u�~�� / �h�X�v���s
static GtkWidget* button_continue;
static GtkWidget* button_exit;

// ��ܦ^�X�����D�ơBĹ������
static GtkWidget* label_round;
static GtkWidget* label_wins;

static int time_left = 15;
static int current_question = 0;
static int score = 0;     // �C�^�X�n��
static int total_questions = 0;
static int round_correct = 0;     // ���^�X�����
static int total_wins = 0;     // �`Ĺ���^�X

static gboolean round_end = FALSE; // �O�_�i�J�^�X����

// === �D�ص��c ===
typedef struct {
    char question[MAX_LENGTH];
    char options[4][MAX_LENGTH];
    int  answer; // ���T����(1~4)
} QuizQuestion;

static QuizQuestion questions[MAX_QUESTIONS];

// === �禡�ŧi ===
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

// === Ū���D�w ===
// �榡�ܨ� (UTF-8 �s�X):
// �Ĥ@�D?  
// �ﶵ1
// �ﶵ2
// �ﶵ3
// �ﶵ4
// 3        (���T���� 1~4)
//
// �ĤG�D?
// (�̦�����)
static void load_questions() {
    FILE* file = fopen("formatted_full_questions.txt", "r");
    if (!file) {
        g_print("Error: Failed to open questions file.\n");
        gtk_label_set_text(GTK_LABEL(label_question), "Failed to load questions!");
        return;
    }

    while (total_questions < MAX_QUESTIONS) {
        // Ū�����D (�J�� '?' ����)
        if (fscanf(file, "%255[^?]?%*c", questions[total_questions].question) != 1) {
            break; // �ɮ׵����ή榡����
        }

        // Ū���|�ӿﶵ
        gboolean readOK = TRUE;
        for (int i = 0; i < 4; i++) {
            char tmpLine[MAX_LENGTH] = { 0 };
            if (!fgets(tmpLine, sizeof(tmpLine), file)) {
                readOK = FALSE;
                break;
            }
            // ���������
            tmpLine[strcspn(tmpLine, "\r\n")] = '\0';
            strncpy(questions[total_questions].options[i], tmpLine, MAX_LENGTH - 1);
        }
        if (!readOK) break;

        // Ū�����T����
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

// === �аO���T���ס]�i��\��^ ===
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

// === �H�������D�� ===
static void randomize_questions() {
    srand((unsigned)time(NULL));
    for (int i = 0; i < total_questions; i++) {
        int j = rand() % total_questions;
        QuizQuestion temp = questions[i];
        questions[i] = questions[j];
        questions[j] = temp;
    }
}

// === �w�������ɮ� ===
static void fclose_safe(FILE* file) {
    if (file) {
        fclose(file);
    }
}

// === ��ܤU�@�D ===
static void show_next_question() {
    if (total_questions == 0 || current_question >= total_questions) {
        gtk_label_set_text(GTK_LABEL(label_question), "Quiz Completed! Congratulations!");
        return;
    }

    // ���]�˼Ʈɶ�
    time_left = 15;

    // �D�بϥ� Pango Markup�A����+��j
    gtk_label_set_use_markup(GTK_LABEL(label_question), TRUE);
    char markup[512];
    snprintf(markup, sizeof(markup),
        "<span weight='bold' font='20'>%s</span>",
        questions[current_question].question);
    gtk_label_set_markup(GTK_LABEL(label_question), markup);

    // ��ܥ|�ӿﶵ
    gtk_button_set_label(GTK_BUTTON(button1), questions[current_question].options[0]);
    gtk_button_set_label(GTK_BUTTON(button2), questions[current_question].options[1]);
    gtk_button_set_label(GTK_BUTTON(button3), questions[current_question].options[2]);
    gtk_button_set_label(GTK_BUTTON(button4), questions[current_question].options[3]);
}

// === �ˬd���� ===
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

// === �ˬd�^�X���A (�C 5 �D���@�^�X) ===
static void check_round_status() {
    int questions_answered_in_round = current_question % QUESTIONS_PER_ROUND;

    // �Y��n 5 �D or �D�w�Χ�
    if (questions_answered_in_round == 0 || current_question >= total_questions) {
        // �D�w���� 5 �D�]�⧹�@�^�X
        if (current_question >= total_questions) {
            questions_answered_in_round = QUESTIONS_PER_ROUND;
        }

        round_end = TRUE;

        // ���å|�ӫ��s
        gtk_widget_set_visible(button1, FALSE);
        gtk_widget_set_visible(button2, FALSE);
        gtk_widget_set_visible(button3, FALSE);
        gtk_widget_set_visible(button4, FALSE);

        // �b����ܦ^�X���G
        // �Y round_correct >= PASS_THRESHOLD (3)�A��ܹL��
        if (round_correct >= PASS_THRESHOLD) {
            // ��ܡu���D����GX �D�v
            // (�̻ݨD�i������ܩ� label_question �Υt�� label)
            char msg[64];
            snprintf(msg, sizeof(msg), "���D����G%d �D", round_correct);
            gtk_label_set_text(GTK_LABEL(label_question), msg);

            // �W�[�uĹ�������v
            total_wins++;
            char wins_buf[64];
            snprintf(wins_buf, sizeof(wins_buf), "Ĺ�������G%d", total_wins);
            gtk_label_set_text(GTK_LABEL(label_wins), wins_buf);
        }
        else {
            gtk_label_set_text(GTK_LABEL(label_question), "�������ѡI");
        }

        // �b label_round ��ܥ��^�X�D��
        char round_buf[64];
        snprintf(round_buf, sizeof(round_buf), "���D���^�X����G%d / %d",
            round_correct, QUESTIONS_PER_ROUND);
        gtk_label_set_text(GTK_LABEL(label_round), round_buf);

        // �C�^�X���� -> ���m����
        score = 0;
        gtk_label_set_text(GTK_LABEL(label_score), "Correct Answers: 0");

        // ��ܡu�~�� / �h�X�v���s
        gtk_widget_set_visible(button_continue, TRUE);
        gtk_widget_set_visible(button_exit, TRUE);
    }
}

// === �I���u�~��C���v ===
static void continue_game(void) {
    round_correct = 0;
    round_end = FALSE;

    // ���áu�~�� / �h�X�v���s
    gtk_widget_set_visible(button_continue, FALSE);
    gtk_widget_set_visible(button_exit, FALSE);

    // ��ܥ|�ӫ��s
    gtk_widget_set_visible(button1, TRUE);
    gtk_widget_set_visible(button2, TRUE);
    gtk_widget_set_visible(button3, TRUE);
    gtk_widget_set_visible(button4, TRUE);

    // �Y�D�ة|���Ѿl�A�~��X�D
    if (current_question < total_questions) {
        show_next_question();
    }
    else {
        gtk_label_set_text(GTK_LABEL(label_question), "�Ҧ��D�س������F�I");
    }
}

// === �I���u�h�X�C���v ===
static void exit_game(void) {
    GApplication* app = g_application_get_default();
    if (app) {
        g_application_quit(app);
    }
}

// === ���s�}�l (�Y���ݭn) ===
static void restart_quiz() {
    current_question = 0;
    score = 0;
    round_correct = 0;
    time_left = 15;
    // total_wins ���@�w�n���m�A���ݨD�M�w

    randomize_questions();
    gtk_label_set_text(GTK_LABEL(label_score), "Correct Answers: 0");
    gtk_label_set_text(GTK_LABEL(label_round), "���D���^�X����G0 / 5");
    show_next_question();
}

// === GUI ��l�� ===
static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* box;

    // �إߥD����
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
    // �Y�n����w����ܡA�]�n�T�O UTF-8
    // gtk_label_set_text(GTK_LABEL(label_question), "���J��...");
    gtk_box_append(GTK_BOX(box), label_question);

    // Progress Bar
    progress_bar = gtk_progress_bar_new();
    gtk_box_append(GTK_BOX(box), progress_bar);

    // ��ܥ��^�X�����
    label_round = gtk_label_new("���D���^�X����G0 / 5");
    gtk_box_append(GTK_BOX(box), label_round);

    // ���Ĺ������
    label_wins = gtk_label_new("Ĺ�������G0");
    gtk_box_append(GTK_BOX(box), label_wins);

    // �|�ӿﶵ���s (�����)
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

    // �u�~�� / �h�X�v���s (������)
    button_continue = gtk_button_new_with_label("�~��C��");
    gtk_widget_set_visible(button_continue, FALSE);
    gtk_box_append(GTK_BOX(box), button_continue);
    g_signal_connect(button_continue, "clicked", G_CALLBACK(continue_game), NULL);

    button_exit = gtk_button_new_with_label("�h�X�C��");
    gtk_widget_set_visible(button_exit, FALSE);
    gtk_box_append(GTK_BOX(box), button_exit);
    g_signal_connect(button_exit, "clicked", G_CALLBACK(exit_game), NULL);

    // Ū�D�w
    load_questions();
    show_next_question();

    // �Ұʭp�ɾ� (�C��)
    g_timeout_add_seconds(1, update_timer, NULL);
    g_timeout_add_seconds(1, update_progress, NULL);

    gtk_window_present(GTK_WINDOW(window));
}

// === �D�{���J�f ===
int main(int argc, char** argv) {
    // **���]�w locale** => ������ॿ�T�B�z (�Y�t�Τ䴩 zh_TW.utf8)
    setlocale(LC_ALL, "");

    GtkApplication* app;
    int status;

    app = gtk_application_new("com.example.quizgame", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
