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
        perror("記憶體分配失敗");
        return 1;
    }

    int numQuestions = 0;

    printf("歡迎來到知識王遊戲！\n");
    loadQuestionsFromTxt("questions.txt", questions, &numQuestions);

    if (numQuestions > 0) {
        printf("成功載入 %d 題！準備開始遊戲！\n", numQuestions);
        playGame(questions, numQuestions); // 執行遊戲邏輯
    }
    else {
        printf("題目載入失敗或檔案為空。\n");
    }

    free(questions); // 釋放記憶體
    printf("感謝遊玩！\n");
    return 0;
}

// 題目載入函數
void loadQuestionsFromTxt(const char* filename, Question questions[], int* numQuestions) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("無法開啟題目檔案");
        return;
    }

    char line[512];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (count >= MAX_QUESTIONS) {
            printf("已達到最大題目數量上限 %d。\n", MAX_QUESTIONS);
            break;
        }

        if (line[0] == '\n') continue; // 跳過空行
        strncpy(questions[count].question, line, sizeof(questions[count].question));
        questions[count].question[strcspn(questions[count].question, "\n")] = '\0'; // 移除換行符

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
        printf("\n題目 %d: %s\n", i + 1, questions[i].question);
        for (int j = 0; j < MAX_OPTIONS; j++) {
            printf("%d. %s\n", j + 1, questions[i].options[j]);
        }

        int answer;
        printf("請輸入答案 (1-%d): ", MAX_OPTIONS);
        scanf("%d", &answer);

        if (answer - 1 == questions[i].correctOption) {
            printf("正確！\n");
            score++;
        }
        else {
            printf("錯誤！正確答案是 %s\n", questions[i].options[questions[i].correctOption]);
        }
    }
    printf("\n遊戲結束！你的總分是 %d/%d\n", score, numQuestions);
}