# Commit Message

生成符合本仓库历史约定的提交信息。

## 格式

**单行**（无 body，无 footer）：

```
[<scope>] <description>
```

历史示例（直接来自 `git log`）：

```
[framework] add gitignore
[cursor] add rules for cursor generated code
[python-net] add websocket server and client
[cpp-video] add mp4 file play with seek function via http,  A/V synchronized
[cpp-camera] add preview via rpicam-vid
[readme] update root README.md
```

> 本约定**不是** Conventional Commits（`feat(scope): ...`）。即便 `.cursor/rules/00-general.mdc` 描述的是 Conventional Commits，本仓库实际使用 `[scope] description` 格式 —— 以本 skill 为准。

## Description 规则

- 英文、小写、动词原形（imperative mood）
- 起始动词：`add` / `update` / `refactor` / `fix` / `remove` / `docs`
- 整行 ≤ 72 字符（含 `[scope] ` 前缀）
- 多项并列用逗号分隔
- 专有名词保持原样：`gstreamer`、`sherpa-onnx`、`bytetrack`、`rpicam-vid`、`qwen2:1.5b`、`A/V synchronized`
- 大写尾注可表达限制：`NO AUDIO PLAY`
- **末尾无句号**
- **不要**泛泛而谈：`update code` / `fix bug` / `change files` 禁止

## Scope 推断

| 变更路径 | Scope |
| :--- | :--- |
| `cpp/<topic>/...` | `cpp-<topic>`（如 `cpp-video`、`cpp-camera`、`cpp-kernel`） |
| `python/<topic>/...` | `python-<topic>`（如 `python-net`、`python-yolo`、`python-camera`、`python-audio`） |
| `c/<topic>/...` | `c-<topic>` |
| `.cursor/...` | `cursor` |
| 根 `README.md` | `readme` |
| `.gitignore`、顶层配置、跨目录重构 | `framework` |
| `docs/...` | `docs` |
| `misc/...` | `misc` |
| 仓库首次提交 | `Initial commit`（无方括号，首字母大写） |

多个 scope 同时变更：

- 同主题下多个子目录 → 取主题（如 `python-audio` 下多个 ASR 实现）
- 跨主题 → 用 `framework`

历史出现过的 scope：`framework`、`cursor`、`python-net`、`cpp-video`、`readme`、`python-yolo`、`python-camera`、`python-audio`、`cpp-kernel`、`cpp-camera`。

## 工作流

1. **查看变更**：

   ```bash
   git status --short
   git diff --stat                 # 未暂存
   git diff --cached --stat        # 已暂存
   ```

2. **如未暂存**，询问用户是否 `git add` 全部或指定文件。

3. **推断 scope**：根据上表规则；不确定时列 1-2 个候选让用户挑。

4. **提炼 description**：阅读 `git diff` 实际内容（一两个关键词 + 动词），不要仅凭文件名泛泛而谈。

5. **输出候选**：给出 1-2 个候选 message。

6. **等待用户确认**后才执行 `git commit -m "..."`。

## 反模式（❌）

- Conventional Commits 格式（`feat(scope): ...`、`fix(scope): ...`）
- 多行 message（带 body / footer）
- 中文 description（subject 必须英文）
- 末尾句号
- Description 首字母大写
- 过去式 / 进行时（`added`、`adding`）
- 模糊词：`update code`、`fix bug`、`change files`

## 示例

输入 diff 摘要：

```text
 python/asr/open_wake_word/     |  8 ++++++++
 python/asr/sherpa_kws/main.py  | 45 +++++++++++++++++++++++++++++++++
```

候选 message：

```text
[python-asr] add open wake word and sherpa-onnx kws
[python-asr] add open wake word, sherpa-onnx key word detection
```

确认后执行：

```bash
git commit -m "[python-asr] add open wake word and sherpa-onnx kws"
```
