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

void drawTopLeftString(M5Canvas &cv, const String &text)
{
    cv.setTextDatum(top_left);
    cv.setTextColor(TFT_DARKGREY);
    cv.setTextSize(1.0);
    cv.drawString(text, 8, 5);
}

void drawTopRightString(M5Canvas &cv, const String &text)
{
    cv.setTextDatum(top_right);
    cv.setTextColor(TFT_DARKGREY);
    cv.setTextSize(1.0);
    cv.drawString(text, cv.width() - 8, 5);
}

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
