# 课程先修关系与教学计划系统

一个用于演示 **AOV 网、拓扑排序、环路检测和学期计划生成** 的数据结构与算法课程设计项目。项目包含 C++17 命令行程序和一个无需服务器、可直接在浏览器打开的中文界面。

## 项目亮点

- 用邻接表表示课程先修关系：边 `A → B` 表示完成课程 A 后才能修读课程 B。
- 使用 Kahn 算法生成合法修读顺序；发现环路时，用 DFS 给出一条可定位的冲突路径。
- 按先修层次生成学期计划，可设置每学期学分上限，并标明无法安排的课程。
- 支持课程和先修约束的增删改、文本数据导入导出，以及五类可复现示例。
- C++ 与网页共用 `COURSE` / `EDGE` 文本数据格式；网页导出的 UTF-8 BOM 文件也可由 C++ 程序读取。
- GitHub Actions 在 Windows 和 Ubuntu 上构建 C++ 程序。

## 快速体验

### 浏览器界面

直接用浏览器打开 [`index.html`](index.html)，选择示例后即可查看课程图、合法顺序、环路和学期计划。编辑内容保存在**当前浏览器的本地存储**中；需要备份或换设备时，请使用“导出”。本项目没有服务器或账号同步功能。

推荐演示顺序见 [`docs/DEMO_GUIDE.md`](docs/DEMO_GUIDE.md)。

### C++ 命令行程序

环境：C++17 编译器；使用 CMake 预设时还需要 CMake 3.21+ 和 Ninja。

```powershell
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
./build/course_plan.exe ./data/sparse.txt
```

Linux / macOS：

```sh
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
./build/course_plan ./data/sparse.txt
```

也可在项目根目录使用 GCC 直接编译：

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 .\course_plan.cpp .\course_plan_core.cpp -o .\course_plan.exe
```

若启动时没有指定数据文件，C++ 程序默认读取 `data/chain.txt`；Windows 批处理启动器默认读取 `data/sparse.txt`。

## 演示数据

| 文件 | 演示内容 |
| --- | --- |
| `data/no_constraints.txt` | 没有先修边，所有课程都可从第一学期开始 |
| `data/chain.txt` | 四门课程组成先修链，展示逐层开课 |
| `data/sparse.txt` | 稀疏 DAG，包含独立课程和多个可并行课程 |
| `data/dense_dag.txt` | 稠密 DAG，展示多条依赖同时约束修读顺序 |
| `data/cycle.txt` | `C02 → C03 → C04 → C02`，展示环路定位与冲突说明 |

## 算法设计

| 功能 | 方法 | 复杂度 |
| --- | --- | --- |
| 合法修读顺序 | Kahn 拓扑排序 | `O(V + E)` |
| 冲突环路路径 | 在 Kahn 未处理的子图上进行 DFS | `O(V + E)` |
| 课程依赖存储 | 邻接表 | `O(V + E)` 空间 |
| 课程编号查找 | 当前为顺序查找 | `O(V)` |

其中 `V` 是课程数，`E` 是先修关系数。没有学分上限时，同一拓扑层的课程可以并行安排。设置学分上限后，程序按输入顺序贪心放入课程，**不保证学期数最少或总学分组合最优**。

## 数据文件格式

使用 UTF-8 文本；空行和以 `#` 开头的注释行会被忽略。每条课程记录包含编号、名称和非负整数学分，先修边引用课程编号：

```text
COURSE C01 程序设计 4
COURSE C02 数据结构 4
EDGE C01 C02
```

为兼容简洁文本格式，C++ 文件中的课程编号和名称不能含空白字符。网页导出会将名称中的空格写成下划线；若名称本身含下划线，当前格式不能无损区分二者。

## 项目结构

```text
course_plan.cpp             C++17 命令行交互界面
course_plan_core.hpp/.cpp   可复用的课程图模型、算法和文件读写
tests/                      无第三方依赖的 CTest 测试程序
index.html                  独立中文网页演示界面
data/                       无约束、链式、稀疏、稠密和有环示例
docs/DEMO_GUIDE.md          演示步骤、算法说明和答辩提示
CMakeLists.txt              CMake 构建定义
CMakePresets.json           默认 Ninja 预设
.github/workflows/build.yml Windows 与 Ubuntu 构建及测试工作流
```

## 当前范围

这是一个**课程设计演示和个人作品集项目**，不是面向学校生产环境的排课系统。网页数据只保存在本地浏览器；项目没有账号、服务端数据库、权限管理、审计日志或学校排课规则引擎。CTest 覆盖 C++ 核心模型、算法和文本文件读写；浏览器界面目前仍需人工演示检查。

仓库暂未指定许可证。商业或第三方复用前，请先确认代码权利和贡献者授权，并选择适用的许可证或商业授权方式。
