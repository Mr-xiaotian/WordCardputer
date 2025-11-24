// =============== 听写模式 ===============

std::vector<int> dictOrder;     // 随机顺序
int dictPos = 0;                // 当前是第几个单词
String userInput = "";          // 用户正在输入的字符串

int correctCount = 0;
int wrongCount = 0;

bool dictShowSummary = false;

// ---------- 初始化听写模式 ----------
void initDictationMode() {
    dictOrder.clear();
    for (int i = 0; i < words.size(); i++) dictOrder.push_back(i);

    // 随机顺序
    for (int i = 0; i < dictOrder.size(); i++) {
        int j = random(dictOrder.size());
        std::swap(dictOrder[i], dictOrder[j]);
    }

    dictPos = 0;
    userInput = "";
    correctCount = 0;
    wrongCount = 0;
    dictShowSummary = false;

    playAudioForWord(words[dictOrder[dictPos]].jp);
    drawDictationInput();
}

// ---------- 绘制答题中的画面 ----------
void drawDictationInput() {
    canvas.fillSprite(BLACK);

    canvas.setTextDatum(top_left);
    canvas.setTextColor(CYAN);
    canvas.setTextSize(1.2);

    canvas.drawString("听写模式", 8, 8);

    // 显示用户输入
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(2.0);
    canvas.setTextColor(WHITE);
    canvas.drawString(userInput, canvas.width() / 2, canvas.height() / 2);

    // 进度提示
    canvas.setTextDatum(bottom_center);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString(
        String(dictPos + 1) + "/" + String(dictOrder.size()),
        canvas.width() / 2,
        canvas.height() - 10
    );

    canvas.pushSprite(0, 0);
}

// ---------- 绘制总结界面 ----------
void drawDictationSummary() {
    canvas.fillSprite(BLACK);

    canvas.setTextDatum(middle_center);
    canvas.setTextSize(2.0);

    canvas.setTextColor(GREEN);
    canvas.drawString("完成！", canvas.width()/2, canvas.height()/2 - 40);

    canvas.setTextColor(WHITE);
    canvas.setTextSize(1.6);
    canvas.drawString("正确: " + String(correctCount), canvas.width()/2, canvas.height()/2);
    canvas.drawString("错误: " + String(wrongCount), canvas.width()/2, canvas.height()/2 + 40);

    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString("按 Enter 返回菜单", canvas.width()/2, canvas.height() - 20);

    canvas.pushSprite(0, 0);
}

// ---------- 听写模式逻辑 ----------
void loopDictationMode() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        // -------- 打字阶段 --------
        auto st = M5Cardputer.Keyboard.keysState();

        // -------- 显示总结界面 --------
        if (dictShowSummary) {
            if (st.enter) {
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
            }
            return;
        }

        // 输入字符
        for (auto c : st.word) {
            if (c >= 32 && c <= 126) {   // 可显示字符
                userInput += c;
                drawDictationInput();
            }
        }

        // 删除
        if (st.del && userInput.length() > 0) {
            userInput.remove(userInput.length() - 1);
            drawDictationInput();
        }

        // 按 ENTER 检查答案
        if (st.enter) {
            String correct = words[dictOrder[dictPos]].jp;

            if (userInput == correct) correctCount++;
            else wrongCount++;

            dictPos++;

            if (dictPos >= dictOrder.size()) {
                dictShowSummary = true;
                drawDictationSummary();
                return;
            }

            // 进入下一个单词
            userInput = "";
            playAudioForWord(words[dictOrder[dictPos]].jp);
            drawDictationInput();
        }

        // ESC 返回上级菜单
        for (auto c : st.word) {
            if (c == 27) { // ESC
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
        }
    }
}
