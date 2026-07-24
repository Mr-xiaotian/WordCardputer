/**
 * @file ModeWordTable.h
 * @brief 当前范围按分数词表模式 - 函数声明
 */
#pragma once
#include "globals.h"

void initWordTableMode();
void loopWordTableMode();
void drawWordTablePage();

void rebuildWordTableFilteredIndices();
int wordTableTotalPages();
void switchWordTableScore(int nextScore);
void stepWordTableScore(int delta);
void moveWordTableToFirstNonEmptyScore();
