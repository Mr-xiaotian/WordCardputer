/**
 * UtilsIme.h - 日语输入法（IME）工具函数（声明）
 */
#pragma once
#include "globals.h"

void commitCandidate();
bool isSokuonConsonant(char c);
String matchRomaji(const String &buffer, bool useKatakana);
void removeLastUTF8Char(String &s);
