// FlsToPly.h
#pragma once

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <core_api/lsproject.h>
#include <core_api/lsscan.h>

using namespace SCENE_API;

class FlsToPly
{
public:
    // 将fls转换为rgb颜色文件
    static std::string transToPlyRgb(const ref_ptr<LSScan> scan, const std::string outputPath);

    // 将fls转换为带法向量的点云文件
    static std::string transToPlyNormals(const ref_ptr<LSScan> scan, const std::string outputPath);

    // 将fls转换为带反射强度的点云文件
    static std::string transToPlyIntensity(const ref_ptr<LSScan> scan, const std::string outputPath);

    // 将fls转换为球坐标文件
    static std::string transToPlySpherical(const ref_ptr<LSScan> scan, const std::string outputPath);

    // 将项目fls转换为项目点云文件
    static std::string transToPlyProject(const ref_ptr<LSProject> project, const std::string outputPath);



    /**
     * @brief 保存XYZRGB点云到PLY文件
     * @param filename 要保存的文件名
     * @param points RGB点集
     * @param binary 是否保存为二进制格式
     */
    static bool savePointCloudToPLY(const std::string&,
        const std::vector<LSPointXYZRGB>&,
        bool binary);

    /**
     * 保存带法向量的点云到PLY文件
     */
    static bool savePointCloudWithNormalsToPLY(const std::string&,
        const std::vector<LSPointXYZN>&,
        bool binary);

    /**
     * 保存带反射强度的点云到PLY文件
     */
    static bool savePointCloudWithIntensityToPLY(const std::string&,
        const std::vector<LSPointXYZI>&,
        bool binary);
    
};