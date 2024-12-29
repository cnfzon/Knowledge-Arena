#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <conio.h> // �Ω�D���릡��J�]_kbhit �M _getch�^
#include <time.h>  // �Ω��H���ƺؤl��l��

#define TOTAL_QUESTIONS 150  // �D���`��
#define SELECTED_QUESTIONS 5 // �C����ܪ��D�ؼƶq
#define MAX_OPTIONS 4        // �C�D�̦h�ﶵ�ƶq
#define TIME_LIMIT 15        // �C�D�^���ɶ�����]��^

typedef struct {
    char question[256];
    char options[MAX_OPTIONS][100];
    int correctOption;
} Question;

void loadQuestionsFromTxt(const char* filename, Question* questions, int* totalQuestions);
void selectRandomQuestions(Question* allQuestions, int totalQuestions, Question* selectedQuestions, int selectedCount);
void playGame(Question* questions, int numQuestions);

int main() {
    srand((unsigned int)time(NULL)); // ��l���H���ƺؤl

    Question* allQuestions = malloc(TOTAL_QUESTIONS * sizeof(Question)); // ���t�Ҧ��D�ت��O����
    if (allQuestions == NULL) {
        perror("�O������t����");
        return 1;
    }

    Question selectedQuestions[SELECTED_QUESTIONS]; // �s�x�H����X���D��
    int totalQuestions = 0;

    printf("�w��Ө쪾�Ѥ��C���I\n");
    loadQuestionsFromTxt("questions.txt", allQuestions, &totalQuestions);

    if (totalQuestions <= 0) {
        printf("�D�ظ��J���ѩ��ɮ׬��šC\n");
        free(allQuestions);
        return 1;
    }

    int continueGame = 1; // �~��C���лx
    while (continueGame) {
        printf("\n�ǳƶ}�l�C���I\n");
        selectRandomQuestions(allQuestions, totalQuestions, selectedQuestions, SELECTED_QUESTIONS);
        playGame(selectedQuestions, SELECTED_QUESTIONS); // �ϥ��H����X���D��

        // �����~��C�������}�C�����ﶵ
        int choice = 0;
        while (choice != 1 && choice != 2) {
            printf("\n�C�������I�п�ܡG\n");
            printf("1. �~��C��\n");
            printf("2. ���}�C��\n");
            printf("�п�J�z����� (1-2): ");
            scanf("%d", &choice);
            if (choice == 1) {
                continueGame = 1; // �~��C��
            }
            else if (choice == 2) {
                continueGame = 0; // ���}�C��
                printf("�P�¹C���I�A���I\n");
            }
            else {
                printf("��J�L�ġA�Э��s��J�C\n");
            }
        }
    }

    free(allQuestions); // ����O����
    return 0;
}

// �D�ظ��J���
void loadQuestionsFromTxt(const char* filename, Question* questions, int* totalQuestions) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("�L�k�}���D���ɮ�");
        return;
    }

    char line[512];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (count >= TOTAL_QUESTIONS) {
            printf("�w�F��̤j�D�ؼƶq�W�� %d�C\n", TOTAL_QUESTIONS);
            break;
        }

        if (line[0] == '\n') continue; // ���L�Ŧ�

        strncpy_s(questions[count].question, sizeof(questions[count].question), line, _TRUNCATE);
        questions[count].question[strcspn(questions[count].question, "\n")] = '\0'; // ���������

        if (fgets(line, sizeof(line), file) == NULL) break;

        char* token;
        char* context = NULL; // �Ω� strtok_s ���W�U��
        token = strtok_s(line, ";", &context);

        int optionIndex = 0;
        while (token != NULL && optionIndex < MAX_OPTIONS) {
            strncpy_s(questions[count].options[optionIndex], sizeof(questions[count].options[optionIndex]), token, _TRUNCATE);
            questions[count].options[optionIndex][strcspn(questions[count].options[optionIndex], "\n")] = '\0'; // ���������
            token = strtok_s(NULL, ";", &context);
            optionIndex++;
        }

        if (fgets(line, sizeof(line), file) == NULL) break;
        questions[count].correctOption = atoi(line) - 1; // ���T���ׯ���
        count++;
    }

    fclose(file);
    *totalQuestions = count;
}

// �H������D�ب��
void selectRandomQuestions(Question* allQuestions, int totalQuestions, Question* selectedQuestions, int selectedCount) {
    int selectedIndices[SELECTED_QUESTIONS] = { 0 }; // �T�O��l��
    int isChosen;

    for (int i = 0; i < selectedCount; i++) {
        do {
            isChosen = 0;
            selectedIndices[i] = rand() % totalQuestions; // �H����ܤ@�ӯ���
            for (int j = 0; j < i; j++) {
                if (selectedIndices[i] == selectedIndices[j]) {
                    isChosen = 1; // �o�{���Ư���
                    break;
                }
            }
        } while (isChosen); // �p�G�w��L�h���s���

        selectedQuestions[i] = allQuestions[selectedIndices[i]];
    }
}

// �C���޿���
void playGame(Question* questions, int numQuestions) {
    int userScore = 0;
    int bossScore = 0;

    for (int i = 0; i < numQuestions; i++) {
        printf("\n�D�� %d: %s\n", i + 1, questions[i].question);
        for (int j = 0; j < MAX_OPTIONS; j++) {
            printf("%d. %s\n", j + 1, questions[i].options[j]);
        }

        int userAnswer = -1; // �ϥΪ̵���
        int bossAnswer = (rand() % 100 < 60) ? questions[i].correctOption : rand() % MAX_OPTIONS; // ���D���סA���T�v >60%
        int timeLeft = TIME_LIMIT;

        printf("�п�J���� (1-%d) [���� %d ��]: \n", MAX_OPTIONS, TIME_LIMIT);

        // �˼ƭp���޿�
        while (timeLeft > 0) {
            printf("\r�Ѿl�ɶ�: %d ��", timeLeft);
            fflush(stdout); // �j���s��X

            Sleep(1000); // ���� 1 ��
            timeLeft--;

            if (_kbhit()) { // �ˬd�O�_����L��J
                char input = _getch(); // �����J�r��
                if (input >= '1' && input <= '4') { // �P�_�O�_�� 1-4
                    userAnswer = input - '0' - 1; // �ഫ�� 0 ��ǯ���
                    break;
                }
            }
        }

        if (timeLeft == 0) {
            printf("\n�ɶ���I�������@���I\n");
        }

        // �P�_�ϥΪ̩M���D������
        if (userAnswer == questions[i].correctOption) {
            userScore++;
        }

        if (bossAnswer == questions[i].correctOption) {
            bossScore++;
        }

        printf("\n���T���׬O %s\n", questions[i].options[questions[i].correctOption]);
        printf("�ثe��� - �ϥΪ�: %d, ���D: %d\n", userScore, bossScore);
    }

    printf("\n�C�������I\n");
    if (userScore > bossScore) {
        printf("���ߡI�AĹ�F�I�̲פ�� - �ϥΪ�: %d, ���D: %d\n", userScore, bossScore);
    }
    else if (userScore < bossScore) {
        printf("�ܿ�ѡA���D�ӧQ�C�̲פ�� - �ϥΪ�: %d, ���D: %d\n", userScore, bossScore);
    }
    else {
        printf("�����I�̲פ�� - �ϥΪ�: %d, ���D: %d\n", userScore, bossScore);
    }
}