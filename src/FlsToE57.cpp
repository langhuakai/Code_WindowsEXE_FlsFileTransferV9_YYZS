#include "FlsToE57.h"
#include "StringConverter.h"
#include <core_api/lsstring.h>
#include <core_api/lsresult.h>
#include <filesystem>

namespace fs = std::filesystem;

std::string FlsToE57::transToE57Scan(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nExporting scan to E57 format..." << std::endl;

    LSString e57Path = LSString::fromCharStr(outputPath.c_str());

    LSResult::Result result = scan->exportData(e57Path, LSScan::E57);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to export scan to E57. Error code: " << result << std::endl;
        return "";
    }

    std::cout << "Scan exported to E57: " << StringConverter::ansiToUtf8(outputPath) << std::endl;
    return outputPath;
}

std::string FlsToE57::transToE57ScanSpatial(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    if (!scan->spatialDataAvailable()) {
        std::cerr << "No spatial data available for this scan." << std::endl;
        return "";
    }

    std::cout << "\nExporting spatial scan data to E57 format..." << std::endl;

    LSString e57Path = LSString::fromCharStr(outputPath.c_str());

    LSResult::Result result = scan->exportSpatialData(e57Path, LSScan::E57);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to export spatial data to E57. Error code: " << result << std::endl;
        return "";
    }

    std::cout << "Spatial data exported to E57: " << StringConverter::ansiToUtf8(outputPath) << std::endl;
    return outputPath;
}

std::string FlsToE57::transToE57Project(const ref_ptr<LSProject> project, const std::string outputDir)
{
    LSPod<std::vector<ref_ptr<LSScan>>> scans = project->getScans();
    if (scans.get().empty()) {
        std::cerr << "No scans found in project." << std::endl;
        return "";
    }

    std::string lastOutput;
    for (size_t i = 0; i < scans.get().size(); ++i) {
        ref_ptr<LSScan> scan = scans.get()[i];
        std::cout << "\n--- Exporting Scan " << (i + 1) << " to E57 ---" << std::endl;

        LSResult::Result result = scan->loadData(LSScan::AUTO);
        if (result != LSResult::Ok) {
            std::cerr << "Failed to load scan data. Error code: " << result << std::endl;
            continue;
        }

        std::string filename = outputDir + "\\scan_" + std::to_string(i + 1) + ".e57";
        lastOutput = transToE57Scan(scan, filename);

        scan->unloadData();
    }

    return lastOutput;
}

