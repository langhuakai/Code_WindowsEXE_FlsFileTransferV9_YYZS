#include "FlsToOff.h"
#include <cmath>
#include <algorithm>

// 将 [0,1] 强度值映射为伪彩色 (jet色谱)
static void intensityToColor(float t, uint8_t& r, uint8_t& g, uint8_t& b)
{
    t = std::max(0.0f, std::min(1.0f, t));
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

    bool hasColor = scan->dataAvailable(LSScan::COLOR);
    bool hasReflection = scan->dataAvailable(LSScan::REFLECTION);

    if (!hasColor && !hasReflection) {
        std::cerr << "Warning: No color or reflection data available for this scan." << std::endl;
        return "";
    }

    std::vector<LSPointXYZRGB> points;
    points.reserve(1000000);
    uint64_t pointCount = 0;

    if (hasColor) {
        std::cout << "Using real camera RGB data." << std::endl;
    } else {
        std::cout << "No camera color. Will apply false-color map." << std::endl;
    }

    LSScanIterator<LSPointXYZRGB> scanIterator = scan->getScanIterator();
    while (scanIterator.moveNext()) {
        points.push_back(scanIterator.getCurrent());
        ++pointCount;
        if (pointCount % 100000 == 0) {
            std::cout << "Collected " << pointCount << " points..." << std::endl;
        }
    }

    if (!hasColor && pointCount > 0) {
        std::cout << "Applying false-color map to contrast-enhanced grayscale..." << std::endl;
        for (auto& pt : points) {
            float gray = (static_cast<float>(pt.r) + static_cast<float>(pt.g) + static_cast<float>(pt.b)) / 3.0f / 255.0f;
            intensityToColor(gray, pt.r, pt.g, pt.b);
        }
    }

    std::cout << "Total points collected: " << pointCount << std::endl;

    if (pointCount > 0) {
        savePointCloudToOFF(outputPath, points);
    }
    return outputPath;
}
