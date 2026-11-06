// ArgumentParser.h
#pragma once

#include <string>
#include <vector>

// 支持的输入点云扩展名 (小写, 含点); 以 .fls 结尾的文件夹当成单个 fls 文件处理
inline const std::vector<std::string> INPUT_EXTENSIONS = { ".fls", ".scn", ".e57" };

// 由 INPUT_EXTENSIONS 生成 "fls, scn, e57" 形式的提示文本
inline std::string supportedInputFormatsText()
{
    std::string text;
    for (size_t i = 0; i < INPUT_EXTENSIONS.size(); ++i) {
        if (i > 0)
            text += ", ";
        text += INPUT_EXTENSIONS[i].substr(1);
    }
    return text;
}

class ArgumentParser
{
public:
    // 参数结构
    struct Options
    {
        std::string inputFile;     // 输入文件或目录
        std::string outputPath;    // 输出文件或目录
        std::string format = "ply"; // 输出格式
        bool outputIsDir = false;  // 输出路径是否为目录
        bool withImage = false;    // 是否额外输出 png 图片

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