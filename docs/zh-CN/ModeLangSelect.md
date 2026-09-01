# ModeLangSelect.ino

> 最后更新日期: 2026/09/01

## 作用

`ModeLangSelect.ino` 实现设备启动后的**语言选择模式**。用户通过方向键在"日语"与"英语"之间切换并按 Enter 确认，系统随后绑定对应的词库根目录与音频根目录，并进入分类选择模式。

## 核心对象

| 对象 | 类型 | 说明 |
|------|------|------|
| `langItems` | `std::vector<String>` | 菜单项：`{"日语", "英语"}` |
| `langIndex` | `int` | 当前高亮索引 |

## 关键流程

```mermaid
flowchart LR
    A[启动 / ESC 菜单] --> B[显示语言菜单]
    B --> C{用户按键}
    C -->|; / .| D[上下移动光标]
    C -->|Enter| E{检查词库数据库}
    E -->|数据库不存在| F[提示并返回]
    E -->|数据库存在| G[setLanguage + saveAppConfig]
    G --> H[进入 MODE_CLASSIFY_SELECT]
```

## 重要细节

- **路径绑定**：确认选择后调用 `setLanguage()`，该函数在 [UtilsData.md](UtilsData.md) 中实现，会同步更新：
  - `currentWordRoot`
  - `currentAudioRoot`
  - `currentSource`
  - 并清空 `words`。
- **数据库校验**：Enter 确认前会先检查对应的 SQLite 数据库文件（`/words_study/jp/jp_words.db` 或 `/words_study/en/en_words.db`）是否存在；不存在时屏幕提示"未找到词库数据库"并保持在语言选择界面。
- **保存配置**：确认选择后调用 `saveAppConfig()` 持久化语言设置。
- **下一流程**：语言确认后进入 `MODE_CLASSIFY_SELECT`（分类选择），再由用户选择按词源或按 Score 分类浏览词库。
- **菜单渲染**：直接复用 `drawTextMenu()`，不显示电量与分页指示器（`showBattery=false, showPager=false`）。

## 使用示例

### 用户操作流程

1. 开机后看到 `> 日语` 与 `英语`。
2. 按 `.` 切换到 `> 英语`（按 `;` 回到日语）。
3. 按 Enter 确认。
4. 若数据库 `/words_study/en/en_words.db` 存在，进入分类选择界面。

## 注意事项

- 旧文档中 `langItems` 被描述为带英文括号版本（如 "日语 (Japanese)"），实际代码只使用中文 `{"日语", "英语"}`。
- 切换语言会清空已加载的 `words`，因此只有在启动时或从 ESC 菜单"重新选择语言"才会执行；学习过程中切换语言前会先触发 `autoSaveIfNeeded()`。
- 从 ESC 菜单进入时，会自动调用 `autoSaveIfNeeded()` 保存当前进度。
