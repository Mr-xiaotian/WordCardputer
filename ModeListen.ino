/**
 * @file ModeListen.ino
 * @brief 听读模式
 *
 * 实现单词自动循环播放功能。每个单词自动播放 3 次语音，
 * 播放完毕后自动切换到下一个单词（基于权重随机选取）。
 * 同时在屏幕上显示单词文本、注音和中文释义，支持音量调节。
 */

// =============== 听读模式 ===============

// 播放控制
int listenPlayCount = 0;                         // 当前单词已经播放了几次（0~3）
unsigned long listenNextActionTime = 0;          // 下一次动作的时间点
const unsigned long listenRepeatInterval = 1200; // 每次播放之间的间隔（毫秒）
const unsigned long listenNextWordDelay = 600;   // 播完3次后,切到下一个单词前等待的时间

/**
 * 获取当前单词用于语音播放的文本
 *
 * 根据当前语言设置返回对应的音频文本。
 * 英语模式返回英文单词，日语模式返回日语假名。
 *
 * @param w 单词对象
 * @return 用于语音播放的文本字符串
 */
String listenAudioText(const Word &w)
{
    if (currentLanguage == LANG_EN)
        return w.en;
    return w.jp;
}

/**
 * 初始化听读模式
 *
 * 检查词库是否已加载（为空则显示提示并返回），
 * 通过权重随机算法选取第一个单词，重置播放计数器和定时器，
 * 然后绘制听读界面并立即开始播放。
 */
void initListenMode()
{
    if (words.empty())
    {
        // 理论上应该已经在 StudyMode 中加载过词库
        canvas.fillSprite(BLACK);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(RED);
        canvas.setTextSize(1.6);
        canvas.drawString("请先加载词库", canvas.width() / 2, canvas.height() / 2);
        canvas.pushSprite(0, 0);
        return;
    }

    wordIndex = pickWeightedRandom();
    listenPlayCount = 0;
    listenNextActionTime = 0; // 立即开始播放

    drawListenWord();
}

/**
 * 绘制听读模式界面
 *
 * 在屏幕上渲染当前单词的详细信息。英语模式显示单词、音标和词性；
 * 日语模式显示日语文本和汉字读音。底部统一显示中文释义。
 * 左上角标注"听读模式"，音量调节时右上角显示当前音量值。
 */
void drawListenWord()
{
    canvas.fillSprite(BLACK);
    canvas.setTextDatum(middle_center);

    if (words.empty())
    {
        canvas.setTextColor(RED);
        canvas.setTextSize(1.6);
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

        String sub = asciiPhonetic(w.phonetic);
        if (w.pos.length() > 0)
        {
            if (sub.length() > 0)
                sub += "  ";
            sub += w.pos;
        }
        if (sub.length() > 0)
        {
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.2);
            drawAutoFitString(canvas, sub, canvas.width() / 2, canvas.height() / 2 + 10, 1.2);
        }
    }
    else if (currentLanguage == LANG_JP)
    {
        // 显示日语（中间偏上）
        canvas.setTextFont(&fonts::efontJA_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w.jp, canvas.width() / 2, canvas.height() / 2 - 25, 2.2);

        // 显示假名（中间）
        if (w.kanji.length() > 0)
        {
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.4);
            canvas.drawString(w.kanji, canvas.width() / 2, canvas.height() / 2 + 10);
        }
    }

    // 显示中文（中间偏下）
    canvas.setTextFont(&fonts::efontCN_16);
    canvas.setTextColor(YELLOW);
    drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 + 40, 1.5); // 显示中文释义

    // 模式
    drawTopLeftString(canvas, "听读模式");

    // HUD 显示音量变化
    if (millis() < volumeMessageDeadline)
    {
        drawTopRightString(canvas, String(soundVolume));
    }

    canvas.pushSprite(0, 0);
}

/**
 * 听读模式的主循环函数
 *
 * 处理两部分逻辑：
 * 1. 键盘输入：ESC 键（`）返回菜单，分号键增大音量，句号键减小音量
 * 2. 自动播放控制：每个单词播放 3 次（间隔 listenRepeatInterval 毫秒），
 *    播完后等待 listenNextWordDelay 毫秒，然后通过权重随机切换到下一个单词
 */
void loopListenMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == '`')
            {
                previousMode = appMode;
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }

            // 处理音量调节（复用学习模式逻辑）
            if (c == ';')
            { // 上：音量+
                soundVolume = min(255, soundVolume + 10);
            }
            else if (c == '.')
            { // 下：音量-
                soundVolume = max(0, soundVolume - 10);
            }

            if (c == ';' || c == '.')
            {
                M5.Speaker.setVolume(soundVolume);

                // 顺便也可以显示一下 HUD（复用 StudyMode 的那套变量）
                volumeMessageDeadline = millis() + 2000;

                // 重画当前单词（保持 HUD 内容）
                drawListenWord();
            }
        }
    }

    // ------- 自动播放控制 -------
    if (words.empty())
        return;

    unsigned long now = millis();

    // 如果现在还没到下一次动作时间,什么也不做
    if (now < listenNextActionTime)
    {
        return;
    }

    // 如果当前没有在播放任何声音
    if (!M5.Speaker.isPlaying())
    {
        if (listenPlayCount < 3)
        {
            // 第 1~3 次播放
            playAudioForWord(listenAudioText(words[wordIndex]));
            listenPlayCount++;
            listenNextActionTime = now + listenRepeatInterval;
        }
        else
        {
            // 已经播了 3 次 → 换下一个单词
            wordIndex = pickWeightedRandom();
            listenPlayCount = 0;
            listenNextActionTime = now + listenNextWordDelay;
            drawListenWord();
        }
    }
}
