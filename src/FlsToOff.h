#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <core_api/lsscan.h>
#include <core_api/lspointtypes.h>

using namespace SCENE_API;

class FlsToOff
{
public:
    static std::string transToOff(const ref_ptr<LSScan> scan, const std::string outputPath);

    static bool savePointCloudToOFF(const std::string& filename,
        const std::vector<LSPointXYZRGB>& points);
};
