# 参与贡献

欢迎提交问题报告和改进建议。

## 修改前

- 说明问题的复现步骤、预期结果和实际结果。
- 算法相关改动请附一个小型课程图或数据文件示例。
- 数据文件使用 UTF-8 编码；课程名和编号不能包含空白字符。

## 本地构建

安装 C++17 编译器、CMake 3.21 或更新版本及 Ninja，然后运行：

```sh
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
```

提交前请确认构建和 CTest 均通过，并检查网页界面在浏览器中能正常加载。CTest 覆盖 C++ 核心模型、算法和文本文件读写；浏览器界面仍需人工检查。
