#include "FlsToOff.h"
#include "StringConverter.h"
#include <iomanip>
#include <sstream>

static BOOL offWriteAll(HANDLE hFile, const void* data, size_t totalBytes)
{
    const size_t CHUNK = 4 * 1024 * 1024;
    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t remaining = totalBytes;
    while (remaining > 0) {
        size_t toWrite = std::min(remaining, CHUNK);
        DWORD w = 0;
        if (!WriteFile(hFile, p, (DWORD)toWrite, &w, NULL) || w == 0) {
            std::cerr << "WriteFile FAILED at offset " << (p - static_cast<const uint8_t*>(data))
                      << " error=" << GetLastError() << std::endl;
            return FALSE;
        }
        p += w;
        remaining -= w;
    }
    return TRUE;
}

// 失败原因既打印到 stderr, 也回传给上层汇总
static bool offFail(std::string* failReason, const std::string& msg)
{
    if (failReason) *failReason = msg;
    std::cerr << msg << std::endl;
    return false;
}

bool FlsToOff::savePointCloudToOFF(const std::string& filename,
    const std::vector<LSPointXYZRGB>& points,
    std::string* failReason)
{
    if (failReason) failReason->clear();
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        return offFail(failReason, "Failed to open OFF file: " + StringConverter::ansiToUtf8(filename)
            + " error=" + std::to_string(err));
    }

    std::ostringstream oss;
    oss << "OFF\n" << points.size() << " 0 0\n";
    oss << std::fixed << std::setprecision(6);
    for (const auto& p : points) {
        oss << p.x << " " << p.y << " " << p.z << " "
            << static_cast<int>(p.r) << " "
            << static_cast<int>(p.g) << " "
            << static_cast<int>(p.b) << "\n";
    }

    std::string content = oss.str();
    BOOL ok = offWriteAll(hFile, content.data(), content.size());

    FlushFileBuffers(hFile);
    CloseHandle(hFile);
    if (!ok) {
        return offFail(failReason, "Failed to write OFF: " + StringConverter::ansiToUtf8(filename));
    }
    std::cout << "Saved " << points.size() << " points to OFF: " << StringConverter::ansiToUtf8(filename) << std::endl;
    return true;
}

std::string FlsToOff::transToOff(const ref_ptr<LSScan> scan, const std::string outputPath,
    std::string* failReason)
{
    if (failReason) failReason->clear();
    std::cout << "\nReading point cloud for OFF..." << std::endl;

    bool hasColor = scan->dataAvailable(LSScan::COLOR);
    bool hasReflection = scan->dataAvailable(LSScan::REFLECTION);

    // 有颜色用 RGB，无颜色用 SDK 对比度增强的灰度
    LSScanIterator<LSPointXYZRGB> scanIterator = scan->getScanIterator();
    std::vector<LSPointXYZRGB> points;
    points.reserve(1000000);
    uint64_t pointCount = 0;

    while (scanIterator.moveNext()) {
        points.push_back(scanIterator.getCurrent());
        ++pointCount;
        if (pointCount % 1000000 == 0)
            std::cout << "  " << pointCount << " points..." << std::endl;
    }

    std::cout << "Total: " << pointCount << " points." << std::endl;

    if (pointCount == 0) {
        offFail(failReason, "No points to export.");
        return std::string();
    }
    return savePointCloudToOFF(outputPath, points, failReason) ? outputPath : std::string();
}
