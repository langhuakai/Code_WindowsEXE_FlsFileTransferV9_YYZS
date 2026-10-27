#pragma once

#define NOMINMAX
#include <windows.h>
#include <string>
#include <core_api/lsscan.h>
#include <core_api/lspointtypes.h>

using namespace SCENE_API;

class FlsToImage
{
public:
    // 导出 png 图片。outputPath 为点云输出路径, 图片按同名不同扩展名生成
    // (例如 scan.ply -> scan.png), 返回实际写出的图片路径
    static std::string exportPng(const ref_ptr<LSScan> scan, const std::string& outputPath);
};