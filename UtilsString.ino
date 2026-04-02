/**
 * @file UtilsString.ino
 * @brief 字符串绘制与转换工具函数
 *
 * 提供自适应字号的文本绘制、屏幕角落文本绘制，
 * 以及 IPA 国际音标 Unicode 符号到 ASCII 近似字符的转换功能。
 */

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
                       int x, int y, float baseSize)
{
    if (text.length() == 0)
        return;

    float size = baseSize;
    cv.setTextSize(size);
    int width = cv.textWidth(text);
    int maxWidth = cv.width() - 20;
    float minSize = 0.8;

    while (width > maxWidth && size > minSize)
    {
        size -= 0.1;
        cv.setTextSize(size);
        width = cv.textWidth(text);
    }

    cv.setTextDatum(middle_center);
    cv.drawString(text, x, y);
}

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
void drawTopLeftString(M5Canvas &cv, const String &text, uint16_t color, float size)
{
    cv.setTextDatum(top_left);
    cv.setTextColor(color);
    cv.setTextSize(size);
    cv.drawString(text, 8, 8);
}

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
void drawTopRightString(M5Canvas &cv, const String &text, uint16_t color, float size)
{
    cv.setTextDatum(top_right);
    cv.setTextColor(color);
    cv.setTextSize(size);
    cv.drawString(text, cv.width() - 8, 8);
}

/**
 * 将 IPA 国际音标 Unicode 符号转换为 ASCII 近似表示
 *
 * 逐字节扫描输入字符串，将常见的 IPA 音标 Unicode 字符替换为
 * 对应的 ASCII 近似写法。例如：
 * - ae (U+00E6) -> "ae"
 * - eth (U+00F0) / theta (U+03B8) -> "th"
 * - eng (U+014B) -> "ng"
 * - esh (U+0283) -> "sh"
 * - schwa (U+0259) -> "e"
 * - 长音符号 (U+02D0) -> ":"
 * - 重音符号 (U+02C8) -> "'"
 *
 * ASCII 字符原样保留，无法识别的多字节字符将被跳过。
 *
 * @param s 包含 IPA 音标的 UTF-8 字符串
 * @return 转换后的 ASCII 近似字符串
 */
String asciiPhonetic(const String &s)
{
    String out;
    for (size_t i = 0; i < s.length();)
    {
        uint8_t c = s[i];
        if (c < 0x80) { out += (char)c; i++; continue; }
        if (i + 1 < s.length())
        {
            uint8_t c2 = s[i + 1];
            if (c == 0xC3 && c2 == 0xA6) { out += "ae"; i += 2; continue; }
            if (c == 0xC3 && c2 == 0xB0) { out += "th"; i += 2; continue; }
            if (c == 0xCE && c2 == 0xB8) { out += "th"; i += 2; continue; }
            if (c == 0xC5 && c2 == 0x8B) { out += "ng"; i += 2; continue; }
            if (c == 0xCA && c2 == 0x83) { out += "sh"; i += 2; continue; }
            if (c == 0xCA && c2 == 0x92) { out += "zh"; i += 2; continue; }
            if (c == 0xC9 && c2 == 0x99) { out += "e"; i += 2; continue; }
            if (c == 0xC9 && c2 == 0x9C) { out += "er"; i += 2; continue; }
            if (c == 0xC9 && c2 == 0x9D) { out += "er"; i += 2; continue; }
            if (c == 0xC9 && c2 == 0x9A) { out += "er"; i += 2; continue; }
            if (c == 0xC9 && c2 == 0x94) { out += "o"; i += 2; continue; }
            if (c == 0xC9 && c2 == 0x92) { out += "o"; i += 2; continue; }
            if (c == 0xCA && c2 == 0x8C) { out += "u"; i += 2; continue; }
            if (c == 0xCA && c2 == 0x8A) { out += "u"; i += 2; continue; }
            if (c == 0xC9 && c2 == 0xAA) { out += "i"; i += 2; continue; }
            if (c == 0xCB && c2 == 0x90) { out += ":"; i += 2; continue; }
            if (c == 0xCB && c2 == 0x88) { out += "'"; i += 2; continue; }
            if (c == 0xCB && c2 == 0x8C) { out += ","; i += 2; continue; }
            if (c == 0xC9 && c2 == 0xA1) { out += "g"; i += 2; continue; }
        }
        i++;
    }
    return out;
}


/**
 * 判断字符是否为合法的英语输入字符
 *
 * 合法字符包括：大小写字母、数字、撇号、连字符、下划线和空格。
 * 用于在英语听写模式下过滤键盘输入。
 *
 * @param c 待检测的字符
 * @return 该字符是否为合法的英语输入字符
 */
bool isEnglishInputChar(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '\'' || c == '-' || c == '_' || c == ' ';
}

/**
 * 规范化英语答案字符串，用于答案比对
 *
 * 进行以下处理：去除首尾空白、转为小写、将下划线替换为空格、
 * 合并连续空格为单个空格。确保用户输入与标准答案的格式一致。
 *
 * @param s 原始输入字符串
 * @return 规范化后的字符串
 */
String normalizeEnglishAnswer(String s)
{
    s.trim();
    s.toLowerCase();
    String out = "";
    bool prevSpace = false;
    for (size_t i = 0; i < s.length(); ++i)
    {
        char c = s[i];
        if (c == '_')
            c = ' ';
        if (c == ' ' || c == '\t')
        {
            if (!prevSpace && out.length() > 0)
                out += ' ';
            prevSpace = true;
            continue;
        }
        prevSpace = false;
        out += c;
    }
    out.trim();
    return out;
}

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
void drawCenterString(M5Canvas &cv, const String &message, uint16_t color, float size) {
    cv.fillSprite(BLACK);
    cv.setTextDatum(middle_center);
    cv.setTextColor(color);
    cv.setTextSize(size);
    cv.drawString(message, cv.width() / 2, cv.height() / 2);
    cv.pushSprite(0, 0);
}