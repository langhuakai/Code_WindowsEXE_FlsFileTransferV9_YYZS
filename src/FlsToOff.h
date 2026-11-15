#pragma once

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <core_api/lsscan.h>
#include <core_api/lspointtypes.h>

using namespace SCENE_API;

class FlsToOff
{
public:
    // 成功返回输出路径, 失败返回空串; failReason 非空时回填失败原因
    static std::string transToOff(const ref_ptr<LSScan> scan, const std::string outputPath,
        std::string* failReason = nullptr);

    static bool savePointCloudToOFF(const std::string& filename,
        const std::vector<LSPointXYZRGB>& points,
        std::string* failReason = nullptr);
};
