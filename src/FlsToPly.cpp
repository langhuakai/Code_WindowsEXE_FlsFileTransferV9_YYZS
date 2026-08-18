// FlsToPly.cpp
#include "FlsToPly.h"
#include <cmath>
#include <algorithm>

// 将 [0,1] 强度值映射为伪彩色 (jet色谱)
static void intensityToColor(float t, uint8_t& r, uint8_t& g, uint8_t& b)
{
    t = std::max(0.0f, std::min(1.0f, t));
    // jet色谱: 蓝 -> 青 -> 绿 -> 黄 -> 红
    float rVal, gVal, bVal;
    if (t < 0.25f) {
        rVal = 0.0f;
        gVal = t * 4.0f;
        bVal = 1.0f;
    } else if (t < 0.5f) {
        rVal = 0.0f;
        gVal = 1.0f;
        bVal = 1.0f - (t - 0.25f) * 4.0f;
    } else if (t < 0.75f) {
        rVal = (t - 0.5f) * 4.0f;
        gVal = 1.0f;
        bVal = 0.0f;
    } else {
        rVal = 1.0f;
        gVal = 1.0f - (t - 0.75f) * 4.0f;
        bVal = 0.0f;
    }
    r = static_cast<uint8_t>(rVal * 255.0f + 0.5f);
    g = static_cast<uint8_t>(gVal * 255.0f + 0.5f);
    b = static_cast<uint8_t>(bVal * 255.0f + 0.5f);
}

// 将fls转换为rgb颜色文件
std::string FlsToPly::transToPlyRgb(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving RGB point cloud..." << std::endl;

    bool hasColor = scan->dataAvailable(LSScan::COLOR);
    bool hasReflection = scan->dataAvailable(LSScan::REFLECTION);
    std::cout << "Color data available: " << (hasColor ? "Yes" : "No") << std::endl;
    std::cout << "Reflection data available: " << (hasReflection ? "Yes" : "No") << std::endl;
    std::cout << "Total rows: " << scan->getNumRows() << ", cols: " << scan->getNumCols() << std::endl;

    if (!hasColor && !hasReflection) {
        std::cerr << "Warning: No color or reflection data available for this scan." << std::endl;
        return "";
    }

    // 诊断: 检查几个不同位置的点的颜色
    if (hasColor && scan->getNumRows() > 0 && scan->getNumCols() > 0) {
        int testRows[] = {0, scan->getNumRows()/4, scan->getNumRows()/2, scan->getNumRows()-1};
        int testCols[] = {0, scan->getNumCols()/4, scan->getNumCols()/2, scan->getNumCols()-1};
        std::cout << "\nDiagnostic point colors (row, col) -> RGB:" << std::endl;
        for (int r : testRows) {
            for (int c : testCols) {
                uint8_t rVal, gVal, bVal;
                bool isRGB = false;
                if (scan->getPointColor(c, r, rVal, gVal, bVal, isRGB)) {
                    std::cout << "  (" << r << "," << c << ") -> rgb=("
                              << static_cast<int>(rVal) << ","
                              << static_cast<int>(gVal) << ","
                              << static_cast<int>(bVal) << ") isRGB=" << (isRGB ? "true" : "false") << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }

    std::vector<LSPointXYZRGB> rgbPoints;
    rgbPoints.reserve(1000000);
    uint64_t pointCount = 0;

    if (hasColor) {
        std::cout << "Using real camera RGB data." << std::endl;
    } else {
        std::cout << "No camera color. Will apply false-color map." << std::endl;
    }

    // 统一用 getScanIterator 获取 RGB 点
    // 有颜色时返回真实 RGB，无颜色时 SDK 做对比度增强返回灰度 RGB
    LSScanIterator<LSPointXYZRGB> scanIterator = scan->getScanIterator();
    while (scanIterator.moveNext()) {
        rgbPoints.push_back(scanIterator.getCurrent());
        pointCount++;
        if (pointCount % 100000 == 0) {
            std::cout << "Collected " << pointCount << " points..." << std::endl;
        }
    }

    // 如果没有真实颜色，对灰度值应用伪彩色
    if (!hasColor && pointCount > 0) {
        std::cout << "Applying false-color map to contrast-enhanced grayscale..." << std::endl;
        for (auto& pt : rgbPoints) {
            // 灰度值取 RGB 三通道平均
            float gray = (static_cast<float>(pt.r) + static_cast<float>(pt.g) + static_cast<float>(pt.b)) / 3.0f / 255.0f;
            intensityToColor(gray, pt.r, pt.g, pt.b);
        }
    }

    std::cout << "Total points collected: " << pointCount << std::endl;

    if (pointCount > 0) {
        savePointCloudToPLY(outputPath, rgbPoints, true);
    }
    return outputPath;
}

// 将fls转换为带法向量的点云文件
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

// 将fls转换为带反射强度的点云文件
std::string FlsToPly::transToPlyIntensity(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving reflection data..." << std::endl;

    if (!scan->dataAvailable(LSScan::REFLECTION)) {
        std::cerr << "Warning: No reflection data available for this scan." << std::endl;
        return "";
    }

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

// 将fls转换为球坐标文件
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

            // 获取点云的迭代器
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

                // 可选：保存为二进制格式
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
 * @param binary 是否保存为二进制格式
 */
bool FlsToPly::savePointCloudToPLY(const std::string& filename,
    const std::vector<LSPointXYZRGB>& points,
    bool binary) {
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
    bool binary) {
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
    bool binary) {
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
