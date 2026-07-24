/**
 * UtilsMenu.h - 通用菜单绘制工具
 *
 * 提供可复用的 UI 组件函数，包括带滚动、选中高亮、
 * 电量显示和分页指示器的文本菜单。
 */

#pragma once

#include "globals.h"

/**
 * 菜单光标导航（带滚动和循环翻页）
 *
 * 向上或向下移动菜单光标，自动处理循环翻页和滚动窗口跟随。
 * 从首项上翻到末项、从末项下翻到首项时自动调整滚动位置。
 *
 * @param index 当前选中索引（会被修改）
 * @param scroll 当前滚动偏移（会被修改）
 * @param itemCount 菜单项总数
 * @param visible 一屏可见行数
 * @param moveUp true 向上移动，false 向下移动
 */
void navigateMenu(int &index, int &scroll, int itemCount, int visible, bool moveUp);

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
void drawTextMenu(
    M5Canvas &cv,
    const String &title,
    const std::vector<String> &items,
    int selectedIndex,
    int scrollIndex,
    int visibleLines,
    const String &emptyText = "无项目",
    bool showBattery = true,
    bool showPager = true);
