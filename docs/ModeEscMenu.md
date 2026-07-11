# ModeEscMenu.ino

> 最后更新日期: 2026/07/11

## 作用

`ModeEscMenu.ino` 实现应用程序的 **全局 ESC 菜单**。用户在学习、听读、听写等模式中按 `` ` ``（ESC）键即可呼出，提供切换模式、重新选择词源/语言、查看统计、查看过往错题、连接 WiFi 等入口。

## 核心对象

| 对象 | 类型 | 说明 |
|------|------|------|
| `escItems` | `std::vector<String>` | 9 个菜单项 |
| `escIndex` | `int` | 当前选中索引 |
| `escScoll` | `int` | 滚动偏移 |
| `previousMode` | `AppMode` | 进入菜单前的模式，退出时恢复 |

## 菜单项索引

| 索引 | 菜单项 | 动作 |
|------|--------|------|
| 0 | 学习统计 | 进入 `MODE_STATS` |
| 1 | 重新选择词源 | 自动保存后进入 `MODE_FILE_SELECT` |
| 2 | 重新选择语言 | 自动保存后进入 `MODE_LANG_SELECT` |
| 3 | 进入学习模式 | 进入 `MODE_STUDY`，重绘当前闪卡 |
| 4 | 进入听读模式 | 进入 `MODE_LISTEN` |
| 5 | 进入听写模式 | 进入 `MODE_DICTATION` |
| 6 | 查看过往错题 | 进入 `MODE_DICTATION_REVIEW`，从数据库加载历史错题 |
| 7 | 按键帮助 | 进入 `MODE_KEY_HELP` |
| 8 | WiFi 连接 | 进入 `MODE_WIFI_SCAN` |

## 关键流程

```mermaid
flowchart TD
    A[按 ESC] --> B[previousMode = 当前模式]
    B --> C[initEscMenuMode]
    C --> D{用户按键}
    D -->|`| E[autoSaveIfNeeded]
    E --> F{恢复 previousMode}
    F -->|STUDY| F1[drawStudyWord]
    F -->|LISTEN| F2[initListenMode]
    F -->|DICTATION| F3[initDictationMode]
    F -->|DICTATION_REVIEW| F4[drawDictationReviewPage]
    D -->|; / .| G[navigateMenu]
    D -->|Enter| H{按索引分发}
    H -->|0| I[MODE_STATS]
    H -->|1/2| J[autoSaveIfNeeded + 切换]
    H -->|3| K[drawStudyWord]
    H -->|4| L[initListenMode]
    H -->|5| M[initDictationMode]
    H -->|6| N[initDictationReviewHistoryMode]
    H -->|7| O[MODE_KEY_HELP]
    H -->|8| P[MODE_WIFI_SCAN]
```

## 重要细节

- **退出菜单**：再次按 `` ` `` 时，会先触发 `autoSaveIfNeeded()`，然后根据 `previousMode` 选择恢复策略：
  - 学习模式：直接重绘当前闪卡（`drawStudyWord()`）。
  - 听读/听写：重新初始化该模式（避免状态错乱）。
  - 错题回顾：重绘回顾页面。
- **切换词源/语言**：在跳转前自动保存当前词库进度，防止丢失。
- **历史错题**：菜单项 6 调用 `initDictationReviewHistoryMode()`，从 SQLite 数据库加载错题记录。

## 使用示例

### 查看历史错题

1. 学习模式中按 `` ` `` 呼出菜单。
2. 按 `.` 高亮"查看过往错题"，按 Enter。
3. 左/右翻页浏览历史错误记录，按 Fn 重播语音。
4. 按 `` ` `` 返回菜单，再按 `` ` `` 回到学习。

### 切换模式

1. 听写模式中按 `` ` `` 呼出菜单。
2. 按 `.` 高亮"进入听读模式"，按 Enter 开始听读。
3. 听读中再按 `` ` `` 可返回菜单。

## 注意事项

- `escScoll` 变量命名故意简写，实际作用与 `fileScroll` 等一致。
- 从菜单进入学习模式时不会重新初始化 `wordIndex`，因此会回到之前学习的单词；进入听读/听写会重新初始化。
- 菜单列表超过 4 项时，`drawTextMenu()` 会自动分页显示。
- 菜单不再包含独立的"保存进度"选项；自动保存机制通过 `markScoreDirty()` / `autoSaveIfNeeded()` 在切换词源/语言/退出时自动触发。
