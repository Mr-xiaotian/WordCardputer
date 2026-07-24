/**
 * @file UtilsTable.h
 * @brief 通用表格绘制工具
 *
 * 提供可复用的表格布局与绘制函数，包括：
 * - 表格列宽测量
 * - 列宽均衡调整（水位线算法 + 截断降级）
 * - 表头与正文绘制
 * - 单元格超宽裁剪
 */

#pragma once

#include "globals.h"

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
    const std::vector<std::vector<String>> &rows);
