/**
 * @file ModeDictation.ino
 * @brief 听写模式（带 Level1 日语罗马音输入法）
 *
 * 实现单词听写功能，支持英语和日语两种语言。
 * 英语模式下直接键入字母；日语模式下通过罗马音输入法将按键转换为假名。
 * 听写完成后展示正确/错误统计，并提供错误回顾功能。
 */

// =============== 听写模式（带 Level1 日语输入法） ===============

std::vector<int> dictOrder; // 随机顺序
int dictPos = 0;            // 当前是第几个单词

// 输入法相关
String commitText = "";    // 已经“确认提交”的假名（真正作为答案的一部分）
String romajiBuffer = "";  // 当前正在输入的罗马音
String candidateKana = ""; // 当前 romajiBuffer 对应的候选假名（还未提交）
String dictEnInput = "";

// 统计
int correctCount = 0;
int wrongCount = 0;

bool dictShowSummary = false;
bool useKatakana = false;

// 错误回顾
bool dictInReview = false; // 是否正在错误回顾

/**
 * 初始化听写模式
 *
 * 检查词库是否已加载，生成随机顺序的单词序列，
 * 重置所有输入状态（罗马音缓冲、候选假名、已提交文本）和统计计数，
 * 绘制初始界面并播放第一个单词的语音。
 * 若词库为空则显示错误提示。
 */
void initDictationMode()
{
    if (words.empty())
    {
        drawCenterMessage(canvas, "请先加载词库");
        return;
    }

    dictOrder.clear();
    for (int i = 0; i < (int)words.size(); i++)
        dictOrder.push_back(i);

    // 随机顺序
    for (int i = 0; i < (int)dictOrder.size(); i++)
    {
        int j = random(dictOrder.size());
        std::swap(dictOrder[i], dictOrder[j]);
    }

    dictPos = 0;

    commitText = "";
    romajiBuffer = "";
    candidateKana = "";
    dictEnInput = "";

    correctCount = 0;
    wrongCount = 0;

    dictErrors.clear();
    dictShowSummary = false;
    dictInReview = false;

    drawDictationInput();
    playAudioForWord(dictationPromptText(words[dictOrder[dictPos]]));
}

/**
 * 绘制听写答题界面
 *
 * 在屏幕上渲染当前听写输入状态。英语模式显示已输入的英文字母、
 * 音标和词性；日语模式显示已提交的假名加当前罗马音缓冲，
 * 以及候选假名提示。底部显示当前进度（第几个/总数）。
 */
void drawDictationInput()
{
    canvas.fillSprite(BLACK);

    canvas.setTextFont(&fonts::efontCN_16);
    if (currentLanguage == LANG_EN)
        drawTopLeftString(canvas, "英语听写");
    else if (currentLanguage == LANG_JP)
        drawTopLeftString(canvas, "日语听写");

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE);
    if (currentLanguage == LANG_EN)
    {
        canvas.setTextFont(&fonts::efontCN_16);
        drawAutoFitString(canvas, dictEnInput, canvas.width() / 2, canvas.height() / 2 - 10, 2.0);
        Word &w = words[dictOrder[dictPos]];
        String sub = asciiPhonetic(w.phonetic);
        if (w.pos.length() > 0)
        {
            if (sub.length() > 0)
                sub += "  ";
            sub += w.pos;
        }
        if (sub.length() > 0)
        {
            canvas.setTextColor(TFT_CYAN);
            drawAutoFitString(canvas, sub, canvas.width() / 2, canvas.height() / 2 + 20, 1.2);
        }
    }
    else if (currentLanguage == LANG_JP)
    {
        canvas.setTextFont(&fonts::efontJA_16);
        canvas.setTextSize(2.0);
        String mainLine = commitText + romajiBuffer;
        drawAutoFitString(canvas, mainLine, canvas.width() / 2, canvas.height() / 2 - 10, 2.0);

        canvas.setTextSize(1.4);
        canvas.setTextColor(TFT_CYAN);
        if (candidateKana.length() > 0)
        {
            canvas.drawString(candidateKana, canvas.width() / 2, canvas.height() / 2 + 20);
        }
    }

    canvas.setTextDatum(bottom_center);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString(
        String(dictPos + 1) + "/" + String(dictOrder.size()),
        canvas.width() / 2,
        canvas.height() - 10);

    canvas.pushSprite(0, 0);
}

/**
 * 绘制听写结果总结界面
 *
 * 在屏幕上显示本次听写的正确数和错误数。
 * 用户可按 Enter 键继续：若有错误则进入错误回顾，否则返回 ESC 菜单。
 */
void drawDictationSummary()
{
    canvas.fillSprite(BLACK);
    canvas.setTextFont(&fonts::efontCN_16);

    drawTopLeftString(canvas, "听写总结");

    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1.3);

    canvas.setTextColor(GREEN);
    canvas.drawString("正确: " + String(correctCount),
                      canvas.width() / 2, canvas.height() / 2);
    canvas.setTextColor(RED);
    canvas.drawString("错误: " + String(wrongCount),
                      canvas.width() / 2, canvas.height() / 2 + 40);

    canvas.pushSprite(0, 0);
}

/**
 * 绘制错误回顾页面
 *
 * 显示当前错误记录的正确答案（绿色）和用户的错误答案（红色），
 * 底部显示当前页码。若无错误记录则显示"没有错误记录"提示。
 * 支持左右翻页浏览所有错误，以及 Fn 键重播当前单词语音。
 */
void drawDictationReviewPage()
{
    canvas.fillSprite(BLACK);

    if (dictErrors.empty())
    {
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(TFT_DARKGREY);
        canvas.drawString("没有错误记录", canvas.width() / 2, canvas.height() / 2);
        canvas.pushSprite(0, 0);
        return;
    }

    DictError &e = dictErrors[reviewPos];
    Word &w = words[e.wordIndex];

    // 标题
    canvas.setTextFont(&fonts::efontCN_16);
    drawTopLeftString(canvas, "错误回顾");


    // 正确答案
    canvas.setTextFont(&fonts::efontCN_16);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(GREEN);
    drawAutoFitString(canvas, dictationPromptText(w), canvas.width() / 2, canvas.height() / 2 - 25, 2.0);

    // 你的答案
    canvas.setTextColor(RED);
    drawAutoFitString(canvas, e.wrong, canvas.width() / 2, canvas.height() / 2 + 20, 2.0);

    // 页码
    canvas.setTextDatum(bottom_center);
    canvas.setTextSize(1.0);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.drawString(
        String(reviewPos + 1) + "/" + String(dictErrors.size()),
        canvas.width() / 2, canvas.height() - 10);

    canvas.pushSprite(0, 0);
}

/**
 * 听写模式的主循环函数
 *
 * 处理听写模式中的所有键盘输入和状态转换，包括：
 * - ESC 键（`）返回菜单
 * - 总结界面的 Enter 键进入错误回顾或返回菜单
 * - 错误回顾模式下的翻页（,/.）和重播（Fn）
 * - 英语模式的字符输入、删除和答案提交
 * - 日语模式的罗马音输入、促音检测、假名候选、Shift 切换片假名
 * - Enter 键提交答案并判定正误，自动切换到下一个单词
 * - 所有单词完成后保存错误记录并显示总结
 */
void loopDictationMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == '`')
            {
                dictOrder.clear();
                previousMode = appMode;
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
        }

        // -------- 显示总结界面 --------
        if (dictShowSummary)
        {
            if (st.enter)
            {
                if (dictErrors.empty())
                {
                    // 没有错误 → 直接返回 ESC 菜单
                    appMode = MODE_ESC_MENU;
                    initEscMenuMode();
                }
                else
                {
                    // 有错误 → 进入回顾模式
                    dictShowSummary = false;
                    dictInReview = true;
                    reviewPos = 0;
                    drawDictationReviewPage();
                }
            }
            return;
        }

        // ========== 错误回顾模式 ==========
        if (dictInReview)
        {
            // 左翻页
            for (auto c : st.word)
            {
                if (c == ',')
                {
                    reviewPos = (reviewPos - 1 + dictErrors.size()) % dictErrors.size();
                    drawDictationReviewPage();
                }
            }

            // 右翻页
            for (auto c : st.word)
            {
                if (c == '/')
                {
                    reviewPos = (reviewPos + 1) % dictErrors.size();
                    drawDictationReviewPage();
                }
            }

            // -------- Fn 键：重复播放当前单词音频 --------
            if (st.fn)
            {
                DictError &e = dictErrors[reviewPos];
                Word &w = words[e.wordIndex];
                playAudioForWord(dictationPromptText(w));
            }

            return; // 防止进入正常输入逻辑
        }

        // -------- 字符输入（字母、确认键等）--------
        if (currentLanguage == LANG_EN)
        {
            for (auto c : st.word)
            {
                if (isEnglishInputChar(c))
                {
                    char out = (char)tolower(c);
                    if (out == '_')
                        out = ' ';
                    dictEnInput += out;
                    drawDictationInput();
                }
            }

            if (st.del)
            {
                if (dictEnInput.length() > 0)
                {
                    dictEnInput.remove(dictEnInput.length() - 1);
                    drawDictationInput();
                }
            }

            if (st.enter)
            {
                String userAnswer = normalizeEnglishAnswer(dictEnInput);
                String correct = normalizeEnglishAnswer(words[dictOrder[dictPos]].en);

                if (userAnswer == correct)
                {
                    correctCount++;
                }
                else
                {
                    wrongCount++;
                    dictErrors.push_back({dictOrder[dictPos], dictEnInput});
                }
            }
        }
        else
        {
            for (auto c : st.word)
            {
                if (c == ';')
                {
                    commitCandidate();
                    drawDictationInput();
                    continue;
                }

            // 字母输入 → romajiBuffer
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-')
                {
                    char lc = (char)tolower(c);

                // ------------ 促音检测：双辅音触发「っ / ッ」 ------------
                    if (romajiBuffer.length() == 1 &&
                        romajiBuffer[0] == lc &&
                        isSokuonConsonant(lc))
                    {
                        // 输出促音
                        commitText += (useKatakana ? "ッ" : "っ");

                        // 把第二个辅音当作下一个音节的开始
                        romajiBuffer = "";
                        romajiBuffer += lc;

                        candidateKana = matchRomaji(romajiBuffer, useKatakana);
                        drawDictationInput();
                        continue;  // <- 不走下面旧逻辑
                    }

                // ------------ 正常字母输入逻辑 ------------
                    romajiBuffer += lc;

                    // 重新计算候选假名
                    candidateKana = matchRomaji(romajiBuffer, useKatakana);
                    drawDictationInput();
                }
            }

            // -------- 删除键处理 --------
            if (st.del)
            {
                if (romajiBuffer.length() > 0)
                {
                    // 优先删 romajiBuffer 的尾字母
                    romajiBuffer.remove(romajiBuffer.length() - 1);
                    candidateKana = matchRomaji(romajiBuffer, useKatakana);
                }
                else if (commitText.length() > 0)
                {
                    removeLastUTF8Char(commitText);
                }
                drawDictationInput();
            }

        // -------- Shift 键：切换假名模式 --------
            if (st.shift)
            {
                useKatakana = !useKatakana;
                candidateKana = matchRomaji(romajiBuffer, useKatakana);
                drawDictationInput();
            }

        // -------- ENTER 检查答案 --------
            if (st.enter)
            {
                // 先把当前候选假名也提交掉（方便操作）
                commitCandidate();

                String userAnswer = commitText;                // 用户最终提交的假名
                String correct = words[dictOrder[dictPos]].jp; // 正确日语（假名）

                if (userAnswer == correct)
                {
                    correctCount++;
                }
                else
                {
                    wrongCount++;
                    dictErrors.push_back({dictOrder[dictPos], userAnswer});
                }
            }
        }

        if (st.enter)
        {
            dictPos++;

            if (dictPos >= (int)dictOrder.size())
            {
                saveDictationMistakesAsWordList();
                dictShowSummary = true;
                drawDictationSummary();
                return;
            }

            // 进入下一个单词
            commitText = "";
            romajiBuffer = "";
            candidateKana = "";
            dictEnInput = "";
            drawDictationInput();
            playAudioForWord(dictationPromptText(words[dictOrder[dictPos]]));
        }

        // -------- Fn 键：重复播放当前单词音频 --------
        if (st.fn)
        {
            // 这里用当前听写单词,而不是 wordIndex 
            playAudioForWord(dictationPromptText(words[dictOrder[dictPos]]));
        }
    }
}
