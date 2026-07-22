/**
 * @file ModeScoreSelect.ino
 * @brief 按 Score 分类的词库选择模式
 *
 * 两级选择结构：
 * - 第一级：选择 Score（1~5），标题"选择级别"，显示各等级单词数量及 > 标记
 * - 第二级：选择分组（每组 ≤50 词），标题"选择分组"
 *
 * ` 键在第二级返回第一级，在第一级返回分类方式选择。
 * , 键在第二级返回第一级。
 * / 键在第一级可直接进入分组选择。
 * 进入模式时一次性查询 1~5 各级单词数，缓存后不再访问 DB。
 */

// --------- 按 Score 选择模式变量 ---------
int scoreLevel = 0;              // 0=选分数, 1=选分组
int scoreListIndex = 0;
int scoreListScroll = 0;
int groupListIndex = 0;
int groupListScroll = 0;
int selectedScore = 0;           // 第一级选中的分数值
int scoreWordCounts[6] = {0};    // 各级单词数缓存（1~5 有效）

// ===== 核心函数（init / draw / loop） =====

/**
 * 初始化按 Score 选择模式
 *
 * 预加载各级单词数缓存，重置为第一级（选择分数）。
 */
void initScoreSelectMode()
{
    loadScoreCounts(scoreWordCounts);
    scoreLevel = 0;
    scoreListIndex = 0;
    scoreListScroll = 0;
    selectedScore = 0;
    drawScoreSelect();
}

/**
 * 绘制当前层级的菜单
 *
 * 第一级显示 Score 1~5，有单词的等级标注数量和 >；
 * 第二级根据缓存中的单词数计算分组并显示（每组 ≤50 词，1-based 显示）。
 */
void drawScoreSelect()
{
    if (scoreLevel == 0) {
        String title = "选择级别";
        std::vector<String> items;
        for (int s = 1; s <= 5; s++) {
            String item = String("Level ") + s;
            if (scoreWordCounts[s] > 0) {
                item += " (" + String(scoreWordCounts[s]) + ") >";
            }
            items.push_back(item);
        }
        drawTextMenu(canvas, title, items, scoreListIndex, scoreListScroll,
                     visibleLines, "无数据", false, false);
    } else {
        String title = "选择分组";
        int total = scoreWordCounts[selectedScore];
        int groupCount = (total + 49) / 50;
        std::vector<String> items;
        for (int g = 0; g < groupCount; g++) {
            int start = g * 50 + 1;
            int end = min(g * 50 + 50, total);
            items.push_back(String(start) + "-" + String(end));
        }
        drawTextMenu(canvas, title, items, groupListIndex, groupListScroll,
                     visibleLines, "无数据", false, false);
    }
}

/**
 * 按 Score 选择模式的主循环
 *
 * 处理以下键盘操作：
 * - ` 键：第二级返回第一级，第一级返回分类方式选择
 * - , 键：第二级返回第一级
 * - / 键：第一级有单词时直接进入分组选择
 * - ; 键：向上移动光标
 * - . 键：向下移动光标
 * - Enter 键：第一级进入分组选择，第二级加载词库并进入学习模式
 */
void loopScoreSelectMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == '`')
            {
                if (scoreLevel == 1) {
                    scoreLevel = 0;
                    drawScoreSelect();
                    return;
                }
                appMode = MODE_CLASSIFY_SELECT;
                initClassifySelectMode();
                return;
            }

            if (c == ',')
            {
                if (scoreLevel == 1) {
                    scoreLevel = 0;
                    drawScoreSelect();
                    return;
                }
            }

            if (c == '/')
            {
                if (scoreLevel == 0) {
                    int s = scoreListIndex + 1;
                    if (scoreWordCounts[s] > 0) {
                        selectedScore = s;
                        groupListIndex = 0;
                        groupListScroll = 0;
                        scoreLevel = 1;
                        drawScoreSelect();
                        return;
                    }
                }
            }

            if (c == ';')
            {
                if (scoreLevel == 0) {
                    navigateMenu(scoreListIndex, scoreListScroll, 5,
                                 visibleLines, true);
                } else {
                    int total = scoreWordCounts[selectedScore];
                    int groupCount = (total + 49) / 50;
                    navigateMenu(groupListIndex, groupListScroll, groupCount,
                                 visibleLines, true);
                }
                drawScoreSelect();
            }

            if (c == '.')
            {
                if (scoreLevel == 0) {
                    navigateMenu(scoreListIndex, scoreListScroll, 5,
                                 visibleLines, false);
                } else {
                    int total = scoreWordCounts[selectedScore];
                    int groupCount = (total + 49) / 50;
                    navigateMenu(groupListIndex, groupListScroll, groupCount,
                                 visibleLines, false);
                }
                drawScoreSelect();
            }
        }

        if (st.enter)
        {
            if (scoreLevel == 0) {
                selectedScore = scoreListIndex + 1;
                if (scoreWordCounts[selectedScore] == 0) {
                    drawCenterString(canvas, "该级别无单词", RED, 1.2);
                    delay(600);
                    drawScoreSelect();
                    return;
                }
                groupListIndex = 0;
                groupListScroll = 0;
                scoreLevel = 1;
                drawScoreSelect();
                return;
            }

            autoSaveIfNeeded();
            if (!loadWordsByScore(selectedScore, groupListIndex) || words.empty()) {
                drawCenterString(canvas, "词库加载失败", RED, 1.2);
                delay(600);
                drawScoreSelect();
                return;
            }
            int start = groupListIndex * 50 + 1;
            int end = min(groupListIndex * 50 + 50, scoreWordCounts[selectedScore]);
            vocabLabel = "score:" + String(selectedScore) + "/" + String(start) + "-" + String(end);
            appMode = MODE_STUDY;
            initStudyMode();
        }
    }
}
