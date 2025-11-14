// =============== 听写模式 ===============

M5Canvas dictCanvas(&M5Cardputer.Display);

std::vector<int> dictOrder;     // 随机顺序
int dictPos = 0;                // 当前是第几个单词
String userInput = "";          // 用户正在输入的字符串

int correctCount = 0;
int wrongCount = 0;

bool dictShowSummary = false;

// ---------- 初始化听写模式 ----------
void initDictationMode() {
    dictCanvas.createSprite(
        M5Cardputer.Display.width(),
        M5Cardputer.Display.height()
    );
    dictCanvas.setTextFont(&fonts::efontCN_16);

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
    dictCanvas.fillSprite(BLACK);

    dictCanvas.setTextDatum(top_left);
    dictCanvas.setTextColor(CYAN);
    dictCanvas.setTextSize(1.2);

    dictCanvas.drawString("听写模式", 8, 8);

    // 显示用户输入
    dictCanvas.setTextDatum(middle_center);
    dictCanvas.setTextSize(2.0);
    dictCanvas.setTextColor(WHITE);
    dictCanvas.drawString(userInput, dictCanvas.width() / 2, dictCanvas.height() / 2);

    // 进度提示
    dictCanvas.setTextDatum(bottom_center);
    dictCanvas.setTextColor(TFT_DARKGREY);
    dictCanvas.setTextSize(1.0);
    dictCanvas.drawString(
        String(dictPos + 1) + "/" + String(dictOrder.size()),
        dictCanvas.width() / 2,
        dictCanvas.height() - 10
    );

    dictCanvas.pushSprite(0, 0);
}

// ---------- 绘制总结界面 ----------
void drawDictationSummary() {
    dictCanvas.fillSprite(BLACK);

    dictCanvas.setTextDatum(middle_center);
    dictCanvas.setTextSize(2.0);

    dictCanvas.setTextColor(GREEN);
    dictCanvas.drawString("完成！", dictCanvas.width()/2, dictCanvas.height()/2 - 40);

    dictCanvas.setTextColor(WHITE);
    dictCanvas.setTextSize(1.6);
    dictCanvas.drawString("正确: " + String(correctCount), dictCanvas.width()/2, dictCanvas.height()/2);
    dictCanvas.drawString("错误: " + String(wrongCount), dictCanvas.width()/2, dictCanvas.height()/2 + 40);

    dictCanvas.setTextColor(TFT_DARKGREY);
    dictCanvas.setTextSize(1.0);
    dictCanvas.drawString("按 Enter 返回菜单", dictCanvas.width()/2, dictCanvas.height() - 20);

    dictCanvas.pushSprite(0, 0);
}

// ---------- 听写模式逻辑 ----------
void loopDictationMode() {
    M5Cardputer.update();

    // -------- 显示总结界面 --------
    if (dictShowSummary) {
        auto st = M5Cardputer.Keyboard.keysState();
        if (st.enter) {
            appMode = MODE_ESC_MENU;
            initEscMenuMode();
        }
        return;
    }

    // -------- 打字阶段 --------
    auto st = M5Cardputer.Keyboard.keysState();

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
