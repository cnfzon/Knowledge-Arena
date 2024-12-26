#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_QUESTIONS 100
#define MAX_OPTIONS 4

typedef struct {
    char question[256];
    char options[MAX_OPTIONS][100];
    int correctOption;
} Question;

void loadQuestionsFromTxt(const char* filename, Question questions[], int* numQuestions);
void playGame(Question questions[], int numQuestions);

int main() {
    Question* questions = malloc(MAX_QUESTIONS * sizeof(Question));
    if (questions == NULL) {
        perror("�O������t����");
        return 1;
    }

    int numQuestions = 0;

    printf("�w��Ө쪾�Ѥ��C���I\n");
    loadQuestionsFromTxt("questions.txt", questions, &numQuestions);

    if (numQuestions > 0) {
        printf("���\���J %d �D�I�ǳƶ}�l�C���I\n", numQuestions);
        playGame(questions, numQuestions); // ����C���޿�
    }
    else {
        printf("�D�ظ��J���ѩ��ɮ׬��šC\n");
    }

    free(questions); // ����O����
    printf("�P�¹C���I\n");
    return 0;
}

// �D�ظ��J���
void loadQuestionsFromTxt(const char* filename, Question questions[], int* numQuestions) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("�L�k�}���D���ɮ�");
        return;
    }

    char line[512];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (count >= MAX_QUESTIONS) {
            printf("�w�F��̤j�D�ؼƶq�W�� %d�C\n", MAX_QUESTIONS);
            break;
        }

        if (line[0] == '\n') continue; // ���L�Ŧ�
        strncpy(questions[count].question, line, sizeof(questions[count].question));
        questions[count].question[strcspn(questions[count].question, "\n")] = '\0'; // ���������

        if (fgets(line, sizeof(line), file) == NULL) break;
        char* token = strtok(line, ";");
        int optionIndex = 0;
        while (token != NULL && optionIndex < MAX_OPTIONS) {
            strncpy(questions[count].options[optionIndex], token, sizeof(questions[count].options[optionIndex]));
            token = strtok(NULL, ";");
            optionIndex++;
        }

        if (fgets(line, sizeof(line), file) == NULL) break;
        questions[count].correctOption = atoi(line) - 1;
        count++;
    }

    fclose(file);
    *numQuestions = count;
}

void playGame(Question questions[], int numQuestions) {
    int score = 0;
    for (int i = 0; i < numQuestions; i++) {
        printf("\n�D�� %d: %s\n", i + 1, questions[i].question);
        for (int j = 0; j < MAX_OPTIONS; j++) {
            printf("%d. %s\n", j + 1, questions[i].options[j]);
        }

        int answer;
        printf("�п�J���� (1-%d): ", MAX_OPTIONS);
        scanf("%d", &answer);

        if (answer - 1 == questions[i].correctOption) {
            printf("���T�I\n");
            score++;
        }
        else {
            printf("���~�I���T���׬O %s\n", questions[i].options[questions[i].correctOption]);
        }
    }
    printf("\n�C�������I�A���`���O %d/%d\n", score, numQuestions);
}