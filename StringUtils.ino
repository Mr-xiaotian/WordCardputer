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

void drawTitleString(M5Canvas &cv, const String &text)
{
    cv.setTextDatum(top_left);
    cv.setTextColor(TFT_DARKGREY);
    cv.setTextSize(1.0);
    cv.drawString(text, 8, 8);
}

void drawVolumeString(M5Canvas &cv)
{
    cv.setTextDatum(top_left);
    cv.setTextColor(TFT_DARKGREY);
    cv.setTextSize(1.0);
    cv.drawString(String(soundVolume), cv.width() - 30, 8);
}