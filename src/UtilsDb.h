/**
 * UtilsDb.h - SQLite 词库访问与导入导出工具（声明）
 */
#pragma once
#include "globals.h"

const char *currentWordTable();
const char *currentSourceTable();
const char *currentDictationErrorTable();
String sqliteColumnText(sqlite3_stmt *stmt, int col);
bool openVocabularyDb(sqlite3 **db);
bool prepareStatement(sqlite3 *db, const String &sql, sqlite3_stmt **stmt);
bool loadWordsBySource(const String &source, const String &chapter);
bool loadWordsByScore(int score, int groupIndex);
void loadScoreCounts(int *counts);
bool saveCurrentWordsToDB();
bool saveWordListToDB(const String &source, const String &chapter, const std::vector<Word> &list);
bool saveDictationErrorsToDB(const std::vector<DictError> &errors);
bool loadDictationReviewEntriesFromDB(std::vector<DictationReviewEntry> &items);
bool loadSourceList(std::vector<String> &items);
bool loadChapterList(const String &source, std::vector<String> &items);
bool sourceHasChapters(const String &source);
bool loadRootAffixNames(const String &idList, const char *table,
                        const char *nameCol,
                        std::vector<std::pair<String, String>> &out);
int normalizeScoreValue(int score);
bool isValidVocabPath(const String &path);
bool parseVocabPath(const String &path, bool &isRoot, String &source, String &chapter);
bool deriveUploadTarget(const String &path, const String &filename, String &source, String &chapter);
bool importJsonFileToDb(const String &jsonPath, const String &source, const String &chapter, int &importedCount, String &error);
