#include "FlsToOff.h"

bool FlsToOff::savePointCloudToOFF(const std::string& filename,
    const std::vector<LSPointXYZRGB>& points)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open OFF file: " << filename << std::endl;
        return false;
    }
    file << "OFF\n";
    file << points.size() << " 0 0\n";
    for (const auto& p : points) {
        file << p.x << " " << p.y << " " << p.z << " "
             << static_cast<int>(p.r) << " "
             << static_cast<int>(p.g) << " "
             << static_cast<int>(p.b) << "\n";
    }
    file.close();
    std::cout << "Saved " << points.size() << " points to OFF file: " << filename << std::endl;
    return true;
}

std::string FlsToOff::transToOff(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving OFF point cloud..." << std::endl;
    LSScanIterator<LSPointXYZRGB> scanIterator = scan->getScanIterator();

    std::vector<LSPointXYZRGB> points;
    points.reserve(1000000);

    uint64_t pointCount = 0;
    while (scanIterator.moveNext()) {
        points.push_back(scanIterator.getCurrent());
        ++pointCount;
        if (pointCount % 100000 == 0) {
            std::cout << "Collected " << pointCount << " points..." << std::endl;
        }
    }

    std::cout << "Total points collected: " << pointCount << std::endl;

    if (pointCount > 0) {
        savePointCloudToOFF(outputPath, points);
    }
    return outputPath;
}
