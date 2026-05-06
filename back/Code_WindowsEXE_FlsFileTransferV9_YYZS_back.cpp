#include "Code_WindowsEXE_FlsFileTransferV9_YYZS.h"
#include "ArgumentParser.h"
#include "StringConverter.h"

#include <iostream>
#include <filesystem>
#include <fstream>     // 添加文件流头文件
#include <iomanip>     // 添加IO格式化头文件
#include <core_api/lsglobaldefinitions.h>
#include <core_api/lsproject.h>
#include <core_api/lsworkspace.h>
#include <core_api/lsobject.h>
#include <core_api/lsscan.h>
#include <core_api/lspointcloud.h>
#include <core_api/lspointtypes.h>
#include <core_api/lsiterator.h>
#include <core_api/lsscaniterator.h>
#include <core_api/lsstring.h>
#include <core_api/lsresult.h>
#include <standalone_api/lsstandalonecontext.h>
#include <standalone_api/lsstandaloneconfig.h>

using namespace SCENE_API;
namespace fs = std::filesystem;

// 你的 API Key (用于解锁读取点云功能)
// const wchar_t* API_KEY = L"YOUR_API_KEY_HERE";
const wchar_t* API_KEY = L"(YOUR_API_KEY_HERE)";
const LSString apiKey = LSString::fromCharStr(R"key(
-----BEGIN SCENE API KEY-----
YOUR_API_KEY_HERE
-----END SCENE API KEY-----
)key");

// FLS 文件路径
const wchar_t* FLS_FILE_PATH = L"C:\\Users\\Administrator\\Desktop\\V9-OldRock-001.fls";

/**
 * 保存XYZRGB点云到PLY文件
 */
bool savePointCloudToPLY(const std::string&,
    const std::vector<LSPointXYZRGB>&,
    bool binary);

/**
 * 保存带法向量的点云到PLY文件
 */
bool savePointCloudWithNormalsToPLY(const std::string&,
    const std::vector<LSPointXYZN>&,
    bool binary);

/**
 * 保存带反射强度的点云到PLY文件
 */
bool savePointCloudWithIntensityToPLY(const std::string&,
    const std::vector<LSPointXYZI>&,
    bool binary);

int main_back(int argc, char* argv[])
{

    // 创建参数解析器
    ArgumentParser parser;

    // 解析参数
    auto options = parser.parse(argc, argv);

    // 显示帮助
    if (options.help)
    {
        ArgumentParser::showHelp(argv[0]);
        return 0;
    }

    // 检查参数是否有效
    if (!options.valid)
    {
        std::cerr << std::endl;
        ArgumentParser::showHelp(argv[0]);
        return 1;
    }

    // 使用参数
    std::cout << "开始处理..." << std::endl;
    std::cout << "输入文件: " << options.inputFile << std::endl;
    std::cout << "输出目录: " << options.outputDir << std::endl;
    std::cout << std::endl;


    // ============================================================
    // 步骤 1: 创建 Standalone Context (需要 API Key)
    // ============================================================
    // 方式 1: 使用 LSStandaloneConfig 配置 (推荐)
    LSStandaloneConfig config;
    config.setLicenseKey(apiKey);
    config.setLoggingEnabled(true);
    LSStandaloneContext* context = new LSStandaloneContext(config);

    // 方式 2: 直接使用 API Key 构造 (简单方式)
    // LSStandaloneContext* context = new LSStandaloneContext(API_KEY);

    std::cout << "SCENE API Context created successfully." << std::endl;

    // ============================================================
    // 步骤 2: 创建临时项目用于导入 FLS 文件
    // ============================================================
    // 创建一个临时项目路径
    LSString tempProjectPath = L"E:\\temp\\fls_import_project";
    LSString projectName = L"FLS_Import_Project";

    // 检查文件夹是否存在，如果存在则删除
    try {
        if (fs::exists(tempProjectPath.toWcharStr())) {
            std::cout << "Temporary project directory already exists. Removing it..." << std::endl;

            // 删除整个目录
            std::error_code ec;
            uintmax_t removedCount = fs::remove_all(tempProjectPath.toWcharStr(), ec);

            if (ec) {
                std::cerr << "Failed to remove existing directory. Error: " << ec.message() << std::endl;
                delete context;
                return -1;
            }

            std::cout << "Removed " << removedCount << " items from existing directory." << std::endl;

            // 等待一小会儿确保文件系统操作完成
#ifdef _WIN32
            _sleep(100);
#else
            usleep(100000);
#endif
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while checking/removing directory: " << e.what() << std::endl;
        delete context;
        return -1;
    }

    // 创建父目录（如果不存在）
    LSString parentDir = L"E:\\temp";
    try {
        if (!fs::exists(parentDir.toWcharStr())) {
            if (fs::create_directories(parentDir.toWcharStr())) {
                std::cout << "Created parent directory: " << parentDir.toCharStr() << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create parent directory: " << e.what() << std::endl;
        delete context;
        return -1;
    }

    LSResult::Result result = LSProject::createProject(tempProjectPath, projectName);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to create project. Error code: " << result << std::endl;
        delete context;
        return -1;
    }
    std::cout << "Project created successfully." << std::endl;

    // ============================================================
    // 步骤 3: 加载项目
    // ============================================================
    ref_ptr<LSProject> project;
    result = LSProject::loadProject(tempProjectPath, project, false);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to load project. Error code: " << result << std::endl;
        delete context;
        return -1;
    }
    std::cout << "Project loaded successfully." << std::endl;

    // 设置当前工作空间 (Standalone API 必需)
    context->setCurrentWorkspace(project);

    // ============================================================
    // 步骤 4: 导入 FLS 文件
    // ============================================================
    // 使用 LSWorkspace::importData() 导入 FLS 文件
    // LSString::fromCharStr(options.inputFile.c_str());
    bool importSuccess = project->importData(LSString::fromCharStr(options.inputFile.c_str()));
    if (!importSuccess) {
        std::cerr << "Failed to import FLS file." << std::endl;
        delete context;
        return -1;
    }
    std::cout << "FLS file imported successfully." << std::endl;

    // ============================================================
    // 步骤 5: 获取导入的扫描数据
    // ============================================================
    // 获取所有扫描
    LSPod<std::vector<ref_ptr<LSScan>>> scans = project->getScans();
    if (scans.get().empty()) {
        std::cerr << "No scans found in project." << std::endl;
        delete context;
        return -1;
    }
    std::cout << "Found " << scans.get().size() << " scan(s) in project." << std::endl;

    // 创建输出目录
    std::string outputDir = options.outputDir;
    try {
        if (!fs::exists(outputDir)) {
            fs::create_directories(outputDir);
            std::cout << "Created output directory: " << outputDir << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create output directory: " << e.what() << std::endl;
        outputDir = ".";  // 使用当前目录
    }

    // ============================================================
    // 步骤 6: 遍历每个扫描并读取点云数据
    // ============================================================
    for (size_t i = 0; i < scans.get().size(); ++i) {
        ref_ptr<LSScan> scan = scans.get()[i];
        std::cout << "\n--- Processing Scan " << (i + 1) << " ---" << std::endl;

        // 检查是否有空间数据
        /*if (!scan->spatialDataAvailable()) {
            std::cout << "No spatial data available for this scan." << std::endl;
            continue;
        }*/

        // ============================================================
        // 步骤 7: 加载扫描数据 (需要 API Key)
        // ============================================================
        // 加载数据 - 这需要 API Key
        result = scan->loadData(LSScan::AUTO);
        if (result != LSResult::Ok) {
            std::cerr << "Failed to load scan data. Error code: " << result << std::endl;
            if (result == LSResult::Unauthorized) {
                std::cerr << "ERROR: API Key is invalid or missing!" << std::endl;
            }
            continue;
        }
        std::cout << "Scan data loaded successfully." << std::endl;

        // ============================================================
        // 步骤 8: 使用迭代器遍历点云数据并保存为PLY
        // ============================================================
        // 保存RGB点云
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
            std::string plyFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_rgb.ply";
            if (savePointCloudToPLY(plyFilename, rgbPoints, false)) {  // 使用ASCII格式
                std::cout << "RGB point cloud saved to: " << plyFilename << std::endl;
            }

            // 可选：保存二进制格式
            std::string plyBinaryFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_rgb_binary.ply";
            savePointCloudToPLY(plyBinaryFilename, rgbPoints, true);
        }

        // ============================================================
        // 步骤 9: 获取并保存带法向量的点云
        // ============================================================
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
                std::string plyFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_normals.ply";
                if (savePointCloudWithNormalsToPLY(plyFilename, normalPoints, false)) {
                    std::cout << "Point cloud with normals saved to: " << plyFilename << std::endl;
                }
            }
        }

        // ============================================================
        // 步骤 10: 获取并保存反射强度数据
        // ============================================================
        if (scan->dataAvailable(LSScan::REFLECTION)) {
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
                std::string plyFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_intensity.ply";
                if (savePointCloudWithIntensityToPLY(plyFilename, intensityPoints, false)) {
                    std::cout << "Point cloud with intensity saved to: " << plyFilename << std::endl;
                }
            }
        }

        // ============================================================
        // 步骤 11: 获取并保存球坐标数据
        // ============================================================
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
                std::string plyFilename = outputDir + "\\scan_" + std::to_string(i + 1) + "_spherical.ply";
                if (savePointCloudToPLY(plyFilename, sphericalPoints, false)) {
                    std::cout << "Spherical point cloud saved to: " << plyFilename << std::endl;
                }
            }
        }

        // ============================================================
        // 步骤 12: 卸载扫描数据 (释放内存)
        // ============================================================
        scan->unloadData();
        std::cout << "\nScan data unloaded." << std::endl;
    }

    // ============================================================
    // 步骤 13: 创建项目点云 (可选)
    // ============================================================
    // 创建项目点云 (合并所有扫描)
    result = project->createPointCloud();
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
                std::string plyFilename = outputDir + "\\project_merged_pointcloud.ply";
                if (savePointCloudToPLY(plyFilename, projectPoints, false)) {
                    std::cout << "Project point cloud saved to: " << plyFilename << std::endl;
                }

                // 可选：保存二进制格式
                std::string plyBinaryFilename = outputDir + "\\project_merged_pointcloud_binary.ply";
                savePointCloudToPLY(plyBinaryFilename, projectPoints, true);
            }
        }
    }

    // ============================================================
    // 清理
    // ============================================================
    std::cout << "\nCleaning up..." << std::endl;

    // 显示输出目录中的文件
    try {
        std::cout << "\nGenerated PLY files in " << outputDir << ":" << std::endl;
        for (const auto& entry : fs::directory_iterator(outputDir)) {
            if (entry.path().extension() == ".ply") {
                std::cout << "  " << entry.path().filename().string() << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Cannot list output files: " << e.what() << std::endl;
    }

    // 释放项目
    project.reset();

    // 清理临时目录
    try {
        std::cout << "\nCleaning up temporary project directory..." << std::endl;
        std::error_code ec;
        uintmax_t removedCount = fs::remove_all(tempProjectPath.toWcharStr(), ec);
        if (ec) {
            std::cerr << "Warning: Could not completely clean up temp directory: " << ec.message() << std::endl;
        }
        else {
            std::cout << "Removed " << removedCount << " items from temp directory." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while cleaning up temp directory: " << e.what() << std::endl;
    }

    // 删除 context (必须在所有 API 对象之后删除)
    delete context;

    std::cout << "\nDone! All point clouds have been saved to PLY format." << std::endl;
    std::cout << "Output directory: " << outputDir << std::endl;
    std::cout << "Done!" << std::endl;
    system("pause");

    return 0;
}

// ============================================================
// 辅助函数: 打印点信息
// ============================================================
void printPoint(const LSPointXYZRGB& point) {
    std::cout << "Point: ("
        << point.x << ", "
        << point.y << ", "
        << point.z << ") Color: ("
        << (int)point.r << ", "
        << (int)point.g << ", "
        << (int)point.b << ")" << std::endl;
}

void printPointWithNormal(const LSPointXYZN& point) {
    std::cout << "Point: ("
        << point.x << ", "
        << point.y << ", "
        << point.z << ") Normal: ("
        << point.nX << ", "
        << point.nY << ", "
        << point.nZ << ")" << std::endl;
}

// ============================================================
// 示例: 带过滤条件的迭代
// ============================================================
void exampleWithFilter(ref_ptr<LSScan> scan) {
    // 加载数据
    scan->loadData(LSScan::AUTO);

    // 获取迭代器
    LSScanIterator<LSPointXYZRGB> iterator = scan->getScanIterator();

    // 定义过滤函数 - 只保留 z > 0 的点
    auto filter = [](const LSPointXYZRGB& point) -> bool {
        return point.z > 0.0;
        };

    // 应用过滤
    LSIterator<LSPointXYZRGB> filteredIterator = iterator.where(filter);

    // 遍历过滤后的点
    uint64_t count = 0;
    while (filteredIterator.moveNext()) {
        LSPointXYZRGB point = filteredIterator.getCurrent();
        // 处理点...
        count++;
    }

    std::cout << "Filtered points count: " << count << std::endl;

    scan->unloadData();
}


// ============================================================
// PLY文件保存函数
// ============================================================

/**
 * 保存XYZRGB点云到PLY文件
 */
bool savePointCloudToPLY(const std::string& filename,
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
bool savePointCloudWithNormalsToPLY(const std::string& filename,
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
bool savePointCloudWithIntensityToPLY(const std::string& filename,
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
