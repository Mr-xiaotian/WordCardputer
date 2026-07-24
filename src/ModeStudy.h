/**
 * @file ModeStudy.h
 * @brief 闪卡学习模式 - 函数声明
 */
#pragma once
#include "globals.h"

void initStudyMode();
void loopStudyMode();
void drawStudyMode();

bool studyHasExample(const Word &w);
bool studyHasRootAffix(const Word &w);
int studyPageCount(const Word &w);
void drawStudyWord(const Word &w);
void drawStudySentence(const Word &w);
void drawStudyRootAffix(const Word &w);
void drawEnglishWord(const Word &w);
void drawJapaneseWord(const Word &w);
void drawEnglishSentence(const Word &w);
void drawJapaneseSentence(const Word &w);
