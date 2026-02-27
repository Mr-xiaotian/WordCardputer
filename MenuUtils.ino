// 通用文本菜单绘制
void drawTextMenu(
    M5Canvas &cv,
    const String &title,                  // 标题文本
    const std::vector<String> &items,     // 菜单项
    int selectedIndex,                    // 当前选中下标
    int scrollIndex,                      // 当前滚动起始下标
    int visibleLines,                     // 一屏显示多少行
    const String &emptyText,              // 没有项目时显示的文字
    bool showBattery,                     // 是否显示右上角电量
    bool showPager                        // 是否显示右下角分页
) {
    cv.fillSprite(BLACK);
    cv.setTextFont(&fonts::efontCN_16);

    // 标题（左上角）
    cv.setTextDatum(top_left);
    cv.setTextColor(GREEN);
    cv.setTextSize(1.2);
    cv.drawString(title, 8, 8);

    // 电量（右上角）
    if (showBattery) {
        int batteryLevel = M5Cardputer.Power.getBatteryLevel();
        cv.setTextDatum(top_right);
        cv.setTextColor(TFT_DARKGREY);
        cv.setTextSize(1.0);
        cv.drawString(String(batteryLevel) + " %", cv.width() - 8, 8);
    }

    // 列表区域
    cv.setTextDatum(top_left);
    cv.setTextSize(1.2);

    const int lineHeight = 24;
    const int startY     = 40;

    if (items.empty()) {
        // 没有任何项目时的提示
        cv.setTextColor(RED);
        cv.setTextDatum(middle_center);
        cv.drawString(emptyText, cv.width() / 2, cv.height() / 2);
        cv.setTextDatum(top_left);
    } else {
        int end = min(scrollIndex + visibleLines, (int)items.size());
        for (int i = scrollIndex; i < end; i++) {
            int y = startY + (i - scrollIndex) * lineHeight;
            if (i == selectedIndex) {
                cv.setTextColor(YELLOW);
                cv.drawString("> " + items[i], 8, y);
            } else {
                cv.setTextColor(WHITE);
                cv.drawString("  " + items[i], 8, y);
            }
        }
    }

    // 右下角分页：当前/总数
    if (showPager && items.size() > (size_t)visibleLines) {
        cv.setTextColor(TFT_DARKGREY);
        cv.setTextDatum(bottom_right);
        cv.setTextSize(1.0);
        cv.drawString(
            String(selectedIndex + 1) + "/" + String(items.size()),
            cv.width() - 8,
            cv.height() - 8
        );
        cv.setTextDatum(top_left);  // 恢复
    }

    cv.pushSprite(0, 0);
}

void drawSimpleTable(
    M5Canvas &cv,
    int startX,
    int startY,
    int rowHeight,
    const String &h1,
    const String &h2,
    const String &h3,
    int col2X,
    int col3X,
    const std::vector<String> &col1,
    const std::vector<String> &col2,
    const std::vector<String> &col3
) {
    cv.setTextDatum(top_left);
    cv.setTextColor(TFT_DARKGREY);
    cv.drawString(h1, startX, startY);
    cv.drawString(h2, col2X, startY);
    cv.drawString(h3, col3X, startY);

    int lineY = startY + rowHeight - 4;
    cv.drawLine(startX, lineY, cv.width() - startX, lineY, TFT_DARKGREY);

    cv.setTextColor(WHITE);
    int rows = min((int)col1.size(), min((int)col2.size(), (int)col3.size()));
    int y = startY + rowHeight + 2;
    for (int i = 0; i < rows; i++)
    {
        cv.drawString(col1[i], startX, y);
        cv.drawString(col2[i], col2X, y);
        cv.drawString(col3[i], col3X, y);
        y += rowHeight;
    }
}
