# UtilsMenu.ino

> 最后更新日期: 2026/07/29

## 作用

`UtilsMenu.ino` 是项目的 **通用 UI 渲染工具库**。提供可滚动菜单等标准化组件，确保语言选择、文件选择、ESC 菜单等视觉风格一致。

> 通用表格绘制功能已迁移至 [UtilsTable.ino](UtilsTable.md)。

## 核心函数

| 函数 | 作用 |
|------|------|
| `navigateMenu(...)` | 处理菜单光标移动与滚动窗口跟随 |
| `drawTextMenu(...)` | 绘制带标题、电量、分页的通用菜单 |

## 关键流程

### 菜单导航

```mermaid
flowchart LR
    A[按 ; 上翻] --> B[index 循环减 1]
    A2[按 . 下翻] --> B2[index 循环加 1]
    B --> C[调整 scroll 使 index 可见]
    B2 --> C
```

### 菜单绘制布局

```
┌─────────────────────────────┐
│ 标题（绿）        电量/WiFi  │
├─────────────────────────────┤
│ > 选中项（黄）                │
│   其他项（白）                │
│   ...                        │
│                    当前/总数  │
└─────────────────────────────┘
```

## 重要细节

### `navigateMenu` 行为

- 光标在首项时按上翻会跳到末项，并自动滚动到底部。
- 光标在末项时按下翻会跳到首项，并滚动到顶部。
- 滚动窗口始终保证 `selectedIndex` 在可见区域内。

### `drawTextMenu` 参数

| 参数 | 说明 |
|------|------|
| `title` | 左上角标题 |
| `items` | 菜单项列表 |
| `selectedIndex` | 当前高亮索引 |
| `scrollIndex` | 第一个可见项索引 |
| `visibleLines` | 一屏最多显示行数（全局为 4） |
| `emptyText` | 列表为空时的提示文字 |
| `showBattery` | 右上角是否显示电量/WiFi |
| `showPager` | 右下角是否显示分页 |

## 使用示例

### 绘制语言选择菜单

```cpp
drawTextMenu(
    canvas,
    "选择语言",
    langItems,
    langIndex,
    0,
    visibleLines,
    "无可选项",
    false,   // 不显示电量
    false    // 不显示分页
);
```

## 注意事项

- `visibleLines` 在 `WordCardputer.ino` 中定义为 4，因此所有菜单默认一屏显示 4 行。
- 电量显示格式为 `WIFI 85%`（WiFi 连接时）或 `85%`（未连接时）。

