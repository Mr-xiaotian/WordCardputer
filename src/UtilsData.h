/**
 * UtilsData.h - 运行时词库数据与学习状态管理（声明）
 */
#pragma once
#include "globals.h"

void autoSaveIfNeeded();
void markScoreDirty();
int pickWeightedRandom();
String dictationPromptText(const Word &w);
void computeStatsFromWords();
void setLanguage(StudyLanguage lang);
