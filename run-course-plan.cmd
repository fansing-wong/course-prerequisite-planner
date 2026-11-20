@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PROJECT_ROOT=%~dp0"
set "BUILD_ROOT=%PROJECT_ROOT%"
set "LOCAL_MINGW=%PROJECT_ROOT%tools\winlibs\mingw64\bin"
set "LOCAL_GXX=%LOCAL_MINGW%\g++.exe"
set "COMPILER="

if exist "%LOCAL_GXX%" (
    set "COMPILER=local"
    set "GCC_EXEC_PREFIX=%PROJECT_ROOT%tools\winlibs\mingw64\lib\gcc\"
    set "PATH=%LOCAL_MINGW%;%PATH%"
)

if not defined COMPILER (
    where g++ >nul 2>nul
    if not errorlevel 1 set "COMPILER=g++"
)

if defined COMPILER (
    rem Use a temporary ASCII drive path when possible; this also helps MinGW
    rem resolve files when the project folder contains non-ASCII characters.
    for %%D in (Z Y X W V U T S R Q P O N M L K J I H G F E) do (
        if not defined PROJECT_ALIAS if exist %%D:\course_plan.cpp (
            fc /b "%%D:\course_plan.cpp" "%PROJECT_ROOT%course_plan.cpp" >nul 2>nul
            if not errorlevel 1 set "PROJECT_ALIAS=%%D:"
        )
    )
    if not defined PROJECT_ALIAS (
        for %%D in (Z Y X W V U T S R Q P O N M L K J I H G F E) do (
            if not defined PROJECT_CANDIDATE if not exist %%D:\ set "PROJECT_CANDIDATE=%%D:"
        )
        if defined PROJECT_CANDIDATE (
            subst !PROJECT_CANDIDATE! "%PROJECT_ROOT%" >nul 2>nul
            if not errorlevel 1 (
                set "PROJECT_ALIAS=!PROJECT_CANDIDATE!"
                set "PROJECT_ALIAS_CREATED=1"
            )
        )
    )
    if defined PROJECT_ALIAS set "BUILD_ROOT=!PROJECT_ALIAS!\"
    if /i "!COMPILER!"=="local" (
        set "COMPILER=!BUILD_ROOT!tools\winlibs\mingw64\bin\g++.exe"
        set "GCC_EXEC_PREFIX=!BUILD_ROOT!tools\winlibs\mingw64\lib\gcc\"
        set "PATH=!BUILD_ROOT!tools\winlibs\mingw64\bin;%PATH%"
    )

    pushd "%BUILD_ROOT%" || (
        echo [错误] 无法进入项目目录。
        exit /b 1
    )
    set "PROJECT_PUSHED=1"
    "!COMPILER!" -std=c++17 -Wall -Wextra -pedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 course_plan.cpp -o course_plan.exe
    if errorlevel 1 (
        popd
        if defined PROJECT_ALIAS_CREATED subst !PROJECT_ALIAS! /d >nul 2>nul
        echo [错误] 编译失败。请确认已安装可用的 g++ 编译器。
        exit /b 1
    )
    set "PROGRAM=!BUILD_ROOT!course_plan.exe"
) else if exist "%PROJECT_ROOT%course_plan.exe" (
    set "PROGRAM=%PROJECT_ROOT%course_plan.exe"
) else (
    echo [错误] 未找到 g++，也没有已编译程序。
    echo 请安装 C++17 编译器，再按 README.md 中的步骤构建。
    exit /b 1
)

if not defined BUILD_ROOT set "BUILD_ROOT=%PROJECT_ROOT%"
if not defined PROGRAM set "PROGRAM=%BUILD_ROOT%course_plan.exe"
if not defined PROJECT_PUSHED (
    pushd "%BUILD_ROOT%" || exit /b 1
    set "PROJECT_PUSHED=1"
)

if "%~1"=="" (
    "!PROGRAM!" "data\sparse.txt"
) else (
    "!PROGRAM!" "%~1"
)
set "RUN_RESULT=%ERRORLEVEL%"

if defined PROJECT_PUSHED popd
if defined PROJECT_ALIAS_CREATED subst !PROJECT_ALIAS! /d >nul 2>nul
exit /b %RUN_RESULT%
