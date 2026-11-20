# Course Prerequisite Planner

课程先修关系与拓扑排序教学计划系统。This project demonstrates an AOV graph, topological sorting, cycle reporting, and semester planning in C++17, with a standalone Chinese web interface.

## 功能

- 用有向边 `A -> B` 表示课程 A 是课程 B 的直接先修课。
- 用邻接表维护课程图；Kahn 算法输出合法修读顺序。
- 当图中有环时，用 DFS 找出并显示一条闭合冲突路径。
- 按先修层次生成学期计划，并可设置每学期学分上限（`0` 表示不限制）。学分上限采用贪心安排，不保证总学期数最少。
- 维护课程和先修约束；命令行程序支持文本文件读取与保存。
- 网页版提供课程关系图、搜索、增删改、文件导入导出和浏览器本地保存。网页无需服务器或第三方 JavaScript 库。

## 快速开始：网页

用浏览器打开 [`index.html`](index.html)。选择示例图即可查看拓扑顺序、冲突环和学期计划。课程及约束编辑会保存在当前浏览器；使用“导出”可下载数据文件备份。

## 构建和运行：C++17

### 环境

- C++17 编译器：GCC、Clang 或 MSVC。
- 使用 CMake 预设时需要 CMake 3.21 或更新版本及 Ninja。
- 仓库不包含编译器、构建目录或预编译程序；请安装本机工具链后构建。

### CMake（推荐）

```powershell
cmake --preset default
cmake --build --preset default
```

运行稀疏图样例：

```powershell
./build/course_plan.exe ./data/sparse.txt
```

在 Linux 或 macOS 上，程序通常位于 `build/course_plan`：

```sh
./build/course_plan ./data/sparse.txt
```

也可以从项目目录直接编译。Windows 下建议使用 PowerShell：

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 .\course_plan.cpp -o .\course_plan.exe
```

项目附带的 [`run-course-plan.cmd`](run-course-plan.cmd) 会尝试用系统 `g++` 编译后运行；如果没有 `g++`，请先安装 C++ 工具链或按上面的 CMake 步骤构建。

### 示例数据

| 文件 | 内容 |
| --- | --- |
| `data/no_constraints.txt` | 无先修边，课程可在第一学期并行安排 |
| `data/chain.txt` | 四门课程形成一条先修链 |
| `data/sparse.txt` | 稀疏 DAG，含独立课程与多条依赖 |
| `data/dense_dag.txt` | 稠密 DAG，每门后续课程依赖此前课程 |
| `data/cycle.txt` | 有向环 `C02 → C03 → C04 → C02`，用于观察冲突报告 |

启动时可传入任一样例文件，例如 `./build/course_plan.exe data/cycle.txt`。不传路径时，C++ 程序默认读取 `data/chain.txt`；批处理启动器默认读取 `data/sparse.txt`。

## 数据格式

文本采用 UTF-8 编码，空行和以 `#` 开头的注释行会被忽略。课程名与编号在 C++ 文件格式中均不能包含空白字符：

```text
COURSE C01 程序设计 4
COURSE C02 数据结构 4
EDGE C01 C02
```

`COURSE` 行依次为类型、编号、名称和非负整数学分。`EDGE` 行依次为类型、先修课程编号和后续课程编号。网页导出时会将课程名中的空格写成下划线，导入后还原为空格。

## 算法与复杂度

- 邻接表保存实际存在的先修边，空间复杂度为 `O(V + E)`。
- Kahn 拓扑排序和 DFS 冲突环搜索均为 `O(V + E)` 时间；算法辅助空间为 `O(V)`。
- 课程编号查找当前为 `O(V)`。
- 学期计划按 Kahn 层次逐轮安排。加入学分上限后会推迟未能放入本学期的课程，但不会搜索全局最优的学分组合。

## 仓库结构

```text
course_plan.cpp             C++17 命令行程序
index.html                  无依赖中文网页界面
data/                       五类课程图样例
CMakeLists.txt              CMake 构建定义
CMakePresets.json           默认 Ninja 预设
.github/workflows/build.yml Linux 与 Windows 构建工作流
```

本机下载的编译器、安装包、生成的 `.exe`、构建目录和课程报告不会提交到 GitHub。

## 许可证

仓库当前尚未指定开源许可证。发布为公开仓库前，请确认是否希望允许他人复制、修改和再发布，并据此添加许可证文件。
