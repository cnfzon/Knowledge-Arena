#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <conio.h> // 用於非阻塞式輸入（_kbhit 和 _getch）

#define INITIAL_QUESTIONS 100 // 初始題目數量
#define MAX_OPTIONS 4         // 每題最多選項數量
#define TIME_LIMIT 15         // 每題回答時間限制（秒）

typedef struct {
    char question[256];
    char options[MAX_OPTIONS][100];
    int correctOption;
} Question;

void loadQuestionsFromTxt(const char* filename, Question** questions, int* numQuestions, int* maxQuestions);
void playGame(Question* questions, int numQuestions);

int main() {
    int maxQuestions = INITIAL_QUESTIONS;
    Question* questions = malloc(maxQuestions * sizeof(Question)); // 初始分配空間
    if (questions == NULL) {
        perror("記憶體分配失敗");
        return 1;
    }

    int numQuestions = 0;

    printf("歡迎來到知識王遊戲！\n");
    loadQuestionsFromTxt("questions.txt", &questions, &numQuestions, &maxQuestions);

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
void loadQuestionsFromTxt(const char* filename, Question** questions, int* numQuestions, int* maxQuestions) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("無法開啟題目檔案");
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (*numQuestions >= *maxQuestions) {
            // 動態擴充空間
            *maxQuestions *= 2; // 擴充為原來的 2 倍
            Question* temp = realloc(*questions, (*maxQuestions) * sizeof(Question));
            if (temp == NULL) {
                perror("記憶體擴充失敗");
                free(*questions);
                fclose(file);
                exit(1);
            }
            *questions = temp;
        }

        if (line[0] == '\n') continue; // 跳過空行

        strncpy((*questions)[*numQuestions].question, line, sizeof((*questions)[*numQuestions].question));
        (*questions)[*numQuestions].question[strcspn((*questions)[*numQuestions].question, "\n")] = '\0'; // 移除換行符

        if (fgets(line, sizeof(line), file) == NULL) break;
        char* token = strtok(line, ";");
        int optionIndex = 0;
        while (token != NULL && optionIndex < MAX_OPTIONS) {
            strncpy((*questions)[*numQuestions].options[optionIndex], token, sizeof((*questions)[*numQuestions].options[optionIndex]));
            (*questions)[*numQuestions].options[optionIndex][strcspn((*questions)[*numQuestions].options[optionIndex], "\n")] = '\0'; // 移除換行符
            token = strtok(NULL, ";");
            optionIndex++;
        }

        if (fgets(line, sizeof(line), file) == NULL) break;
        (*questions)[*numQuestions].correctOption = atoi(line) - 1;
        (*numQuestions)++;
    }

    fclose(file);
}

// 遊戲邏輯函數
void playGame(Question* questions, int numQuestions) {
    int score = 0;

    for (int i = 0; i < numQuestions; i++) {
        printf("\n題目 %d: %s\n", i + 1, questions[i].question);
        for (int j = 0; j < MAX_OPTIONS; j++) {
            printf("%d. %s\n", j + 1, questions[i].options[j]);
        }

        int answer = -1; // 預設答案為無效值
        int timeLeft = TIME_LIMIT;

        printf("請輸入答案 (1-%d) [限時 %d 秒]: \n", MAX_OPTIONS, TIME_LIMIT);

        // 倒數計時邏輯
        while (timeLeft > 0) {
            printf("\r剩餘時間: %d 秒", timeLeft);
            fflush(stdout); // 強制刷新輸出

            Sleep(1000); // 等待 1 秒
            timeLeft--;

            if (_kbhit()) { // 檢查是否有鍵盤輸入
                char input = _getch(); // 獲取輸入字元
                if (input >= '1' && input <= '4') { // 判斷是否為 1-4
                    answer = input - '0'; // 轉換為整數
                    break;
                }
                else {
                    printf("\n輸入錯誤！\n");
                    answer = -1; // 無效輸入
                    break;
                }
            }
        }

        if (timeLeft == 0) {
            printf("\n時間到！視為放棄作答！\n");
        }
        else if (answer - 1 == questions[i].correctOption) {
            printf("正確！\n");
            score++;
        }
        else {
            printf("錯誤！正確答案是 %s\n", questions[i].options[questions[i].correctOption]);
        }
    }

    printf("\n遊戲結束！你的總分是 %d/%d\n", score, numQuestions);
}
