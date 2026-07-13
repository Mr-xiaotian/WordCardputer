/**
 * @file UtilsTable.ino
 * @brief 通用表格绘制工具
 *
 * 提供可复用的表格布局与绘制函数，包括：
 * - 表格列宽测量
 * - 列宽均衡调整（水位线算法 + 截断降级）
 * - 表头与正文绘制
 * - 单元格超宽裁剪
 */

/**
 * 绘制通用简易表格
 *
 * 在画布的指定位置绘制一个带表头和分隔线的表格。
 * 表头行使用灰色字体，数据行使用白色字体，表头下方绘制水平分隔线。
 * 列布局根据列数自动计算，每行的列数取该行数据和列数的较小值。
 *
 * @param cv 目标画布引用
 * @param headers 表头字符串列表
 * @param rows 二维字符串数组，每个子数组为一行数据
 */
void drawSimpleTable(
    M5Canvas &cv,
    const std::vector<String> &headers,
    const std::vector<std::vector<String>> &rows)
{
    cv.setTextDatum(top_left);
    const int startY = 34;
    const int rowHeight = 22;
    const float headerSize = 1.0;
    const float bodySize = 1.1;

    std::vector<int> colXs;
    std::vector<int> colWidths;
    int cols = _buildSimpleTableLayout(
        cv,
        headers,
        rows,
        headerSize,
        bodySize,
        colXs,
        colWidths);
    if (cols <= 0)
    {
        return;
    }

    _drawSimpleTableHeader(cv, headers, colXs, colWidths, cols, startY, rowHeight, headerSize);
    _drawSimpleTableRows(cv, rows, colXs, colWidths, cols, startY, rowHeight, bodySize);
}

/**
 * 计算当前列宽数组的总宽度。
 *
 * @param colWidths 各列像素宽度
 * @return 所有列宽之和
 */
int _simpleTableTotalWidth(const std::vector<int> &colWidths)
{
    int total = 0;
    for (int width : colWidths)
    {
        total += width;
    }
    return total;
}

/**
 * 列宽均衡分配算法。
 *
 * 策略：
 * 1. 总宽充足（targetTotal >= sum(minColWidths)）：水位线均衡，
 *    剩余空间优先给当前最窄的列，直到趋于一致。
 * 2. 总宽不足但够显示 "..."（targetTotal >= truncateMinWidth * cols）：
 *    按比例压缩，底线为 truncateMinWidth。
 * 3. 总宽严重不足（targetTotal >= emergencyMinWidth * cols）：
 *    底线为 emergencyMinWidth，按比例分配。
 * 4. 极端情况：强行均分。
 *
 * @param targetTotal 目标总列宽（可用内容宽度）
 * @param minColWidths 各列最小宽度（内容最大宽度 + padding）
 * @param colWidths 输出参数，各列最终宽度
 * @param truncateMinWidth 能显示 "..." 的最小列宽
 * @param emergencyMinWidth 极端情况下的最小列宽（至少显示1个字符）
 */
void _rebalanceSimpleTableWidths(
    int targetTotal,
    const std::vector<int> &minColWidths,
    std::vector<int> &colWidths,
    int truncateMinWidth,
    int emergencyMinWidth)
{
    int cols = (int)colWidths.size();
    if (cols <= 0)
    {
        return;
    }

    int totalMin = 0;
    for (int w : minColWidths)
    {
        totalMin += w;
    }

    // ===== 情况1：总宽充足，水位线均衡 =====
    if (totalMin <= targetTotal)
    {
        colWidths = minColWidths;
        int remaining = targetTotal - totalMin;

        while (remaining > 0)
        {
            // 找当前最窄宽度
            int currentMin = colWidths[0];
            for (int w : colWidths)
            {
                if (w < currentMin)
                    currentMin = w;
            }

            // 收集所有处于最窄水平的列
            std::vector<int> candidates;
            for (int i = 0; i < cols; i++)
            {
                if (colWidths[i] == currentMin)
                    candidates.push_back(i);
            }

            // 找下一个水平（比 currentMin 大的最小值）
            int nextLevel = INT_MAX;
            for (int w : colWidths)
            {
                if (w > currentMin && w < nextLevel)
                    nextLevel = w;
            }

            // 尝试批量提升到下一个水平
            if (nextLevel != INT_MAX)
            {
                int gap = nextLevel - currentMin;
                int need = (int)candidates.size() * gap;
                if (need <= remaining)
                {
                    for (int i : candidates)
                    {
                        colWidths[i] += gap;
                    }
                    remaining -= need;
                    continue;
                }
            }

            // 空间不够批量提升，逐列+1
            if ((int)candidates.size() <= remaining)
            {
                for (int i : candidates)
                {
                    colWidths[i]++;
                    remaining--;
                }
            }
            else
            {
                for (int i = 0; i < remaining; i++)
                {
                    colWidths[candidates[i]]++;
                }
                remaining = 0;
            }
        }
        return;
    }

    // ===== 情况2：总宽不足，允许截断（底线 = truncateMinWidth） =====
    if (targetTotal >= truncateMinWidth * cols)
    {
        int base = truncateMinWidth * cols;
        int available = targetTotal - base;
        int totalContent = 0;
        for (int w : minColWidths)
        {
            totalContent += w;
        }

        colWidths.resize(cols);
        for (int i = 0; i < cols; i++)
        {
            int extra = (totalContent > 0)
                            ? (int)((long long)available * minColWidths[i] / totalContent)
                            : 0;
            colWidths[i] = truncateMinWidth + extra;
        }

        // 处理舍入误差：把剩余空间补给最"亏"的列（实际/期望比例最小）
        int deficit = targetTotal;
        for (int w : colWidths)
            deficit -= w;
        while (deficit > 0)
        {
            float minRatio = 1e30f;
            int idx = 0;
            for (int i = 0; i < cols; i++)
            {
                float expected = truncateMinWidth + (float)available * minColWidths[i] / totalContent;
                if (expected > 0)
                {
                    float ratio = (float)colWidths[i] / expected;
                    if (ratio < minRatio)
                    {
                        minRatio = ratio;
                        idx = i;
                    }
                }
            }
            colWidths[idx]++;
            deficit--;
        }
        return;
    }

    // ===== 情况3：严重不足（底线 = emergencyMinWidth） =====
    if (targetTotal >= emergencyMinWidth * cols)
    {
        int base = emergencyMinWidth * cols;
        int available = targetTotal - base;
        int totalContent = 0;
        for (int w : minColWidths)
        {
            totalContent += w;
        }

        colWidths.resize(cols);
        for (int i = 0; i < cols; i++)
        {
            int extra = (totalContent > 0)
                            ? (int)((long long)available * minColWidths[i] / totalContent)
                            : 0;
            colWidths[i] = emergencyMinWidth + extra;
        }

        int deficit = targetTotal;
        for (int w : colWidths)
            deficit -= w;
        while (deficit > 0)
        {
            float minRatio = 1e30f;
            int idx = 0;
            for (int i = 0; i < cols; i++)
            {
                float expected = emergencyMinWidth + (float)available * minColWidths[i] / totalContent;
                if (expected > 0)
                {
                    float ratio = (float)colWidths[i] / expected;
                    if (ratio < minRatio)
                    {
                        minRatio = ratio;
                        idx = i;
                    }
                }
            }
            colWidths[idx]++;
            deficit--;
        }
        return;
    }

    // ===== 情况4：完全不够，强行均分 =====
    colWidths.resize(cols);
    int avg = targetTotal / cols;
    for (int i = 0; i < cols; i++)
    {
        colWidths[i] = avg;
    }
    int remainder = targetTotal - avg * cols;
    for (int i = 0; i < remainder; i++)
    {
        colWidths[i]++;
    }
}

/**
 * 根据表头和当前页数据生成表格布局。
 *
 * 会先测量各列内容的实际像素宽度，再在当前屏幕可用宽度内
 * 做压缩分配，尽量把空间让给更宽、内容更多的列，
 * 同时避免列宽差距过大。
 *
 * @param cv 目标画布引用
 * @param headers 表头列表
 * @param rows 当前页数据行
 * @param headerSize 表头字号
 * @param bodySize 正文字号
 * @param colXs 输出参数，各列起始 X 坐标
 * @param colWidths 输出参数，各列最终宽度
 * @return 实际参与绘制的列数
 */
int _buildSimpleTableLayout(
    M5Canvas &cv,
    const std::vector<String> &headers,
    const std::vector<std::vector<String>> &rows,
    float headerSize,
    float bodySize,
    std::vector<int> &colXs,
    std::vector<int> &colWidths)
{
    int cols = min((int)headers.size(), 3);
    if (cols <= 0)
    {
        colXs.clear();
        colWidths.clear();
        return 0;
    }

    const int leftPadding = (cols >= 3) ? 6 : 8;
    const int rightPadding = 6;
    const int colGap = 8;
    const int cellPadding = 8; // 单元格内部左右 padding 之和
    const int availableWidth =
        cv.width() - leftPadding - rightPadding - colGap * (cols - 1);

    std::vector<int> minColWidths(cols, 0);
    std::vector<int> contentWidths(cols, 0);
    colWidths.assign(cols, 0);

    // 测量表头
    cv.setTextSize(headerSize);
    for (int c = 0; c < cols; c++)
    {
        contentWidths[c] = (int)cv.textWidth(headers[c]);
    }

    // 测量正文
    cv.setTextSize(bodySize);
    for (const auto &row : rows)
    {
        int rowCols = min((int)row.size(), cols);
        for (int c = 0; c < rowCols; c++)
        {
            contentWidths[c] = max(contentWidths[c], (int)cv.textWidth(row[c]));
        }
    }

    // 最小列宽 = 内容最大像素宽 + 内部 padding
    for (int c = 0; c < cols; c++)
    {
        minColWidths[c] = contentWidths[c] + cellPadding;
    }

    // 测量截断底线：能显示 "..." 的最小列宽
    int ellipsisWidth = (int)cv.textWidth("...");
    int truncateMinWidth = ellipsisWidth + cellPadding;
    int emergencyMinWidth = cellPadding + (int)cv.textWidth("W");

    // 确保全局最小值
    for (int c = 0; c < cols; c++)
    {
        minColWidths[c] = max(minColWidths[c], emergencyMinWidth);
    }

    _rebalanceSimpleTableWidths(availableWidth, minColWidths, colWidths,
                                truncateMinWidth, emergencyMinWidth);

    colXs.assign(cols, leftPadding);
    for (int c = 1; c < cols; c++)
    {
        colXs[c] = colXs[c - 1] + colWidths[c - 1] + colGap;
    }
    return cols;
}

/**
 * 绘制表格表头和表头下方分隔线。
 *
 * @param cv 目标画布引用
 * @param headers 表头列表
 * @param colXs 各列起始 X 坐标
 * @param cols 参与绘制的列数
 * @param startY 表头起始 Y 坐标
 * @param rowHeight 行高
 * @param headerSize 表头字号
 */
void _drawSimpleTableHeader(
    M5Canvas &cv,
    const std::vector<String> &headers,
    const std::vector<int> &colXs,
    const std::vector<int> &colWidths,
    int cols,
    int startY,
    int rowHeight,
    float headerSize)
{
    const int cellPadding = 8;
    cv.setTextColor(TFT_DARKGREY);
    cv.setTextSize(headerSize);
    for (int i = 0; i < cols; i++)
    {
        int maxWidth = max(0, colWidths[i] - cellPadding);
        String headerText = _fitWordTableCellText(cv, headers[i], maxWidth);
        cv.drawString(headerText, colXs[i], startY);
    }

    int lineY = startY + rowHeight - 4;
    cv.drawLine(
        colXs[0],
        lineY,
        cv.width() - colXs[0],
        lineY,
        TFT_DARKGREY);
}

/**
 * 绘制表格正文行。
 *
 * 会按列宽对单元格文本做裁剪，避免正文内容压到下一列。
 *
 * @param cv 目标画布引用
 * @param rows 当前页数据行
 * @param colXs 各列起始 X 坐标
 * @param colWidths 各列最终宽度
 * @param cols 参与绘制的列数
 * @param startY 表头起始 Y 坐标
 * @param rowHeight 行高
 * @param bodySize 正文字号
 */
void _drawSimpleTableRows(
    M5Canvas &cv,
    const std::vector<std::vector<String>> &rows,
    const std::vector<int> &colXs,
    const std::vector<int> &colWidths,
    int cols,
    int startY,
    int rowHeight,
    float bodySize)
{
    const int cellPadding = 8;
    cv.setTextColor(WHITE);
    cv.setTextSize(bodySize);
    int y = startY + rowHeight + 2;
    for (size_t r = 0; r < rows.size(); r++)
    {
        const auto &row = rows[r];
        int rowCols = min((int)row.size(), cols);
        for (int c = 0; c < rowCols; c++)
        {
            int maxWidth = max(0, colWidths[c] - cellPadding);
            String cellText = _fitWordTableCellText(cv, row[c], maxWidth);
            cv.drawString(cellText, colXs[c], y);
        }
        y += rowHeight;
    }
}

/**
 * 将单元格文本裁剪到指定像素宽度内。
 *
 * 若原文过长，则保留前部内容并追加 `...`，避免表格越列。
 * 极端情况下如果连 `...` 都放不下，则直接截断到能容纳的最长前缀。
 */
String _fitWordTableCellText(
    M5Canvas &cv,
    const String &text,
    int maxWidth)
{
    if (text.isEmpty())
    {
        return "";
    }
    if (cv.textWidth(text) <= maxWidth)
    {
        return text;
    }

    // 如果连 "..." 都放不下，直接截断到能放下的最长前缀
    if (cv.textWidth("...") > maxWidth)
    {
        String clipped = text;
        while (clipped.length() > 0)
        {
            removeLastUTF8Char(clipped);
            if (cv.textWidth(clipped) <= maxWidth)
            {
                return clipped;
            }
        }
        return "";
    }

    // 尝试保留前缀 + "..."
    String clipped = text;
    while (clipped.length() > 0)
    {
        removeLastUTF8Char(clipped);
        String candidate = clipped + "...";
        if (cv.textWidth(candidate) <= maxWidth)
        {
            return candidate;
        }
    }
    return "...";
}