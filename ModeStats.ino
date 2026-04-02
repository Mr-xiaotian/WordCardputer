/**
 * @file ModeStats.ino
 * @brief 学习统计模式
 *
 * 对当前加载的词库进行学习数据统计和展示。
 * 包含三页内容：第一页显示词库概览（总数、平均分、中位数、掌握评价），
 * 第二页和第三页分别展示各等级（1-3 和 4-5）的数量和占比。
 */

int statsTotal = 0;
float statsAvg = 0;
float statsMedian = 0;
int statsCounts[6] = {0, 0, 0, 0, 0, 0};
String statsLevel = "";
int statsPage = 0;

/**
 * 绘制统计页面（共 3 页）
 *
 * 根据 statsPage 变量渲染对应页面：
 * - 第 0 页：词库概览（文件名、总数、平均分、中位数、掌握评价）
 * - 第 1 页：等级 1-3 的数量和百分比表格
 * - 第 2 页：等级 4-5 的数量和百分比表格
 * 词库为空时显示"请先加载词库"提示。右上角显示当前页码。
 */
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
        drawCenterMessage(canvas, "请先加载词库");
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

/**
 * 初始化统计模式
 *
 * 将页码重置为第一页，调用 computeStatsFromWords 计算统计数据，
 * 然后绘制统计页面。
 */
void initStatsMode()
{
    statsPage = 0;
    computeStatsFromWords();
    drawStatsPage();
}

/**
 * 统计模式的主循环函数，处理页面导航
 *
 * 处理以下键盘操作：
 * - ESC 键（`）返回 ESC 菜单
 * - 逗号键（,）向左翻页（循环）
 * - 斜杠键（/）向右翻页（循环）
 * 共 3 页，支持循环浏览。
 */
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
