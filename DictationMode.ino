// =============== 听写模式（带 Level1 日语输入法） ===============

std::vector<int> dictOrder; // 随机顺序
int dictPos = 0;            // 当前是第几个单词

// 输入法相关
String commitText = "";    // 已经“确认提交”的假名（真正作为答案的一部分）
String romajiBuffer = "";  // 当前正在输入的罗马音
String candidateKana = ""; // 当前 romajiBuffer 对应的候选假名（还未提交）

// 统计
int correctCount = 0;
int wrongCount = 0;

bool dictShowSummary = false;
bool useKatakana = false;

// 错误回顾
struct DictError
{
    int wordIndex;
    String wrong;
};
std::vector<DictError> dictErrors;

int reviewPos = 0;         // 当前错误回顾的索引
bool dictInReview = false; // 是否正在错误回顾

// ---------- 初始化听写模式 ----------
void initDictationMode()
{
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

    correctCount = 0;
    wrongCount = 0;

    dictErrors.clear();
    dictShowSummary = false;
    dictInReview = false;

    drawDictationInput();
    playAudioForWord(words[dictOrder[dictPos]].jp);
}

// ---------- 绘制答题中的画面 ----------
void drawDictationInput()
{
    canvas.fillSprite(BLACK);

    // 标题（中文）
    canvas.setTextFont(&fonts::efontCN_16);
    drawTitleString(canvas, "听写模式");

    // 主输入显示：已提交假名 + 当前罗马音（大字号，居中）
    canvas.setTextFont(&fonts::efontJA_16);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE);
    canvas.setTextSize(2.0);
    String mainLine = commitText + romajiBuffer;
    drawAutoFitString(canvas, mainLine, canvas.width() / 2, canvas.height() / 2 - 10, 2.0);

    // 候选假名（小一号字，显示在下面）
    canvas.setTextSize(1.4);
    canvas.setTextColor(TFT_CYAN);
    if (candidateKana.length() > 0)
    {
        canvas.drawString(candidateKana, canvas.width() / 2, canvas.height() / 2 + 20);
    }

    // 进度提示
    canvas.setTextDatum(bottom_center);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString(
        String(dictPos + 1) + "/" + String(dictOrder.size()),
        canvas.width() / 2,
        canvas.height() - 10);

    canvas.pushSprite(0, 0);
}

// ---------- 绘制总结界面 ----------
void drawDictationSummary()
{
    canvas.fillSprite(BLACK);
    canvas.setTextFont(&fonts::efontCN_16);

    drawTitleString(canvas, "听写总结");

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

// ---------- 绘制错误回顾界面 ----------
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
    drawTitleString(canvas, "错误回顾");

    // 正确答案
    canvas.setTextFont(&fonts::efontJA_16);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(GREEN);
    drawAutoFitString(canvas, w.jp, canvas.width() / 2, canvas.height() / 2 - 25, 2.0);

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

// 帮助函数：提交当前候选假名（由 ';' 或 Enter 触发）
void commitCandidate()
{
    if (candidateKana.length() > 0)
    {
        commitText += candidateKana;
        romajiBuffer = "";
        candidateKana = "";
    }
}

// ---------- 听写模式逻辑 ----------
void loopDictationMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        // -------- ESC 返回上级菜单 --------
        for (auto c : st.word)
        {
            if (c == '`')
            {                           // ESC
                dictOrder.clear();
                previousMode = appMode; // 记录当前模式
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
                playAudioForWord(w.jp);
            }

            return; // 防止进入正常输入逻辑
        }

        // -------- 字符输入（字母、确认键等）--------
        for (auto c : st.word)
        {
            // ';' 用来“确认”候选假名
            if (c == ';')
            {
                commitCandidate();
                drawDictationInput();
                continue;
            }

            // 字母输入 → romajiBuffer
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            {
                char lc = (char)tolower(c);
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

            dictPos++;

            if (dictPos >= (int)dictOrder.size())
            {
                dictShowSummary = true;
                drawDictationSummary();
                return;
            }

            // 进入下一个单词
            commitText = "";
            romajiBuffer = "";
            candidateKana = "";
            drawDictationInput();
            playAudioForWord(words[dictOrder[dictPos]].jp);
        }

        // -------- Fn 键：重复播放当前单词音频 --------
        if (st.fn)
        {
            // 这里用当前听写单词，而不是 wordIndex
            playAudioForWord(words[dictOrder[dictPos]].jp);
        }
    }
}
