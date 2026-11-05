#include "FlsToImage.h"
#include "StringConverter.h"

#include <filesystem>
#include <iostream>
#include <core_api/lsstring.h>
#include <core_api/lsresult.h>

namespace fs = std::filesystem;

std::string FlsToImage::exportPng(const ref_ptr<LSScan> scan, const std::string& outputPath)
{
    // 图片与点云同名不同扩展名: out\a.ply -> out\a.png
    fs::path pngPath(outputPath);
    pngPath.replace_extension(".png");
    std::string pngStr = pngPath.string();

    // 图片导出需要颜色数据, 点云导出用的可能是反射强度数据, 这里补一次加载
    if (!scan->dataLoaded(LSScan::COLOR) && scan->dataAvailable(LSScan::COLOR))
    {
        LSResult::Result loadResult = scan->loadData(LSScan::COLOR);
        if (loadResult != LSResult::Ok)
        {
            std::cerr << "Warning: failed to load color data for image export. Error code: "
                      << loadResult << std::endl;
        }
    }

    try {
        fs::path parent = pngPath.parent_path();
        if (!parent.empty() && !fs::exists(parent)) {
            fs::create_directories(parent);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create image output directory: " << e.what() << std::endl;
        return std::string();
    }

    std::cout << "Exporting png image to " << StringConverter::ansiToUtf8(pngStr) << "..." << std::endl;

    LSResult::Result result = scan->exportData(
        LSString::fromCharStr(pngStr.c_str()), LSScan::PNG);
    if (result != LSResult::Ok)
    {
        std::cerr << "Failed to export png image. Error code: " << result << std::endl;
        return std::string();
    }

    std::cout << "Image completed: " << StringConverter::ansiToUtf8(pngStr) << std::endl;
    return pngStr;
}