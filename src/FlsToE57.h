#pragma once

#include <iostream>
#include <string>
#include <core_api/lsproject.h>
#include <core_api/lsscan.h>

using namespace SCENE_API;

class FlsToE57
{
public:
    static std::string transToE57Scan(const ref_ptr<LSScan> scan, const std::string outputPath);

    static std::string transToE57ScanSpatial(const ref_ptr<LSScan> scan, const std::string outputPath);

    static std::string transToE57Project(const ref_ptr<LSProject> project, const std::string outputPath);
};

