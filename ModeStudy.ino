/**
 * StudyMode.ino - 闪卡学习模式
 *
 * 实现类似 Anki 的双面闪卡学习功能。支持日语和英语两种语言的卡片显示，
 * 卡片正面/反面随机切换（Side A: 外语优先；Side B: 中文优先）。
 * 用户可通过键盘控制翻卡、调节音量、播放发音、以及调整熟练度评分。
 */

// 学习模式公共变量
bool showMeaning = false;      // 是否显示释义（翻卡状态）
bool showAnkiSideA = true;     // true=先显示外语（Side A），false=先显示中文（Side B）

/**
 * 绘制当前闪卡的完整画面
 *
 * 清屏后根据 currentLanguage 调用对应的语言绘制函数渲染卡片内容，
 * 左上角显示当前单词的熟练度评分，右上角在音量调节后短暂显示音量值。
 * 若词库为空则显示错误提示。
 */
void drawStudyWord()
{
    canvas.fillSprite(BLACK);
    canvas.setTextDatum(middle_center);

    if (words.empty())
    {
        drawCenterString(canvas, "未找到单词数据", RED, 1.2);
        return;
    }

    Word &w = words[wordIndex];

    if (currentLanguage == LANG_EN)
    {
        drawEnglishWord(w);
    }
    else if (currentLanguage == LANG_JP)
    {
        drawJapaneseWord(w);
    }

    // 熟练度提示
    drawTopLeftString(canvas, "Score: " + String(words[wordIndex].score), TFT_DARKGREY, 1.0);

    // HUD 显示音量变化
    if (millis() < volumeMessageDeadline)
    {
        drawTopRightString(canvas, String(soundVolume), TFT_DARKGREY, 1.0);
    }

    canvas.pushSprite(0, 0);
}


/**
 * 绘制英语单词闪卡
 *
 * Side A（showAnkiSideA=true）：主显英语单词和音标，翻卡后显示中文释义。
 * Side B（showAnkiSideA=false）：主显中文释义和词性，翻卡后显示英语原文。
 * 文字大小自动适配屏幕宽度，颜色编码区分不同信息层级。
 *
 * @param w 要显示的单词引用
 */
void drawEnglishWord(Word &w)
{
    if (showAnkiSideA)
    {
        // === 模式1：显示英语,隐藏中文 ===
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w.en, canvas.width() / 2, canvas.height() / 2 - 25, 2.2);

        canvas.setTextColor(GREEN);
        canvas.setTextSize(1.3);
        {
            String phon = asciiPhonetic(w.phonetic);
            canvas.drawString(phon, canvas.width() / 2, canvas.height() / 2 + 5);
        }

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(YELLOW);
            drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 + 40, 1.5);
        }
    }
    else 
    {
        // === 模式2：显示中文,隐藏英语 ===
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(YELLOW);
        drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 - 25, 2.0); // 显示中文释义主行

        if (w.pos.length() > 0)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.4);
            canvas.drawString(w.pos, canvas.width() / 2, canvas.height() / 2 + 5);
        }

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(CYAN);
            drawAutoFitString(canvas, w.en, canvas.width() / 2, canvas.height() / 2 + 40, 1.8); // 显示英语原文
        }
    }
}

/**
 * 绘制日语单词闪卡
 *
 * Side A（showAnkiSideA=true）：主显日语假名和声调，翻卡后显示中文释义。
 * Side B（showAnkiSideA=false）：主显中文释义和汉字，翻卡后显示日语假名。
 * 文字大小自动适配屏幕宽度，颜色编码区分不同信息层级。
 *
 * @param w 要显示的单词引用
 */
void drawJapaneseWord(Word &w)
{
    if (showAnkiSideA)
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
}

/**
 * 学习模式主循环 - 处理键盘输入
 *
 * 支持以下操作：
 * - BtnA（物理按钮）：切换释义显示（翻卡）
 * - ` (ESC)：打开 ESC 菜单
 * - ; / . ：增大/减小音量
 * - Fn：播放当前单词发音
 * - Enter：标记为"记住"，提升熟练度并跳到下一个单词
 * - Del：标记为"不熟"，降低熟练度并跳到下一个单词
 *
 * Enter/Del 操作后会随机切换 Side A/B 并重新抽词。
 */
void loopStudyMode()
{
    // BtnA键 → 切换释义
    if (M5Cardputer.BtnA.wasPressed())
    {
        showMeaning = !showMeaning;
        drawStudyWord();
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
            else if (c == ';' || c == '.')
            {
                if (adjustVolume(c)) {
                    drawStudyWord();
                }
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
            markScoreDirty();
        }
        // Del = 不熟,降低熟练度
        else if (status.del)
        {
            words[wordIndex].score = max(1, words[wordIndex].score - 1);
            markScoreDirty();
        }
        if (status.enter || status.del)
        {
            wordIndex = pickWeightedRandom();
            showMeaning = false;
            showAnkiSideA = random(2);
            drawStudyWord();
        }
    }
}
