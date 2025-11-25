// =============== 听读模式 ===============

// 播放控制
int listenWordIndex = 0;                 // 当前正在播放的单词索引
int listenPlayCount = 0;                 // 当前单词已经播放了几次（0~3）
unsigned long listenNextActionTime = 0;  // 下一次动作的时间点
const unsigned long listenRepeatInterval = 1200;   // 每次播放之间的间隔（毫秒）
const unsigned long listenNextWordDelay = 600;     // 播完3次后，切到下一个单词前等待的时间

void drawListenWord() {
    canvas.fillSprite(BLACK);
    canvas.setTextDatum(middle_center);

    if (words.empty()) {
        canvas.setTextColor(RED);
        canvas.setTextSize(1.6);
        canvas.drawString("未找到单词数据", canvas.width()/2, canvas.height()/2);
        canvas.pushSprite(0, 0);
        return;
    }

    Word &w = words[listenWordIndex];

    // 显示日语（中间偏上）
    canvas.setTextFont(&fonts::efontJA_16);
    canvas.setTextColor(CYAN);
    drawAutoFitString(canvas, w.jp, canvas.width()/2, canvas.height()/2 - 25,
                    canvas.width() - 20, 2.2, 0.8);  // 自动适配

    // 显示假名（中间）
    if (w.kanji.length() > 0) {
        canvas.setTextColor(ORANGE);
        canvas.setTextSize(1.4);
        canvas.drawString(w.kanji, canvas.width()/2, canvas.height()/2 + 10);
    }

    // 显示中文（中间偏下）
    canvas.setTextFont(&fonts::efontCN_16);
    canvas.setTextColor(YELLOW);
    drawAutoFitString(canvas, w.zh, canvas.width()/2, canvas.height()/2 + 40,
                    canvas.width() - 20, 1.5, 0.8);  // 显示中文释义

    // 模式
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString("听读模式", 50, 15);

    // HUD 显示音量变化
    if (millis() < volumeMessageDeadline && volumeMessageText.length() > 0) {
        canvas.setTextColor(TFT_DARKGREY);
        canvas.setTextSize(1.0);
        canvas.drawString(volumeMessageText, canvas.width() - 15, 15);
    }

    canvas.pushSprite(0, 0);
}

// ---------- 初始化听读模式 ----------
void initListenMode() {
    if (words.empty()) {
        // 理论上应该已经在 StudyMode 中加载过词库
        canvas.fillSprite(BLACK);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(RED);
        canvas.setTextSize(1.6);
        canvas.drawString("请先加载词库", canvas.width()/2, canvas.height()/2);
        canvas.pushSprite(0, 0);
        return;
    }

    listenWordIndex = pickWeightedRandom();
    listenPlayCount = 0;
    listenNextActionTime = 0;   // 立即开始播放

    drawListenWord();
}

// ---------- 听读模式循环逻辑 ----------
void loopListenMode() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        // 键盘状态
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        // 处理 ESC（你在学习模式里用的是 '`' 来代表 ESC）
        for (auto c : st.word) {
            if (c == '`') {  // ESC
                previousMode = appMode; // 记录当前模式
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
        }

        // 处理音量调节（复用学习模式逻辑）
        for (auto c : st.word) {
            if (c == ';') {  // 上：音量+
                soundVolume = min(255, soundVolume + 10);
            } else if (c == '.') {  // 下：音量-
                soundVolume = max(0, soundVolume - 10);
            }

            if (c == ';' || c == '.') {
                M5.Speaker.setVolume(soundVolume);

                // 顺便也可以显示一下 HUD（复用 StudyMode 的那套变量）
                volumeMessageDeadline = millis() + 2000;
                volumeMessageText = String(soundVolume);

                // 重画当前单词（保持 HUD 内容）
                drawListenWord();
            }
        }
    }

    // ------- 自动播放控制 -------
    if (words.empty()) return;

    unsigned long now = millis();

    // 如果现在还没到下一次动作时间，什么也不做
    if (now < listenNextActionTime) {
        return;
    }

    // 如果当前没有在播放任何声音
    if (!M5.Speaker.isPlaying()) {
        if (listenPlayCount < 3) {
            // 第 1~3 次播放
            playAudioForWord(words[listenWordIndex].jp);
            listenPlayCount++;
            listenNextActionTime = now + listenRepeatInterval;
        } else {
            // 已经播了 3 次 → 换下一个单词
            listenWordIndex = pickWeightedRandom();
            listenPlayCount = 0;
            listenNextActionTime = now + listenNextWordDelay;
            drawListenWord();
        }
    }
}
