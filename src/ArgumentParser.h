// ArgumentParser.h
#pragma once

#include <string>

class ArgumentParser
{
public:
    // 参数结构
    struct Options
    {
        std::string inputFile;     // 输入文件
        std::string outputDir;     // 输出目录

        bool help = false;         // 是否显示帮助
        bool valid = false;        // 参数是否有效
    };

public:
    ArgumentParser() = default;

    // 解析命令行参数
    Options parse(int argc, char* argv[]);

    // 显示帮助信息
    static void showHelp(const char* programName);
};