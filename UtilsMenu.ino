/**
 * MenuUtils.ino - 通用菜单与表格绘制工具
 *
 * 提供可复用的 UI 组件函数，包括带滚动、选中高亮、电量显示和分页指示器的
 * 文本菜单，以及带表头和分隔线的简易表格渲染器。
 */

/**
 * 绘制通用可滚动文本菜单
 *
 * 在画布上绘制一个完整的菜单界面，包含：
 * - 左上角绿色标题
 * - 右上角电池电量百分比（可选）
 * - 带高亮选中项的可滚动列表（选中项显示为黄色带 ">" 前缀）
 * - 右下角"当前/总数"分页指示器（当列表超出一屏时显示，可选）
 * - 列表为空时显示自定义提示文字
 *
 * @param cv 目标画布引用
 * @param title 菜单标题文本
 * @param items 菜单项字符串列表
 * @param selectedIndex 当前选中项的索引
 * @param scrollIndex 当前滚动偏移（列表中第一个可见项的索引）
 * @param visibleLines 一屏最多显示的行数
 * @param emptyText 列表为空时显示的提示文字，默认"无项目"
 * @param showBattery 是否在右上角显示电池电量，默认 true
 * @param showPager 是否在右下角显示分页信息，默认 true
 */
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

/**
 * 绘制通用简易表格
 *
 * 在画布的指定位置绘制一个带表头和分隔线的表格。
 * 表头行使用灰色字体，数据行使用白色字体，表头下方绘制水平分隔线。
 * 列数取 headers 和 colXs 的较小值，每行的列数取该行数据和列数的较小值。
 *
 * @param cv 目标画布引用
 * @param startY 表格起始 Y 坐标（表头顶部位置）
 * @param rowHeight 每行的高度（像素）
 * @param headers 表头字符串列表
 * @param colXs 每列的 X 坐标列表
 * @param headerSize 表头文字大小
 * @param bodySize 数据行文字大小
 * @param rows 二维字符串数组，每个子数组为一行数据
 */
void drawSimpleTable(
    M5Canvas &cv,
    int startY,
    int rowHeight,
    const std::vector<String> &headers,
    const std::vector<int> &colXs,
    float headerSize,
    float bodySize,
    const std::vector<std::vector<String>> &rows
) {
    cv.setTextDatum(top_left);
    cv.setTextColor(TFT_DARKGREY);
    cv.setTextSize(headerSize);
    int cols = min((int)headers.size(), (int)colXs.size());
    for (int i = 0; i < cols; i++)
    {
        cv.drawString(headers[i], colXs[i], startY);
    }

    int lineY = startY + rowHeight - 4;
    if (cols > 0)
    {
        cv.drawLine(colXs[0], lineY, cv.width() - colXs[0], lineY, TFT_DARKGREY);
    }

    cv.setTextColor(WHITE);
    cv.setTextSize(bodySize);
    int y = startY + rowHeight + 2;
    for (size_t r = 0; r < rows.size(); r++)
    {
        const auto &row = rows[r];
        int rowCols = min((int)row.size(), cols);
        for (int c = 0; c < rowCols; c++)
        {
            cv.drawString(row[c], colXs[c], y);
        }
        y += rowHeight;
    }
}
