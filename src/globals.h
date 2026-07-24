/**
 * globals.h - 全局数据结构、枚举与外部变量声明
 *
 * 基于 M5Cardputer 的便携单词学习机。支持日语和英语词库的闪卡学习、
 * 听写测试、听读练习及学习统计等多种模式。
 */

#pragma once

#include "M5Cardputer.h"
#include "M5GFX.h"
#include "esp_system.h"
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <sqlite3.h>
#include <vector>
#include <time.h>

// ---------- SD 卡引脚 ----------
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

// ---------- 模式定义 ----------
enum AppMode {
    MODE_SPLASH,
    MODE_LANG_SELECT,
    MODE_CLASSIFY_SELECT,
    MODE_FILE_SELECT,
    MODE_SCORE_SELECT,
    MODE_STUDY,
    MODE_ESC_MENU,
    MODE_DICTATION,
    MODE_DICTATION_REVIEW,
    MODE_LISTEN,
    MODE_STATS,
    MODE_WORD_TABLE,
    MODE_WIFI_SCAN,
    MODE_KEY_HELP,
};

enum StudyLanguage {
    LANG_JP,
    LANG_EN
};

// ---------- 数据结构 ----------
struct Word {
    int dbId;
    String jp;
    String zh;
    String kanji;
    String romaji;
    String en;
    String pos;
    String phonetic;
    String sentence;
    String sentenceZh;
    String root;
    String affix;
    int tone;
    int score;
};

struct DictError {
    int wordIndex;
    int wordDbId;
    String wrong;
    String createdAt;
};

struct DictationReviewEntry {
    int wordDbId;
    String correct;
    String wrong;
    String createdAt;
};

struct WiFiCredential {
    String ssid;
    String pass;
};

// ---------- EscMenu ----------
enum EscMenuGroup {
    ESC_MENU_ROOT,
    ESC_MENU_VOCAB,
    ESC_MENU_MODE
};

// ---------- WiFi ----------
enum WiFiScanState {
    WIFI_SCANNING,
    WIFI_LIST,
    WIFI_PASSWORD_INPUT,
    WIFI_CONNECTING,
    WIFI_STATUS
};

// ---------- KeyHelp ----------
struct HelpSectionData {
    const char *section;
    std::vector<std::vector<std::vector<String>>> pages;
};

// ============================================================
// 全局变量 extern 声明（定义见 globals.cpp）
// ============================================================

extern AppMode appMode;
extern AppMode previousMode;

extern M5Canvas canvas;
extern const int visibleLines;
extern int soundVolume;
extern bool wifiConnected;

extern StudyLanguage currentLanguage;
extern String currentWordRoot;
extern String currentAudioRoot;
extern String currentSource;
extern String vocabLabel;

extern std::vector<String> langItems;
extern int langIndex;
extern std::vector<String> classifyItems;
extern int classifyIndex;

extern unsigned long volumeMessageDeadline;

extern bool scoresDirty;
extern int dirtyCount;
extern int autoSaveThreshold;

extern bool userAction;
extern unsigned long lastActivityTime;
extern bool isDimmed;
extern unsigned long idleTimeout;
extern uint8_t normalBrightness;
extern uint8_t dimBrightness;
extern int loopDelay;

extern std::vector<Word> words;
extern int wordIndex;

extern std::vector<DictError> dictErrors;
extern std::vector<DictationReviewEntry> dictationReviewEntries;
extern int dictationReviewIndex;
extern String dictationReviewTitle;

extern std::vector<WiFiCredential> savedWiFiList;

// --- ModeDictation ---
extern std::vector<int> dictOrder;
extern int dictPos;
extern String commitText;
extern String romajiBuffer;
extern String candidateKana;
extern String dictEnInput;
extern int correctCount;
extern int wrongCount;
extern bool dictShowSummary;
extern bool useKatakana;

// --- ModeEscMenu ---
extern std::vector<String> escRootItems;
extern std::vector<String> escVocabItems;
extern std::vector<String> escModeItems;
extern EscMenuGroup escMenuGroup;
extern int escRootIndex;
extern int escRootScroll;
extern int escVocabIndex;
extern int escVocabScroll;
extern int escModeIndex;
extern int escModeScroll;

// --- ModeKeyHelp ---
extern std::vector<HelpSectionData> helpSections;
extern int helpSectionIndex;
extern int helpPageIndex;

// --- ModeListen ---
extern int listenPlayCount;
extern unsigned long listenNextActionTime;
extern const unsigned long listenRepeatInterval;
extern const unsigned long listenNextWordDelay;

// --- ModeScoreSelect ---
extern int scoreLevel;
extern int scoreListIndex;
extern int scoreListScroll;
extern int groupListIndex;
extern int groupListScroll;
extern int selectedScore;
extern int scoreWordCounts[6];

// --- ModeSourceSelect ---
extern std::vector<String> files;
extern std::vector<bool> fileExpandable;
extern String selectedSource;
extern String selectedChapter;
extern int fileIndex;
extern int fileScroll;

// --- ModeStats ---
extern int statsTotal;
extern float statsAvg;
extern float statsMedian;
extern int statsCounts[6];
extern String statsLevel;
extern int statsPage;

// --- ModeStudy ---
extern int studyPage;
extern bool showMeaning;
extern bool showSentenceZh;
extern bool showAnkiSideA;
extern bool showRoots;

// --- ModeWiFiScan ---
extern WiFiScanState wifiScanState;
extern std::vector<String> wifiSSIDs;
extern std::vector<String> wifiRawSSIDs;
extern int wifiListIndex;
extern int wifiListScroll;
extern String wifiSelectedSSID;
extern String wifiPasswordInput;
extern bool wifiConnectSuccess;
extern int wifiPage;

// --- ModeWordTable ---
extern int wordTableScore;
extern int wordTablePage;
extern const int wordTableRowsPerPage;
extern std::vector<int> wordTableFilteredIndices;

// --- UtilsWebServer ---
extern WebServer server;
extern bool webServerRunning;
extern File uploadFile;
extern String uploadTempPath;
extern String uploadTargetSource;
extern String uploadTargetChapter;
extern String uploadError;
extern int uploadImportedCount;

// ============================================================
// 函数声明
// ============================================================

// --- ModeClassifySelect ---
void initClassifySelectMode();
void loopClassifySelectMode();

// --- ModeSourceSelect ---
void initFileSelectMode();
void loopFileSelectMode();

// --- ModeScoreSelect ---
void initScoreSelectMode();
void loopScoreSelectMode();

// --- ModeStudy ---
void initStudyMode();
void loopStudyMode();

// --- ModeEscMenu ---
void initEscMenuMode();
void loopEscMenuMode();

// --- ModeDictation ---
void initDictationMode();
void loopDictationMode();

// --- ModeDictationReview ---
void drawDictationReviewPage();
void initDictationReviewFromSession();
void initDictationReviewHistoryMode();
void loopDictationReviewMode();

// --- ModeListen ---
void initListenMode();
void loopListenMode();

// --- ModeStats ---
void initStatsMode();
void loopStatsMode();

// --- ModeWordTable ---
void initWordTableMode();
void loopWordTableMode();

// --- ModeLangSelect ---
void initLanguageSelectMode();
void loopLanguageSelectMode();

// --- ModeWiFiScan ---
void initWiFiScanMode();
void loopWiFiScanMode();

// --- ModeKeyHelp ---
void initKeyHelpMode();
void loopKeyHelpMode();

// --- ModeSplash ---
void initSplashMode();
void loopSplashMode();

// --- UtilsDb ---
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

// --- UtilsData ---
void autoSaveIfNeeded();
void markScoreDirty();
int pickWeightedRandom();
String dictationPromptText(const Word &w);
void computeStatsFromWords();
void setLanguage(StudyLanguage lang);

// --- UtilsString ---
void drawAutoFitString(M5Canvas &cv, const String &text, int x, int y, float baseSize);
void drawTopLeftString(M5Canvas &cv, const String &text, uint16_t color, float size);
void drawTopRightString(M5Canvas &cv, const String &text, uint16_t color, float size);
void drawCenterString(M5Canvas &cv, const String &message, uint16_t color, float size);
void drawWrappedTextBlock(M5Canvas &cv, const String &text, int left, int top, int maxWidth, float fontSize, int lineGap);
bool isEnglishInputChar(char c);
String normalizeEnglishAnswer(String s);

// --- UtilsIme ---
void commitCandidate();
bool isSokuonConsonant(char c);
String matchRomaji(const String &buffer, bool useKatakana);
void removeLastUTF8Char(String &s);

// --- UtilsMenu ---
void navigateMenu(int &index, int &scroll, int itemCount, int visible, bool moveUp);
void drawTextMenu(M5Canvas &cv, const String &title, const std::vector<String> &items,
                  int selectedIndex, int scrollIndex, int visibleLines,
                  const String &emptyText = "无项目", bool showBattery = true, bool showPager = true);

// --- UtilsTable ---
void drawSimpleTable(M5Canvas &cv, const std::vector<String> &headers,
                     const std::vector<std::vector<String>> &rows);

// --- UtilsAudio ---
bool adjustVolume(char c);
bool playWavStream(const String& path);
void playAudioForWord(const String& jpWord);

// --- UtilsConfig ---
bool saveAppConfig();
void loadAppConfig();

// --- UtilsWiFi ---
String getNtpTimeString();
String rssiIndicator(int rssi);
void processWiFiScanResults(int count);
void attemptWiFiConnect();
void saveWiFiCredential(const String &ssid, const String &pass);
String findSavedPassword(const String &ssid);

// --- UtilsWebServer ---
void initWebServer();
void handleWebServer();
