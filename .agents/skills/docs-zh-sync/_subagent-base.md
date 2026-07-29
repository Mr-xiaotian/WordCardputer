# Subagent Base Rules（WordCardputer 项目专属）

> 本文件补充通用框架的 `_subagent-base.md`，定义 WordCardputer 项目专属的路径映射和特殊规则。

## 必读文件清单

子代理开始工作前，请按顺序阅读：

1. `~/.agents/skills/docs-zh-sync/_subagent-base.md`（通用规则、输出格式）
2. `~/.agents/skills/docs-zh-sync/_subagent-audit.md`（通用审计清单）
3. `~/.agents/skills/docs-zh-sync/_subagent-writing.md`（通用写作规范）
4. `.agents/skills/docs-zh-sync/_subagent-base.md`（本文件——项目专属路径映射）
5. 主 agent 指定的区域 `subagent-*.md`（区域特化提示）

## 路径映射规则

本项目使用**扁平映射**（文档直接放在 `docs/zh-CN/` 下，不走 `src/` 子目录）。

### 源文件 → 文档映射

| 源文件模式 | 目标文档路径 |
|-----------|-------------|
| `src/ModeXxx.cpp/.h` | `docs/zh-CN/ModeXxx.md` |
| `src/UtilsXxx.cpp/.h` | `docs/zh-CN/UtilsXxx.md` |
| `src/globals.cpp/.h` + `src/main.cpp` | `docs/zh-CN/WordCardputer.md` |
| `utils/*.py` | `docs/zh-CN/PythonTools.md` |

### 后缀映射

| 代码后缀 | 文档后缀 |
|:-------:|:-------:|
| `.cpp` + `.h` (成对) | `.md` |

## 文档标题规范

文档内使用 `# ModeXxx.ino` 或 `# UtilsXxx.ino` 风格标题（历史遗留原因，.ino 为 Arduino 旧后缀）。实际源码现在使用 `.cpp`/`.h`。

## 文档日期

每篇文档使用以下格式标注最后更新日期：

```markdown
> 最后更新日期: YYYY/MM/DD
```

只在文档内容实际变更时更新日期。

## 文档骨架

参考已有文档风格，每篇 Mode/Utils 文档应包含：

1. **作用** — 一句话说明模块职责
2. **核心对象 / 核心函数** — 关键变量、结构体、函数列表（表格化）
3. **关键流程** — Mermaid 流程图展示主要逻辑
4. **重要细节** — 算法、边界情况、配置参数
5. **使用示例** — 代码片段或用户操作示例（C++ 代码用 ```cpp）
6. **注意事项** — 常见误区、依赖关系

## 特殊规则

### C++ 头文件处理

每个模块通常由同名 `.cpp` 和 `.h` 组成。审计时以 `.cpp` 为主要事实来源，`.h` 用于确认函数签名和公开接口。

### globals 相关

`src/globals.h` 定义了全局枚举、结构体和 extern 声明。在审计任何文档时，如遇到枚举值、结构体字段、全局变量变化，需对照 `globals.h` 进行校验。

### 枚举值同步

如果 `AppMode` 等枚举新增了值，检查以下文档是否需同步：
- `docs/zh-CN/WordCardputer.md` 中的 AppMode 列表
- 对应的 ModeXxx.md 中引用枚举值的地方

### 数据库相关

文档中涉及 SQLite 的描述需遵循项目规则：
- journal_mode = **DELETE**（不是 WAL）
- 数据库文件位于 SD 卡 `/words_study/<lang>/` 下
