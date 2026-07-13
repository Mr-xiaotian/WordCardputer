/**
 * @file ModeWordTable.ino
 * @brief 当前范围按分数词表模式
 *
 * 展示当前已加载词库的词表视图。页面按 score=1~5 分组，
 * 每个分组在小屏上使用多页表格展示，便于快速查看当前范围内
 * 哪些词属于某个分数段。
 */

int wordTableScore = 1;
int wordTablePage = 0;
const int wordTableRowsPerPage = 3;
std::vector<int> wordTableFilteredIndices;

// ===== 工具函数 =====

/**
 * 重建当前 score 分组的词索引列表。
 */
void rebuildWordTableFilteredIndices()
{
    wordTableFilteredIndices.clear();
    for (int i = 0; i < (int)words.size(); i++)
    {
        if (words[i].score == wordTableScore)
        {
            wordTableFilteredIndices.push_back(i);
        }
    }
}

/**
 * 获取当前分组总页数。
 */
int wordTableTotalPages()
{
    if (wordTableFilteredIndices.empty())
    {
        return 1;
    }
    return ((int)wordTableFilteredIndices.size() + wordTableRowsPerPage - 1) / wordTableRowsPerPage;
}

/**
 * 切换到指定分数组并重置页码。
 */
void switchWordTableScore(int nextScore)
{
    wordTableScore = constrain(nextScore, 1, 5);
    wordTablePage = 0;
    rebuildWordTableFilteredIndices();
}

/**
 * 按方向切换到下一个非空分数组。
 *
 * @param delta -1 表示向前，1 表示向后
 */
void stepWordTableScore(int delta)
{
    int originalScore = wordTableScore;
    for (int i = 0; i < 5; i++)
    {
        int nextScore = wordTableScore + delta;
        if (nextScore < 1)
        {
            nextScore = 5;
        }
        else if (nextScore > 5)
        {
            nextScore = 1;
        }

        switchWordTableScore(nextScore);
        if (!wordTableFilteredIndices.empty())
        {
            return;
        }
    }

    switchWordTableScore(originalScore);
}

/**
 * 若 score=1 为空，则定位到第一个非空分组。
 */
void moveWordTableToFirstNonEmptyScore()
{
    for (int score = 1; score <= 5; score++)
    {
        wordTableScore = score;
        rebuildWordTableFilteredIndices();
        if (!wordTableFilteredIndices.empty())
        {
            wordTablePage = 0;
            return;
        }
    }

    wordTableScore = 1;
    wordTablePage = 0;
    rebuildWordTableFilteredIndices();
}

// ===== 核心函数（init / draw / loop） =====

/**
 * 绘制当前词表页面。
 */
void drawWordTablePage()
{
    canvas.fillSprite(BLACK);
    canvas.setTextFont(&fonts::efontCN_16);
    drawTopLeftString(canvas, "当前词表", GREEN, 1.2);
    drawTopRightString(
        canvas,
        "S" + String(wordTableScore) + " " + String(wordTablePage + 1) + "/" + String(wordTableTotalPages()),
        TFT_DARKGREY,
        1.0
    );

    if (words.empty())
    {
        drawCenterString(canvas, "请先加载词库", RED, 1.2);
        canvas.pushSprite(0, 0);
        return;
    }

    canvas.setTextDatum(top_left);
    canvas.setTextSize(1.0);

    const int pageStart = wordTablePage * wordTableRowsPerPage;
    const int pageEnd = min(pageStart + wordTableRowsPerPage, (int)wordTableFilteredIndices.size());

    std::vector<String> headers = {
        (currentLanguage == LANG_EN) ? "英文" : "日语",
        "中文",
        "分数"
    };
    std::vector<std::vector<String>> rows;

    for (int i = pageStart; i < pageEnd; i++)
    {
        const Word &w = words[wordTableFilteredIndices[i]];
        String surface = (currentLanguage == LANG_EN) ? w.en : w.jp;
        rows.push_back({
            surface,
            w.zh,
            String(w.score)
        });
    }

    drawSimpleTable(
        canvas,
        headers,
        rows
    );

    canvas.pushSprite(0, 0);
}

/**
 * 初始化词表模式。
 */
void initWordTableMode()
{
    wordTableScore = 1;
    wordTablePage = 0;
    moveWordTableToFirstNonEmptyScore();
    drawWordTablePage();
}

/**
 * 词表模式主循环。
 */
void loopWordTableMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == '`')
            {
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
            if (c == ';')
            {
                stepWordTableScore(-1);
                drawWordTablePage();
                return;
            }
            if (c == '.')
            {
                stepWordTableScore(1);
                drawWordTablePage();
                return;
            }
            if (c == ',')
            {
                int totalPages = wordTableTotalPages();
                wordTablePage = (wordTablePage - 1 + totalPages) % totalPages;
                drawWordTablePage();
                return;
            }
            if (c == '/')
            {
                int totalPages = wordTableTotalPages();
                wordTablePage = (wordTablePage + 1) % totalPages;
                drawWordTablePage();
                return;
            }
        }
    }
}
