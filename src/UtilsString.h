/**
 * @file UtilsString.h
 * @brief 字符串绘制与转换工具函数
 *
 * 提供自适应字号的文本绘制、屏幕角落文本绘制，
 * 以及 IPA 国际音标 Unicode 符号到 ASCII 近似字符的转换功能。
 */

#pragma once

#include "globals.h"

/**
 * 自适应字号绘制文本
 *
 * 以指定的基准字号开始尝试绘制文本，如果文本宽度超出画布可用宽度（画布宽度 - 20px），
 * 则逐步缩小字号（每次减 0.1），直到文本适配宽度或达到最小字号 0.8。
 * 文本以居中对齐方式绘制在指定坐标处。
 *
 * @param cv 目标画布引用
 * @param text 要绘制的文本内容
 * @param x 绘制中心点的 X 坐标
 * @param y 绘制中心点的 Y 坐标
 * @param baseSize 初始字号大小
 */
void drawAutoFitString(M5Canvas &cv, const String &text,
                       int x, int y, float baseSize);

/**
 * 在画布左上角绘制文本
 *
 * 在画布左上角 (8, 8) 位置绘制指定颜色和字号的文本，
 * 常用于显示页面标题或状态标签。
 *
 * @param cv 目标画布引用
 * @param text 要绘制的文本内容
 * @param color 文本颜色
 * @param size 字号大小
 */
void drawTopLeftString(M5Canvas &cv, const String &text, uint16_t color, float size);

/**
 * 在画布右上角绘制文本
 *
 * 在画布右上角（右边距 8px，顶部 8px）位置绘制指定颜色和字号的文本，
 * 常用于显示页码、计数器或电量等辅助信息。
 *
 * @param cv 目标画布引用
 * @param text 要绘制的文本内容
 * @param color 文本颜色
 * @param size 字号大小
 */
void drawTopRightString(M5Canvas &cv, const String &text, uint16_t color, float size);

/**
 * 在屏幕中央绘制提示信息
 *
 * 清空画布后居中显示消息文本，适用于各模式中的错误/成功/状态提示。
 *
 * @param cv 目标画布引用
 * @param message 要显示的提示文本
 * @param color 文字颜色，默认红色
 * @param size 文字大小，默认 1.2
 */
void drawCenterString(M5Canvas &cv, const String &message, uint16_t color, float size);

/**
 * 在限定区域内按宽度自动换行绘制文本。
 *
 * @param cv 目标画布引用
 * @param text 要显示的文本
 * @param left 文本块左边界
 * @param top 文本块上边界
 * @param maxWidth 文本块最大宽度
 * @param fontSize 字号大小
 * @param lineGap 行间距
 */
void drawWrappedTextBlock(
    M5Canvas &cv,
    const String &text,
    int left,
    int top,
    int maxWidth,
    float fontSize,
    int lineGap);

/**
 * 判断字符是否为合法的英语输入字符
 *
 * 合法字符包括：大小写字母、数字、撇号、连字符、下划线和空格。
 * 用于在英语听写模式下过滤键盘输入。
 *
 * @param c 待检测的字符
 * @return 该字符是否为合法的英语输入字符
 */
bool isEnglishInputChar(char c);

/**
 * 规范化英语答案字符串，用于答案比对
 *
 * 进行以下处理：去除首尾空白、转为小写、将下划线替换为空格、
 * 合并连续空格为单个空格。确保用户输入与标准答案的格式一致。
 *
 * @param s 原始输入字符串
 * @return 规范化后的字符串
 */
String normalizeEnglishAnswer(String s);

/**
 * 将 IPA 音标字符串中的 Unicode 符号转换为 ASCII 近似表示
 *
 * 例如 æ→ae, ʃ→sh, ɪ→i 等，便于在无 Unicode 支持时显示。
 *
 * @param s 原始 IPA 音标字符串
 * @return ASCII 近似字符串
 */
String asciiPhonetic(const String &s);
