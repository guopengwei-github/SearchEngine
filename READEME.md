# fake everything
本项目为学习目的。主要用于实现 windwos ntfs 系统的快速搜索，类似于 Everything.

## 模块
1. SearchApp
C# 实现，为界面程序
2. SearchConsole
测试SearchEngine程序，已被SearchApp代替，忽略。
3. SearchEngine 
搜索引擎。C++实现
4. SearchEngineCLI
封装了C++与C#之间的交互

## 功能支持
1. windows ntfs
2. 支持了监控文件的变化