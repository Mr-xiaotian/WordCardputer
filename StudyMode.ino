// 学习模式公共变量
bool showMeaning = false;
bool showJPFirst = true; // true=先显示日语, false=先显示中文

void drawWord()
{
    canvas.fillSprite(BLACK);
    canvas.setTextDatum(middle_center);

    if (words.empty())
    {
        canvas.setTextColor(RED);
        canvas.drawString("未找到单词数据", canvas.width() / 2, canvas.height() / 2);
        canvas.pushSprite(0, 0);
        return;
    }

    Word &w = words[wordIndex];

    if (currentLanguage == LANG_EN)
    {
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w.en, canvas.width() / 2, canvas.height() / 2 - 25, 2.2);

        String subLine = w.phonetic;
        if (w.pos.length() > 0)
        {
            if (subLine.length() > 0)
                subLine += "  " + w.pos;
            else
                subLine = w.pos;
        }
        if (subLine.length() > 0)
        {
            canvas.setTextColor(GREEN);
            canvas.setTextSize(1.1);
            canvas.drawString(subLine, canvas.width() / 2, canvas.height() / 2 + 5);
        }

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(YELLOW);
            drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 + 40, 1.5);
        }
    }
    else if (showJPFirst)
    {
        // === 模式1：显示日语,隐藏中文 ===
        canvas.setTextFont(&fonts::efontJA_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w.jp, canvas.width() / 2, canvas.height() / 2 - 25, 2.2); // 自动适配

        canvas.setTextColor(GREEN);
        canvas.setTextSize(1.3);
        canvas.drawString("Tone: " + String(w.tone), canvas.width() / 2, canvas.height() / 2 + 5);

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(YELLOW);
            drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 + 40, 1.5); // 显示中文释义
        }
    }
    else
    {
        // === 模式2：显示中文,隐藏日语 ===
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(YELLOW);
        drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 - 25, 2.0); // 显示中文释义主行

        if (w.kanji.length() > 0)
        {
            canvas.setTextFont(&fonts::efontJA_16);
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.4);
            canvas.drawString(w.kanji, canvas.width() / 2, canvas.height() / 2 + 5);
        }

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontJA_16);
            canvas.setTextColor(CYAN);
            drawAutoFitString(canvas, w.jp, canvas.width() / 2, canvas.height() / 2 + 40, 1.8); // 显示日语原文
        }
    }

    // 熟练度提示
    drawTopLeftString(canvas, "Score: " + String(words[wordIndex].score));

    // HUD 显示音量变化
    if (millis() < volumeMessageDeadline)
    {
        drawTopRightString(canvas, String(soundVolume));
    }

    canvas.pushSprite(0, 0);
}

void startStudyMode(const String &filePath)
{
    bool ok = loadWordsFromJSON(filePath);
    if (!ok || words.empty())
    {
        M5Cardputer.Display.fillScreen(BLACK);
        canvas.fillSprite(BLACK);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(RED);
        canvas.drawString("词库加载失败", canvas.width() / 2, canvas.height() / 2);
        canvas.pushSprite(0, 0);
        return;
    }

    wordIndex = pickWeightedRandom();
    showMeaning = false;
    if (currentLanguage == LANG_EN)
        showJPFirst = true;
    drawWord();
}

void loopStudyMode()
{
    // BtnA键 → 切换释义
    if (M5Cardputer.BtnA.wasPressed())
    {
        showMeaning = !showMeaning;
        drawWord();
        userAction = true;
    }

    // 键盘操作
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        userAction = true;

        // 检测 esc 键
        for (auto c : status.word)
        {
            if (c == '`')
            {                           // ESC 键
                previousMode = appMode; // 记录当前模式
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
            else if (c == ';')
            { // 上
                soundVolume = min(255, soundVolume + 10);
            }
            else if (c == '.')
            { // 下
                soundVolume = max(0, soundVolume - 10);
            }

            if (c == ';' || c == '.')
            {
                M5.Speaker.setVolume(soundVolume);

                volumeMessageDeadline = millis() + 2000;
                drawWord();
            }
        }

        // fn = 播放音频
        if (status.fn)
        {
            if (currentLanguage == LANG_JP)
            {
                playAudioForWord(words[wordIndex].jp);
            }
            else if (words[wordIndex].en.length() > 0)
            {
                playAudioForWord(words[wordIndex].en);
            }
        }

        // Enter = 记住,提升熟练度
        if (status.enter)
        {
            words[wordIndex].score = min(5, words[wordIndex].score + 1);
        }
        // Del = 不熟,降低熟练度
        else if (status.del)
        {
            words[wordIndex].score = max(1, words[wordIndex].score - 1);
        }
        if (status.enter || status.del)
        {
            wordIndex = pickWeightedRandom();
            showMeaning = false;
            if (currentLanguage == LANG_JP)
                showJPFirst = random(2);
            else
                showJPFirst = true;
            drawWord();
        }
    }
}
