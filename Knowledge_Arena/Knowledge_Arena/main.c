#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <conio.h> // 用於非阻塞式輸入（_kbhit 和 _getch）
#include <time.h>  // 用於隨機數種子初始化

#define TOTAL_QUESTIONS 150  // 題目總數
#define SELECTED_QUESTIONS 5 // 每次選擇的題目數量
#define MAX_OPTIONS 4        // 每題最多選項數量
#define TIME_LIMIT 15        // 每題回答時間限制（秒）

typedef struct {
    char question[256];
    char options[MAX_OPTIONS][100];
    int correctOption;
} Question;

void loadQuestionsFromTxt(const char* filename, Question* questions, int* totalQuestions);
void selectRandomQuestions(Question* allQuestions, int totalQuestions, Question* selectedQuestions, int selectedCount);
void playGame(Question* questions, int numQuestions);

int main() {
    srand((unsigned int)time(NULL)); // 初始化隨機數種子

    Question* allQuestions = malloc(TOTAL_QUESTIONS * sizeof(Question)); // 分配所有題目的記憶體
    if (allQuestions == NULL) {
        perror("記憶體分配失敗");
        return 1;
    }

    Question selectedQuestions[SELECTED_QUESTIONS]; // 存儲隨機選出的題目
    int totalQuestions = 0;

    printf("歡迎來到知識王遊戲！\n");
    loadQuestionsFromTxt("questions.txt", allQuestions, &totalQuestions);

    if (totalQuestions <= 0) {
        printf("題目載入失敗或檔案為空。\n");
        free(allQuestions);
        return 1;
    }

    int continueGame = 1; // 繼續遊戲標誌
    while (continueGame) {
        printf("\n準備開始遊戲！\n");
        selectRandomQuestions(allQuestions, totalQuestions, selectedQuestions, SELECTED_QUESTIONS);
        playGame(selectedQuestions, SELECTED_QUESTIONS); // 使用隨機選出的題目

        // 提供繼續遊戲或離開遊戲的選項
        int choice = 0;
        while (choice != 1 && choice != 2) {
            printf("\n遊戲結束！請選擇：\n");
            printf("1. 繼續遊戲\n");
            printf("2. 離開遊戲\n");
            printf("請輸入您的選擇 (1-2): ");
            scanf("%d", &choice);
            if (choice == 1) {
                continueGame = 1; // 繼續遊戲
            }
            else if (choice == 2) {
                continueGame = 0; // 離開遊戲
                printf("感謝遊玩！再見！\n");
            }
            else {
                printf("輸入無效，請重新輸入。\n");
            }
        }
    }

    free(allQuestions); // 釋放記憶體
    return 0;
}

// 題目載入函數
void loadQuestionsFromTxt(const char* filename, Question* questions, int* totalQuestions) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("無法開啟題目檔案");
        return;
    }

    char line[512];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (count >= TOTAL_QUESTIONS) {
            printf("已達到最大題目數量上限 %d。\n", TOTAL_QUESTIONS);
            break;
        }

        if (line[0] == '\n') continue; // 跳過空行

        strncpy_s(questions[count].question, sizeof(questions[count].question), line, _TRUNCATE);
        questions[count].question[strcspn(questions[count].question, "\n")] = '\0'; // 移除換行符

        if (fgets(line, sizeof(line), file) == NULL) break;

        char* token;
        char* context = NULL; // 用於 strtok_s 的上下文
        token = strtok_s(line, ";", &context);

        int optionIndex = 0;
        while (token != NULL && optionIndex < MAX_OPTIONS) {
            strncpy_s(questions[count].options[optionIndex], sizeof(questions[count].options[optionIndex]), token, _TRUNCATE);
            questions[count].options[optionIndex][strcspn(questions[count].options[optionIndex], "\n")] = '\0'; // 移除換行符
            token = strtok_s(NULL, ";", &context);
            optionIndex++;
        }

        if (fgets(line, sizeof(line), file) == NULL) break;
        questions[count].correctOption = atoi(line) - 1; // 正確答案索引
        count++;
    }

    fclose(file);
    *totalQuestions = count;
}

// 隨機選擇題目函數
void selectRandomQuestions(Question* allQuestions, int totalQuestions, Question* selectedQuestions, int selectedCount) {
    int selectedIndices[SELECTED_QUESTIONS] = { 0 }; // 確保初始化
    int isChosen;

    for (int i = 0; i < selectedCount; i++) {
        do {
            isChosen = 0;
            selectedIndices[i] = rand() % totalQuestions; // 隨機選擇一個索引
            for (int j = 0; j < i; j++) {
                if (selectedIndices[i] == selectedIndices[j]) {
                    isChosen = 1; // 發現重複索引
                    break;
                }
            }
        } while (isChosen); // 如果已選過則重新選擇

        selectedQuestions[i] = allQuestions[selectedIndices[i]];
    }
}

// 遊戲邏輯函數
void playGame(Question* questions, int numQuestions) {
    int userScore = 0;
    int bossScore = 0;

    for (int i = 0; i < numQuestions; i++) {
        printf("\n題目 %d: %s\n", i + 1, questions[i].question);
        for (int j = 0; j < MAX_OPTIONS; j++) {
            printf("%d. %s\n", j + 1, questions[i].options[j]);
        }

        int userAnswer = -1; // 使用者答案
        int bossAnswer = (rand() % 100 < 60) ? questions[i].correctOption : rand() % MAX_OPTIONS; // 關主答案，正確率 >60%
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
                    userAnswer = input - '0' - 1; // 轉換為 0 基準索引
                    break;
                }
            }
        }

        if (timeLeft == 0) {
            printf("\n時間到！視為放棄作答！\n");
        }

        // 判斷使用者和關主的答案
        if (userAnswer == questions[i].correctOption) {
            userScore++;
        }

        if (bossAnswer == questions[i].correctOption) {
            bossScore++;
        }

        printf("\n正確答案是 %s\n", questions[i].options[questions[i].correctOption]);
        printf("目前比分 - 使用者: %d, 關主: %d\n", userScore, bossScore);
    }

    printf("\n遊戲結束！\n");
    if (userScore > bossScore) {
        printf("恭喜！你贏了！最終比分 - 使用者: %d, 關主: %d\n", userScore, bossScore);
    }
    else if (userScore < bossScore) {
        printf("很遺憾，關主勝利。最終比分 - 使用者: %d, 關主: %d\n", userScore, bossScore);
    }
    else {
        printf("平局！最終比分 - 使用者: %d, 關主: %d\n", userScore, bossScore);
    }
}