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

// ===== 核心函数（init / draw / loop） =====

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
        drawCenterString(canvas, "请先加载词库", RED, 1.2);
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

    drawDictationInput();
    playAudioForWord(dictationPromptText(words[dictOrder[dictPos]]));
}

/**
 * 绘制听写答题界面
 *
 * 在屏幕上渲染当前听写输入状态。
 * - 英语模式：显示当前已输入的英文答案
 * - 日语模式：显示已提交的假名、当前罗马音缓冲和候选假名提示
 *
 * 底部显示当前进度（第几个/总数）。
 */
void drawDictationInput()
{
    canvas.fillSprite(BLACK);

    canvas.setFont(&fonts::efontCN_16);
    if (currentLanguage == LANG_EN)
        drawTopLeftString(canvas, "英语听写", TFT_DARKGREY, 1.0);
    else if (currentLanguage == LANG_JP)
        drawTopLeftString(canvas, "日语听写", TFT_DARKGREY, 1.0);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE);
    if (currentLanguage == LANG_EN)
    {
        canvas.setFont(&fonts::efontCN_16);
        drawAutoFitString(canvas, dictEnInput, canvas.width() / 2, canvas.height() / 2 - 10, 2.0);
    }
    else if (currentLanguage == LANG_JP)
    {
        canvas.setFont(&fonts::efontJA_16);
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
 * 仅负责在屏幕上显示本次听写的正确数和错误数。
 * 后续按键处理与页面跳转逻辑由 loopDictationMode() 负责。
 */
void drawDictationSummary()
{
    canvas.fillSprite(BLACK);
    canvas.setFont(&fonts::efontCN_16);

    drawTopLeftString(canvas, "听写总结", TFT_DARKGREY, 1.0);

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
 * 听写模式的主循环函数
 *
 * 处理听写模式中的所有键盘输入和状态转换，包括：
 * - ESC 键（`）返回菜单
 * - 总结界面的 Enter 键进入独立错误回顾页或返回菜单
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
                    // 有错误 → 进入独立回顾页面
                    dictShowSummary = false;
                    appMode = MODE_DICTATION_REVIEW;
                    initDictationReviewFromSession();
                }
            }
            return;
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
                    dictErrors.push_back({
                        dictOrder[dictPos],
                        words[dictOrder[dictPos]].dbId,
                        dictEnInput,
                        getNtpTimeString()
                    });
                }
            }
        }
        else if (currentLanguage == LANG_JP)
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
                    dictErrors.push_back({
                        dictOrder[dictPos],
                        words[dictOrder[dictPos]].dbId,
                        userAnswer,
                        getNtpTimeString()
                    });
                }
            }
        }

        if (st.enter)
        {
            dictPos++;

            if (dictPos >= (int)dictOrder.size())
            {
                if (dictErrors.empty()) return;

                if (saveDictationErrorsToDB(dictErrors)) {
                    Serial.printf("已写入 %d 条听写错误记录\n", dictErrors.size());
                } else {
                    Serial.println("听写错误记录写入失败");
                }
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
