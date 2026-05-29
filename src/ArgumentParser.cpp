// ArgumentParser.cpp
#include "ArgumentParser.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

static const std::vector<std::string> FORMAT_NAMES = {
    "dxf", "e57", "fls", "igs", "pod", "pts", "ptx",
    "wrl", "xyb", "xyz", "cpe", "bmp", "jpg", "png",
    "tif", "las", "ply"
};

static const std::vector<std::string> FORMAT_EXTENSIONS = {
    ".dxf", ".e57", ".fls", ".igs", ".pod", ".pts", ".ptx",
    ".wrl", ".xyb", ".xyz", ".cpe", ".bmp", ".jpg", ".png",
    ".tif", ".las", ".ply"
};

static bool isKnownExtension(const std::string& ext)
{
    for (const auto& known : FORMAT_EXTENSIONS) {
        if (ext == known) return true;
    }
    return false;
}

static bool isValidFormat(const std::string& fmt)
{
    std::string lower = fmt;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const auto& name : FORMAT_NAMES) {
        if (lower == name) return true;
    }
    return false;
}

ArgumentParser::Options ArgumentParser::parse(int argc, char* argv[])
{
    Options options;

    if (argc == 1)
    {
        options.help = true;
        return options;
    }

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
                std::cerr << "错误: -i 需要指定输入文件或目录" << std::endl;
                return options;
            }
        }
        else if (arg == "-o" || arg == "--output")
        {
            if (i + 1 < argc)
            {
                options.outputPath = argv[++i];
            }
            else
            {
                std::cerr << "错误: -o 需要指定输出文件或目录" << std::endl;
                return options;
            }
        }
        else if (arg == "--format" || arg == "-f")
        {
            if (i + 1 < argc)
            {
                std::string fmt = argv[++i];
                std::string lower = fmt;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (!isValidFormat(lower))
                {
                    std::cerr << "错误: 不支持的格式 '" << fmt << "'" << std::endl;
                    return options;
                }
                options.format = lower;
            }
            else
            {
                std::cerr << "错误: --format 需要指定格式" << std::endl;
                return options;
            }
        }
        else if (arg == "-h" || arg == "--help")
        {
            options.help = true;
        }
        else
        {
            std::cerr << "错误: 存在未知参数: " << arg << std::endl;
        }
    }

    if (options.inputFile.empty())
    {
        std::cerr << "错误: 必须指定输入文件或目录 (-i)" << std::endl;
        return options;
    }

    if (!options.outputPath.empty())
    {
        std::string ext = fs::path(options.outputPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        options.outputIsDir = !isKnownExtension(ext);
    }
    else
    {
        options.outputPath = ".";
        options.outputIsDir = true;
    }

    options.valid = true;
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
    std::cout << "  -i, --input <路径>   输入 FLS/SCN 文件或目录" << std::endl;
    std::cout << "  -o, --output <路径>  输出文件或目录" << std::endl;
    std::cout << "                       (指定扩展名时为单文件模式," << std::endl;
    std::cout << "                        否则为目录批处理模式)" << std::endl;
    std::cout << "  -f, --format <格式>  输出格式 (默认: ply)" << std::endl;
    std::cout << "       支持的格式:" << std::endl;
    std::cout << "        点云: dxf e57 fls igs las ply pod pts ptx wrl xyb xyz cpe" << std::endl;
    std::cout << "        图像: bmp jpg png tif" << std::endl;
    std::cout << "  -h, --help          显示帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  单文件模式: " << programName << " -i scan.fls -o output.las" << std::endl;
    std::cout << "  目录批处理: " << programName << " -i ./scans/ -o ./output/ -f las" << std::endl;
    std::cout << std::endl;
}
