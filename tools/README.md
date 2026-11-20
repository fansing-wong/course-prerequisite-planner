# 本机工具

本目录中的 MinGW、CMake、Ninja 和 LibreOffice 文件属于本机开发环境，不纳入 Git 版本控制。

从 GitHub 克隆本项目后，请自行安装 C++17 编译器、CMake 3.21 或更新版本及 Ninja。`setup-env.ps1` 仅用于本机已有的便携 WinLibs 工具链；若未下载该工具链，它会继续使用系统 `PATH` 中的编译器和 CMake。
