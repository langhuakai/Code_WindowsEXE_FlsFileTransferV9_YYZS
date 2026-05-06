#include "Code_WindowsEXE_FlsFileTransferV9_YYZS.h"
#include "ArgumentParser.h"
#include "StringConverter.h"
#include "FlsToPly.h"

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
const LSString apiKey = LSString::fromCharStr( R"key(
-----BEGIN SCENE API KEY-----
AAIAAMQkN9tn1q/Unp9ozi4TPv+ULNOCU5DBE/Gm6qmwW83gAwyFOwXyU3NXdD12Fbjgq2Nm
brPJNQugoImH5U7mPfmbMiSAIUwHMGEdwLtB0X6D7eAOfORMp7WfLGbursxzeHPxfutQLm68
aSxnFev2nO8Nnnwe5xotk4zMgg4Q/hMuZdJa99pdp8Ev9HBJcTFBW+z1AZz71rSH/tNTGots
/zy3i0xDkh2u+lwFM8mlEyPYo2yqyjSiWZHq2+mYuUOh38lr+Bu2YktDCQ2/H20PB+utjCJ/
gqpksHNAcrcn+XcsHsU66g0EtpZXUn8lBqpc1zAQ+THuL/162rOYyxCOaEoLBkB6zBW0gMxH
4pXFfUOrqvtweoYiPmqdESgTiGRbWph56cCgxcBvjSk1yp3JrIBuI31HTowhb7pKMsYX8QNa
wkmLqkgI/wGkndBCX66eYsoMo/LZD2EUbjxx4NkE8xTon8VKoJJsskIEhyv0FR24ZqKgKyB5
D4yeBBiU2KEZtLAtp9mGiXVt44C8B6tIZG2iPtPox8UBELaCqargcAKxHqunBI1FGYOfuqlu
aDanUhCauYndHVJnSn02Ak09qSxWtRASK1MMRINMgcHan2O2k0ZgIcBKJcGb7qgehskTU1Xq
ScjCY48WhhLpObx35BfTYyg0mn/HKg8rw0lH0L/CewogICJjb250YWN0IjogIlRpYW5xaSBZ
aSA8amVzc2ljYUBtZXRhZGlnaXRhbC5uZXQuY24+IiwKICAiZmVhdHVyZXMiOiBbInRvcGNv
bkltcG9ydCIsInByb2Nlc3NTY2FuIiwiY29sb3JpemVTY2FuIiwiVjlpbXBvcnQiXSwKICAi
bmFtZSI6ICJNZXRhZGlnaXRhbCIsCiAgInV1aWQiOiAiMGI1N2FhNWUtNWE3Ni00OTJkLWE5
ZmUtMzMzOGQ1ZGQ1YzdkIiwKICAidmVyc2lvbiI6IDIsCiAgImV4cGlyYXRpb24iOiAiMjAy
OS0wNC0yMFQwMDowMDowMC4wMDBaIgp9
-----END SCENE API KEY-----
)key");

int main(int argc, char* argv[])
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
    LSString tempProjectPath = L".\\fls_transfer\\fls_import_project";
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
    LSString parentDir = L".\\fls_transfer";
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
    //try {
    //    if (!fs::exists(outputDir)) {
    //        fs::create_directories(outputDir);
    //        std::cout << "Created output directory: " << outputDir << std::endl;
    //    }
    //}
    //catch (const std::exception& e) {
    //    std::cerr << "Failed to create output directory: " << e.what() << std::endl;
    //    outputDir = ".";  // 使用当前目录
    //}

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
        FlsToPly::transToPlyRgb(scan, options.outputDir);

        // ============================================================
        // 步骤 12: 卸载扫描数据 (释放内存)
        // ============================================================
        scan->unloadData();
        std::cout << "\nScan data unloaded." << std::endl;
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
    // system("pause");

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
