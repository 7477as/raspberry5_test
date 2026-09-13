# Cursor 代码生成规范（Rules）

本目录是给 **Cursor 编辑器** 自动加载的代码生成规则，每次 AI 生成 / 修改代码时会按这些规则执行。

## 规则文件清单

| 文件 | 范围 | 说明 |
| :--- | :--- | :--- |
| `00-general.mdc` | `**/*`（alwaysApply） | 仓库结构、命名规范、Git 工作流、Cursor 行为约束 |
| `20-cpp.mdc` | `cpp/**/*.{cpp,h,hpp,sh,cmake}`（alwaysApply） | C++ 强制 CMake + `build_run.sh`、源码组织、CMake 模板 |
| `10-c.mdc` | `c/**/*.{c,h,sh,cmake}`（alwaysApply） | C 语言强制 CMake + `build_run.sh`、无注释自解释、snake_case 命名 |
| `30-python.mdc` | `python/**/*.py`、`requirements.txt`（alwaysApply） | Python 强制 `main.py` + `requirements.txt`、`logging` / `pathlib` |
| `40-readme.mdc` | `**/README.md`（alwaysApply） | README.md 必填章节、命令示例、表格规范 |

> 所有规则都用 `alwaysApply: true` + `globs` 双重触发，确保目标文件被操作时自动加载。

## 关键约束一览

### C++（`20-cpp.mdc`）
- ✅ **必须 CMake**（`>=3.16`、`LANGUAGES C CXX`、C++17、Release 默认）
- ✅ **每个功能目录必须 `build_run.sh`**（`set -euo pipefail`、统一产物到 `build/`、支持 `run/build/clean/rebuild`、依赖检测 + 提示）
- ❌ 禁止裸 `Makefile`、硬编码 `-I/usr/include`、脚本里 `sudo apt install`

### C（`10-c.mdc`）
- ✅ **必须 CMake**（`LANGUAGES C`、C11、`-Wshadow -Wstrict-prototypes`）
- ✅ **函数实现内禁止写注释**——用命名和拆分讲清意图（`verb_object` + 有意义参数）
- ✅ 文件头 / 头文件公共 API 可写注释
- ❌ 禁止匈牙利命名、`process()` / `handle()` 这类模糊动词、`gets()` / 不限长 `strcpy`

### Python（`30-python.mdc`）
- ✅ **`main.py` + `requirements.txt` 双件套**
- ✅ `pathlib` / `logging` / Google docstring / 类型注解
- ❌ 禁止 `print()` 散布调试、`os.system('sudo ...')`、吞错

### README（`40-readme.mdc`）
- ✅ **每个功能目录必须有 README.md**（不是 `.mk`）
- ✅ 必填 6 章节：`功能简介 / 依赖 / 构建与运行 / 关键参数 / 已知问题 / 参考资料`
- ✅ 命令示例使用 `<占位符>`、关键参数用表格

### 通用（`00-general.mdc`）
- ✅ 顶层目录固定：`cpp/ python/ misc/ docs/`
- ✅ Conventional Commits（`feat(cpp/video): ...`）
- ✅ UTF-8 LF、提交前自检、不提交 `build/` 与模型权重

## 新增功能目录的标准动作（Cursor 会自动遵守）

当用户说"新建一个 X 示例"：

1. **C++**：`cp -r cpp/_template cpp/<topic>/<feature>` → 改 `CMakeLists.txt` + `build_run.sh` + `main.cpp` + `README.md` → 提醒 `chmod +x build_run.sh`
2. **C**：`cp -r c/_template c/<topic>/<feature>` → 改 `CMakeLists.txt` + `build_run.sh` + `main.c` + `README.md` → 提醒 `chmod +x build_run.sh`
3. **Python**：`cp -r python/_template python/<topic>/<feature>` → 改 `main.py` + `requirements.txt` + `README.md`

## 如何自定义规则

- 新增规则：新建 `NN-<scope>.mdc`（数字前缀控制加载顺序）
- 修改规则：直接编辑对应 `.mdc`，下个会话自动生效
- 临时关闭：在 Cursor Settings → Rules 里切换

## 与仓库其他文档的关系

- `CONTRIBUTING.md`：面向**人**（贡献者守则）
- `README.md`：面向**使用者**（功能导航）
- **`.cursor/rules/`** ⬅️ 本目录：面向 **Cursor AI**（生成规则）
