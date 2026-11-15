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
    // 以下 transTo* 成功返回输出路径, 失败返回空串
    // failReason 非空时回填失败原因, 供上层汇总打印
    // 将点云转换为ply文件（颜色与反射强度都有时同时写出, 否则逐级回退）
    static std::string transToPlyRgb(const ref_ptr<LSScan> scan, const std::string outputPath,
        std::string* failReason = nullptr);

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
        bool binary,
        std::string* failReason = nullptr);

    /**
     * 保存带法向量的点云到PLY文件
     */
    static bool savePointCloudWithNormalsToPLY(const std::string&,
        const std::vector<LSPointXYZN>&,
        bool binary,
        std::string* failReason = nullptr);

    /**
     * 保存带反射强度的点云到PLY文件
     */
    static bool savePointCloudWithIntensityToPLY(const std::string&,
        const std::vector<LSPointXYZI>&,
        bool binary,
        std::string* failReason = nullptr);

    /**
     * 保存同时带颜色与反射强度的点云到PLY文件
     */
    static bool savePointCloudWithColorAndIntensityToPLY(const std::string&,
        const std::vector<LSPointXYZRGB>&,
        const std::vector<float>&,
        bool binary,
        std::string* failReason = nullptr);

};