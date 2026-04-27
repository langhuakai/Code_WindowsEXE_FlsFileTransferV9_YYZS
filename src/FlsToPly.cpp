// StringConverter.h
#pragma once

#include "FlsToPly.h"

// 将fls转成rgb点云文件
std::string FlsToPly::transToPlyRgb(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving RGB point cloud..." << std::endl;
    LSScanIterator<LSPointXYZRGB> scanIterator = scan->getScanIterator();

    std::vector<LSPointXYZRGB> rgbPoints;
    rgbPoints.reserve(1000000);  // 预分配空间

    uint64_t pointCount = 0;
    while (scanIterator.moveNext()) {
        LSPointXYZRGB point = scanIterator.getCurrent();
        rgbPoints.push_back(point);
        pointCount++;

        if (pointCount % 100000 == 0) {
            std::cout << "Collected " << pointCount << " RGB points..." << std::endl;
        }
    }

    std::cout << "Total RGB points collected: " << pointCount << std::endl;

    // 保存到PLY文件
    /* std::string plyFilename = outputDir + "\\scan_" + std::to_string(1) + "_rgb.ply";
    if (savePointCloudToPLY(plyFilename, rgbPoints, false)) {  // 使用ASCII格式
        std::cout << "RGB point cloud saved to: " << plyFilename << std::endl;
    }*/

    // 可选：保存二进制格式
    // std::string plyBinaryFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_rgb_binary.ply";
    savePointCloudToPLY(outputPath, rgbPoints, true);
    return outputPath;
}

// 将fls转成带法向量的点云文件
std::string FlsToPly::transToPlyNormals(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving point cloud with normals..." << std::endl;
    // 获取带法向量的扫描迭代器
    LSScanIterator<LSPointXYZN> normalIterator = scan->getScanNormalsIterator();

    std::vector<LSPointXYZN> normalPoints;
    normalPoints.reserve(1000000);

    uint64_t pointCount = 0;
    while (normalIterator.moveNext()) {
        LSPointXYZN point = normalIterator.getCurrent();
        normalPoints.push_back(point);
        pointCount++;

        if (pointCount % 100000 == 0) {
            std::cout << "Collected " << pointCount << " points with normals..." << std::endl;
        }
    }

    std::cout << "Total points with normals collected: " << pointCount << std::endl;

    if (pointCount > 0) {
        // std::string plyFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_normals.ply";
        if (savePointCloudWithNormalsToPLY(outputPath, normalPoints, false)) {
            std::cout << "Point cloud with normals saved to: " << outputPath << std::endl;
        }
    }
    return outputPath;
}

// 将fls转成带反射强度的点云文件
std::string FlsToPly::transToPlyIntensity(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving reflection data..." << std::endl;
    LSScanIterator<LSPointXYZI> reflectionIterator = scan->getScanReflectionIterator();

    std::vector<LSPointXYZI> intensityPoints;
    intensityPoints.reserve(1000000);

    uint64_t pointCount = 0;
    while (reflectionIterator.moveNext()) {
        LSPointXYZI point = reflectionIterator.getCurrent();
        intensityPoints.push_back(point);
        pointCount++;

        if (pointCount % 100000 == 0) {
            std::cout << "Collected " << pointCount << " intensity points..." << std::endl;
        }
    }

    std::cout << "Total points with reflection collected: " << pointCount << std::endl;

    if (pointCount > 0) {
        // std::string plyFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_intensity.ply";
        if (savePointCloudWithIntensityToPLY(outputPath, intensityPoints, false)) {
            std::cout << "Point cloud with intensity saved to: " << outputPath << std::endl;
        }
    }
    return outputPath;
}

// 将fls转成球坐标点云文件
std::string FlsToPly::transToPlySpherical(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving spherical coordinates..." << std::endl;
    // 获取球坐标迭代器
    LSSphericalIterator<LSPointSphericalRGB> sphericalIterator = scan->getSphericalCoordinatesIterator();

    // 注意：球坐标点需要转换为笛卡尔坐标保存
    std::vector<LSPointXYZRGB> sphericalPoints;
    sphericalPoints.reserve(1000000);

    uint64_t pointCount = 0;
    while (sphericalIterator.moveNext()) {
        LSPointSphericalRGB sphericalPoint = sphericalIterator.getCurrent();

        // 将球坐标转换为笛卡尔坐标
        double radius = sphericalPoint.radius;
        double phi = sphericalPoint.phi;     // 方位角
        double theta = sphericalPoint.theta; // 极角

        LSPointXYZRGB cartesianPoint;
        cartesianPoint.x = radius * sin(theta) * cos(phi);
        cartesianPoint.y = radius * sin(theta) * sin(phi);
        cartesianPoint.z = radius * cos(theta);
        cartesianPoint.r = sphericalPoint.r;
        cartesianPoint.g = sphericalPoint.g;
        cartesianPoint.b = sphericalPoint.b;

        sphericalPoints.push_back(cartesianPoint);
        pointCount++;

        if (pointCount % 100000 == 0) {
            std::cout << "Collected " << pointCount << " spherical points..." << std::endl;
        }
    }

    std::cout << "Total spherical points collected: " << pointCount << std::endl;

    if (pointCount > 0) {
        // std::string plyFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_spherical.ply";
        if (savePointCloudToPLY(outputPath, sphericalPoints, false)) {
            std::cout << "Spherical point cloud saved to: " << outputPath << std::endl;
        }
    }
    return outputPath;
}

std::string FlsToPly::transToPlyProject(const ref_ptr<LSProject> project, const std::string outputPath)
{
    // 创建项目点云 (合并所有扫描)
    LSResult::Result result = project->createPointCloud();
    if (result == LSResult::Ok) {
        std::cout << "\nProject point cloud created successfully." << std::endl;

        // 获取项目点云
        ref_ptr<LSPointCloud> pointCloud = project->getPointCloud();
        if (pointCloud) {
            std::cout << "Reading and saving project point cloud..." << std::endl;

            // 获取点云迭代器
            LSIterator<LSPointXYZRGB> pcIterator = pointCloud->getPointCloudIterator();

            std::vector<LSPointXYZRGB> projectPoints;
            projectPoints.reserve(1000000);

            uint64_t pcPointCount = 0;
            while (pcIterator.moveNext()) {
                LSPointXYZRGB point = pcIterator.getCurrent();
                projectPoints.push_back(point);
                pcPointCount++;

                if (pcPointCount % 100000 == 0) {
                    std::cout << "Collected " << pcPointCount << " project points..." << std::endl;
                }
            }

            std::cout << "Total points in project point cloud: " << pcPointCount << std::endl;

            if (pcPointCount > 0) {
                // std::string plyFilename = outputDir + "\\project_merged_pointcloud.ply";
                if (savePointCloudToPLY(outputPath, projectPoints, false)) {
                    std::cout << "Project point cloud saved to: " << outputPath << std::endl;
                }

                // 可选：保存二进制格式
                // std::string plyBinaryFilename = outputDir + "\\project_merged_pointcloud_binary.ply";
                // savePointCloudToPLY(plyBinaryFilename, projectPoints, true);
            }
        }
    }
    return outputPath;
}

/**
 * @brief 保存XYZRGB点云到PLY文件
 * @param filename 要保存的文件名
 * @param points 点集
 * @param binary 是否保存为二进制点云
 */
bool FlsToPly::savePointCloudToPLY(const std::string& filename,
    const std::vector<LSPointXYZRGB>& points,
    bool binary = false) {
    std::ofstream file;
    if (binary) {
        file.open(filename, std::ios::binary);
    }
    else {
        file.open(filename);
    }

    if (!file.is_open()) {
        std::cerr << "Failed to open PLY file: " << filename << std::endl;
        return false;
    }

    // 写入PLY头部
    file << "ply\n";
    if (binary) {
        file << "format binary_little_endian 1.0\n";
    }
    else {
        file << "format ascii 1.0\n";
    }
    file << "element vertex " << points.size() << "\n";
    file << "property float x\n";
    file << "property float y\n";
    file << "property float z\n";
    file << "property uchar red\n";
    file << "property uchar green\n";
    file << "property uchar blue\n";
    file << "end_header\n";

    // 写入点数据
    if (binary) {
        for (const auto& point : points) {
            // 写入坐标（float格式）
            float x = static_cast<float>(point.x);
            float y = static_cast<float>(point.y);
            float z = static_cast<float>(point.z);
            file.write(reinterpret_cast<const char*>(&x), sizeof(float));
            file.write(reinterpret_cast<const char*>(&y), sizeof(float));
            file.write(reinterpret_cast<const char*>(&z), sizeof(float));

            // 写入颜色
            file.write(reinterpret_cast<const char*>(&point.r), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&point.g), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&point.b), sizeof(uint8_t));
        }
    }
    else {
        // ASCII格式
        file << std::fixed << std::setprecision(6);
        for (const auto& point : points) {
            file << point.x << " " << point.y << " " << point.z << " "
                << static_cast<int>(point.r) << " "
                << static_cast<int>(point.g) << " "
                << static_cast<int>(point.b) << "\n";
        }
    }

    file.close();
    std::cout << "Saved " << points.size() << " points to PLY file: " << filename << std::endl;
    return true;
}

/**
 * 保存带法向量的点云到PLY文件
 */
bool FlsToPly::savePointCloudWithNormalsToPLY(const std::string& filename,
    const std::vector<LSPointXYZN>& points,
    bool binary = false) {
    std::ofstream file;
    if (binary) {
        file.open(filename, std::ios::binary);
    }
    else {
        file.open(filename);
    }

    if (!file.is_open()) {
        std::cerr << "Failed to open PLY file: " << filename << std::endl;
        return false;
    }

    // 写入PLY头部
    file << "ply\n";
    if (binary) {
        file << "format binary_little_endian 1.0\n";
    }
    else {
        file << "format ascii 1.0\n";
    }
    file << "element vertex " << points.size() << "\n";
    file << "property float x\n";
    file << "property float y\n";
    file << "property float z\n";
    file << "property float nx\n";
    file << "property float ny\n";
    file << "property float nz\n";
    file << "end_header\n";

    // 写入点数据
    if (binary) {
        for (const auto& point : points) {
            // 写入坐标（float格式）
            float x = static_cast<float>(point.x);
            float y = static_cast<float>(point.y);
            float z = static_cast<float>(point.z);
            file.write(reinterpret_cast<const char*>(&x), sizeof(float));
            file.write(reinterpret_cast<const char*>(&y), sizeof(float));
            file.write(reinterpret_cast<const char*>(&z), sizeof(float));

            // 写入法向量
            float nx = point.nX;
            float ny = point.nY;
            float nz = point.nZ;
            file.write(reinterpret_cast<const char*>(&nx), sizeof(float));
            file.write(reinterpret_cast<const char*>(&ny), sizeof(float));
            file.write(reinterpret_cast<const char*>(&nz), sizeof(float));
        }
    }
    else {
        // ASCII格式
        file << std::fixed << std::setprecision(6);
        for (const auto& point : points) {
            file << point.x << " " << point.y << " " << point.z << " "
                << point.nX << " " << point.nY << " " << point.nZ << "\n";
        }
    }

    file.close();
    std::cout << "Saved " << points.size() << " points with normals to PLY file: " << filename << std::endl;
    return true;
}

/**
 * 保存带反射强度的点云到PLY文件
 */
bool FlsToPly::savePointCloudWithIntensityToPLY(const std::string& filename,
    const std::vector<LSPointXYZI>& points,
    bool binary = false) {
    std::ofstream file;
    if (binary) {
        file.open(filename, std::ios::binary);
    }
    else {
        file.open(filename);
    }

    if (!file.is_open()) {
        std::cerr << "Failed to open PLY file: " << filename << std::endl;
        return false;
    }

    // 写入PLY头部
    file << "ply\n";
    if (binary) {
        file << "format binary_little_endian 1.0\n";
    }
    else {
        file << "format ascii 1.0\n";
    }
    file << "element vertex " << points.size() << "\n";
    file << "property float x\n";
    file << "property float y\n";
    file << "property float z\n";
    file << "property float intensity\n";
    file << "end_header\n";

    // 写入点数据
    if (binary) {
        for (const auto& point : points) {
            // 写入坐标（float格式）
            float x = static_cast<float>(point.x);
            float y = static_cast<float>(point.y);
            float z = static_cast<float>(point.z);
            file.write(reinterpret_cast<const char*>(&x), sizeof(float));
            file.write(reinterpret_cast<const char*>(&y), sizeof(float));
            file.write(reinterpret_cast<const char*>(&z), sizeof(float));

            // 写入反射强度
            float intensity = point.i;
            file.write(reinterpret_cast<const char*>(&intensity), sizeof(float));
        }
    }
    else {
        // ASCII格式
        file << std::fixed << std::setprecision(6);
        for (const auto& point : points) {
            file << point.x << " " << point.y << " " << point.z << " "
                << point.i << "\n";
        }
    }

    file.close();
    std::cout << "Saved " << points.size() << " points with intensity to PLY file: " << filename << std::endl;
    return true;
}