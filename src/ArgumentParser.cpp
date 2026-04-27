// ArgumentParser.cpp
#include "ArgumentParser.h"
#include <iostream>
#include <string>

ArgumentParser::Options ArgumentParser::parse(int argc, char* argv[])
{
    Options options;

    // 如果没有参数，显示帮助
    if (argc == 1)
    {
        options.help = true;
        return options;
    }

    // 遍历所有参数
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-i" || arg == "--input")
        {
            if (i + 1 < argc)
            {
                options.inputFile = argv[++i];
            }
            else
            {
                std::cerr << "错误: -i 需要指定输入文件" << std::endl;
                return options;
            }
        }
        else if (arg == "-o" || arg == "--output")
        {
            if (i + 1 < argc)
            {
                options.outputDir = argv[++i];
            }
            else
            {
                std::cerr << "错误: -o 需要指定输出目录" << std::endl;
                return options;
            }
        }
        else if (arg == "-h" || arg == "--help")
        {
            options.help = true;
        }
        else
        {
            std::cerr << "警告: 忽略未知参数: " << arg << std::endl;
        }
    }

    // 验证必要参数
    if (!options.inputFile.empty())
    {
        options.valid = true;
    }
    else
    {
        std::cerr << "错误: 必须指定输入文件 (-i)" << std::endl;
    }

    return options;
}

void ArgumentParser::showHelp(const char* programName)
{
    std::cout << std::endl;
    std::cout << "FARO 文件转换工具" << std::endl;
    std::cout << "================" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: " << programName << " [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -i, --input <文件>   输入 FLS/SCN 文件" << std::endl;
    std::cout << "  -o, --output <文件>  输出文件路径" << std::endl;
    std::cout << "  -h, --help          显示帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << programName << " -i scan.fls -o ./output/test.ply" << std::endl;
    std::cout << std::endl;
}