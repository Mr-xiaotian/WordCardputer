int statsTotal = 0;
float statsAvg = 0;
float statsMedian = 0;
int statsCounts[6] = {0, 0, 0, 0, 0, 0};
String statsLevel = "";
int statsPage = 0;

String statsFileName(const String &path)
{
    int pos = path.lastIndexOf('/');
    if (pos >= 0 && pos + 1 < (int)path.length())
    {
        return path.substring(pos + 1);
    }
    return path;
}

void computeStatsFromWords()
{
    for (int i = 0; i < 6; i++)
    {
        statsCounts[i] = 0;
    }
    statsTotal = words.size();
    statsAvg = 0;
    statsMedian = 0;
    statsLevel = "";

    if (statsTotal == 0)
    {
        statsLevel = "词库为空";
        return;
    }

    std::vector<int> scores;
    scores.reserve(statsTotal);
    long sum = 0;

    for (auto &w : words)
    {
        int score = w.score;
        if (score < 1 || score > 5)
            score = 3;
        scores.push_back(score);
        statsCounts[score] += 1;
        sum += score;
    }

    statsAvg = (float)sum / (float)statsTotal;
    std::sort(scores.begin(), scores.end());

    if (statsTotal % 2 == 1)
    {
        statsMedian = scores[statsTotal / 2];
    }
    else
    {
        int mid = statsTotal / 2;
        statsMedian = (scores[mid - 1] + scores[mid]) / 2.0f;
    }

    if (statsAvg >= 4.5)
    {
        statsLevel = "非常熟练";
    }
    else if (statsAvg >= 3.8)
    {
        statsLevel = "较为熟练";
    }
    else if (statsAvg >= 3.0)
    {
        statsLevel = "掌握中";
    }
    else if (statsAvg >= 2.0)
    {
        statsLevel = "不牢固";
    }
    else
    {
        statsLevel = "需要重点复习";
    }
}

void drawStatsPage()
{
    canvas.fillSprite(BLACK);
    canvas.setTextFont(&fonts::efontCN_16);
    canvas.setTextDatum(top_left);
    canvas.setTextColor(GREEN);
    canvas.setTextSize(1.2);
    canvas.drawString("学习统计", 8, 6);
    canvas.setTextDatum(top_right);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString(String(statsPage + 1) + "/3", canvas.width() - 8, 8);

    if (statsTotal == 0)
    {
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(RED);
        canvas.setTextSize(1.6);
        canvas.drawString("请先加载词库", canvas.width() / 2, canvas.height() / 2);
        canvas.pushSprite(0, 0);
        return;
    }

    canvas.setTextDatum(top_left);
    canvas.setTextSize(1.1);
    const int lineHeight = 22;
    int y = 34;

    if (statsPage == 0)
    {
        int labelX = 8;
        int valueX = 72;
        String fileName = statsFileName(selectedFilePath);
        canvas.setTextColor(TFT_DARKGREY);
        canvas.drawString("词库", labelX, y);
        canvas.setTextColor(CYAN);
        canvas.drawString(fileName, valueX, y);
        y += lineHeight;
        canvas.setTextColor(TFT_DARKGREY);
        canvas.drawString("总数", labelX, y);
        canvas.setTextColor(WHITE);
        canvas.drawString(String(statsTotal), valueX, y);
        y += lineHeight;
        canvas.setTextColor(TFT_DARKGREY);
        canvas.drawString("平均", labelX, y);
        canvas.setTextColor(WHITE);
        String avgText = String(statsAvg, 2);
        canvas.drawString(avgText, valueX, y);
        int midLabelX = valueX + canvas.textWidth(avgText) + 16;
        canvas.setTextColor(TFT_DARKGREY);
        canvas.drawString("中位", midLabelX, y);
        canvas.setTextColor(WHITE);
        int midValueX = midLabelX + canvas.textWidth("中位") + 8;
        canvas.drawString(String(statsMedian, 2), midValueX, y);
        y += lineHeight;
        uint16_t levelColor = WHITE;
        if (statsAvg >= 4.5)
            levelColor = GREEN;
        else if (statsAvg >= 3.8)
            levelColor = CYAN;
        else if (statsAvg >= 3.0)
            levelColor = YELLOW;
        else if (statsAvg >= 2.0)
            levelColor = ORANGE;
        else
            levelColor = RED;
        canvas.setTextColor(TFT_DARKGREY);
        canvas.drawString("评价", labelX, y);
        canvas.setTextColor(levelColor);
        canvas.drawString(statsLevel, valueX, y);
    }
    else if (statsPage == 1)
    {
        int p1 = (int)round(statsCounts[1] * 100.0f / statsTotal);
        int p2 = (int)round(statsCounts[2] * 100.0f / statsTotal);
        int p3 = (int)round(statsCounts[3] * 100.0f / statsTotal);
        int p4 = (int)round(statsCounts[4] * 100.0f / statsTotal);
        int p5 = (int)round(statsCounts[5] * 100.0f / statsTotal);
        std::vector<String> headers = {"等级", "数量", "占比"};
        std::vector<int> colXs = {8, 92, 154};
        std::vector<std::vector<String>> rows = {
            {"1", String(statsCounts[1]), String(p1) + "%"},
            {"2", String(statsCounts[2]), String(p2) + "%"},
            {"3", String(statsCounts[3]), String(p3) + "%"}
        };
        drawSimpleTable(
            canvas,
            y,
            lineHeight,
            headers,
            colXs,
            1.0,
            1.1,
            rows
        );
    }
    else
    {
        int p4 = (int)round(statsCounts[4] * 100.0f / statsTotal);
        int p5 = (int)round(statsCounts[5] * 100.0f / statsTotal);
        std::vector<String> headers = {"等级", "数量", "占比"};
        std::vector<int> colXs = {8, 92, 154};
        std::vector<std::vector<String>> rows = {
            {"4", String(statsCounts[4]), String(p4) + "%"},
            {"5", String(statsCounts[5]), String(p5) + "%"}
        };
        drawSimpleTable(
            canvas,
            y,
            lineHeight,
            headers,
            colXs,
            1.0,
            1.1,
            rows
        );
    }

    canvas.pushSprite(0, 0);
}

void initStatsMode()
{
    statsPage = 0;
    computeStatsFromWords();
    drawStatsPage();
}

void loopStatsMode()
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
            if (c == ',')
            {
                statsPage = (statsPage - 1 + 3) % 3;
                drawStatsPage();
                return;
            }
            if (c == '/')
            {
                statsPage = (statsPage + 1) % 3;
                drawStatsPage();
                return;
            }
        }
    }
}
