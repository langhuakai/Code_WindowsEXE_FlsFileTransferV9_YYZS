// FlsToPly.cpp
#include "FlsToPly.h"

// 将fls转换为ply点云（优先RGB，回退强度，最后纯坐标）
std::string FlsToPly::transToPlyRgb(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading point cloud..." << std::endl;

    bool hasColor = scan->dataAvailable(LSScan::COLOR);
    bool hasReflection = scan->dataAvailable(LSScan::REFLECTION);
    std::cout << "Color: " << (hasColor ? "Yes" : "No")
              << ", Reflection: " << (hasReflection ? "Yes" : "No") << std::endl;

    // 优先: 有真实颜色 -> RGB PLY
    if (hasColor) {
        std::cout << "Exporting RGB point cloud..." << std::endl;
        LSScanIterator<LSPointXYZRGB> scanIterator = scan->getScanIterator();
        std::vector<LSPointXYZRGB> points;
        points.reserve(1000000);
        uint64_t count = 0;
        while (scanIterator.moveNext()) {
            points.push_back(scanIterator.getCurrent());
            count++;
            if (count % 1000000 == 0)
                std::cout << "  " << count << " points..." << std::endl;
        }
        std::cout << "Total: " << count << " RGB points." << std::endl;
        if (count > 0) savePointCloudToPLY(outputPath, points, true);
        return outputPath;
    }

    // 其次: 有反射强度 -> 强度 PLY
    if (hasReflection) {
        std::cout << "No camera color. Exporting intensity PLY..." << std::endl;
        LSScanIterator<LSPointXYZI> reflIterator = scan->getScanReflectionIterator();
        std::vector<LSPointXYZI> points;
        points.reserve(1000000);
        uint64_t count = 0;
        while (reflIterator.moveNext()) {
            points.push_back(reflIterator.getCurrent());
            count++;
            if (count % 1000000 == 0)
                std::cout << "  " << count << " points..." << std::endl;
        }
        std::cout << "Total: " << count << " intensity points." << std::endl;
        if (count > 0) savePointCloudWithIntensityToPLY(outputPath, points, true);
        return outputPath;
    }

    // 最后: 纯坐标 PLY（无颜色无强度）
    std::cout << "No color or reflection. Exporting XYZ-only PLY..." << std::endl;
    LSScanIterator<LSPointXYZRGB> fallbackIterator = scan->getScanIterator();
    std::vector<LSPointXYZRGB> points;
    points.reserve(1000000);
    uint64_t count = 0;
    while (fallbackIterator.moveNext()) {
        points.push_back(fallbackIterator.getCurrent());
        count++;
    }
    std::cout << "Total: " << count << " points (no color)." << std::endl;
    if (count > 0) savePointCloudToPLY(outputPath, points, true);
    return outputPath;
}

// 将fls转换为带法向量的点云文件
std::string FlsToPly::transToPlyNormals(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving point cloud with normals..." << std::endl;
    LSScanIterator<LSPointXYZN> normalIterator = scan->getScanNormalsIterator();

    std::vector<LSPointXYZN> normalPoints;
    normalPoints.reserve(1000000);

    uint64_t pointCount = 0;
    while (normalIterator.moveNext()) {
        normalPoints.push_back(normalIterator.getCurrent());
        pointCount++;
        if (pointCount % 1000000 == 0)
            std::cout << "  " << pointCount << " points..." << std::endl;
    }

    std::cout << "Total points with normals: " << pointCount << std::endl;

    if (pointCount > 0) {
        savePointCloudWithNormalsToPLY(outputPath, normalPoints, true);
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
        intensityPoints.push_back(reflectionIterator.getCurrent());
        pointCount++;
        if (pointCount % 1000000 == 0)
            std::cout << "  " << pointCount << " points..." << std::endl;
    }

    std::cout << "Total points with reflection: " << pointCount << std::endl;

    if (pointCount > 0) {
        savePointCloudWithIntensityToPLY(outputPath, intensityPoints, true);
    }
    return outputPath;
}

// 将fls转换为球坐标文件
std::string FlsToPly::transToPlySpherical(const ref_ptr<LSScan> scan, const std::string outputPath)
{
    std::cout << "\nReading and saving spherical coordinates..." << std::endl;
    LSSphericalIterator<LSPointSphericalRGB> sphericalIterator = scan->getSphericalCoordinatesIterator();

    std::vector<LSPointXYZRGB> sphericalPoints;
    sphericalPoints.reserve(1000000);

    uint64_t pointCount = 0;
    while (sphericalIterator.moveNext()) {
        LSPointSphericalRGB sphericalPoint = sphericalIterator.getCurrent();

        double radius = sphericalPoint.radius;
        double phi = sphericalPoint.phi;
        double theta = sphericalPoint.theta;

        LSPointXYZRGB cartesianPoint;
        cartesianPoint.x = radius * sin(theta) * cos(phi);
        cartesianPoint.y = radius * sin(theta) * sin(phi);
        cartesianPoint.z = radius * cos(theta);
        cartesianPoint.r = sphericalPoint.r;
        cartesianPoint.g = sphericalPoint.g;
        cartesianPoint.b = sphericalPoint.b;

        sphericalPoints.push_back(cartesianPoint);
        pointCount++;

        if (pointCount % 1000000 == 0)
            std::cout << "  " << pointCount << " points..." << std::endl;
    }

    std::cout << "Total spherical points: " << pointCount << std::endl;

    if (pointCount > 0) {
        savePointCloudToPLY(outputPath, sphericalPoints, true);
    }
    return outputPath;
}

std::string FlsToPly::transToPlyProject(const ref_ptr<LSProject> project, const std::string outputPath)
{
    LSResult::Result result = project->createPointCloud();
    if (result == LSResult::Ok) {
        std::cout << "\nProject point cloud created successfully." << std::endl;

        ref_ptr<LSPointCloud> pointCloud = project->getPointCloud();
        if (pointCloud) {
            std::cout << "Reading and saving project point cloud..." << std::endl;

            LSIterator<LSPointXYZRGB> pcIterator = pointCloud->getPointCloudIterator();

            std::vector<LSPointXYZRGB> projectPoints;
            projectPoints.reserve(1000000);

            uint64_t pcPointCount = 0;
            while (pcIterator.moveNext()) {
                projectPoints.push_back(pcIterator.getCurrent());
                pcPointCount++;
                if (pcPointCount % 1000000 == 0)
                    std::cout << "  " << pcPointCount << " points..." << std::endl;
            }

            std::cout << "Total project points: " << pcPointCount << std::endl;

            if (pcPointCount > 0) {
                savePointCloudToPLY(outputPath, projectPoints, true);
            }
        }
    }
    return outputPath;
}

// ─── PLY 保存函数 ───

static std::string buildPlyHeader(bool binary,
    const char* extraProp1, const char* extraProp2, const char* extraProp3,
    size_t vertexCount)
{
    std::string hdr;
    hdr.append("ply\n");
    hdr.append(binary ? "format binary_little_endian 1.0\n" : "format ascii 1.0\n");
    hdr.append("element vertex ").append(std::to_string(vertexCount)).append("\n");
    hdr.append("property float x\n");
    hdr.append("property float y\n");
    hdr.append("property float z\n");
    if (extraProp1) hdr.append(extraProp1);
    if (extraProp2) hdr.append(extraProp2);
    if (extraProp3) hdr.append(extraProp3);
    hdr.append("end_header\n");
    return hdr;
}

static void verifyPlyFile(const std::string& filename, size_t vertexCount, size_t bytesPerVertex)
{
    std::error_code ec;
    auto fileSize = std::filesystem::file_size(std::filesystem::path(filename), ec);
    if (ec) {
        std::cerr << "Warning: cannot stat output file: " << filename << std::endl;
        return;
    }
    size_t hdrSize = 0;
    {
        std::ifstream hfile(filename, std::ios::binary);
        std::string allHdr;
        char buf[4096];
        while (hfile.good()) {
            hfile.read(buf, sizeof(buf));
            allHdr.append(buf, static_cast<size_t>(hfile.gcount()));
            if (allHdr.find("end_header\n") != std::string::npos) break;
        }
        hdrSize = allHdr.find("end_header\n");
        if (hdrSize != std::string::npos) hdrSize += strlen("end_header\n");
        else hdrSize = 0;
    }
    size_t expectedData = vertexCount * bytesPerVertex;
    size_t expectedTotal = hdrSize + expectedData;
    std::cout << "File size: " << fileSize << " bytes, expected ~" << expectedTotal
              << " bytes (header=" << hdrSize << ", data=" << expectedData << ")" << std::endl;
    if (fileSize < expectedTotal) {
        std::cerr << "ERROR: File is TRUNCATED by " << (expectedTotal - fileSize) << " bytes!" << std::endl;
    } else if (fileSize > expectedTotal) {
        std::cerr << "WARNING: File is larger than expected by " << (fileSize - expectedTotal) << " bytes" << std::endl;
    }
}

static BOOL plyWriteAll(HANDLE hFile, const void* data, size_t totalBytes)
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

bool FlsToPly::savePointCloudToPLY(const std::string& filename,
    const std::vector<LSPointXYZRGB>& points,
    bool binary) {
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open PLY file: " << filename << " error=" << GetLastError() << std::endl;
        return false;
    }

    std::string hdr = buildPlyHeader(binary, "property uchar red\n", "property uchar green\n", "property uchar blue\n", points.size());
    DWORD written = 0;
    WriteFile(hFile, hdr.data(), (DWORD)hdr.size(), &written, NULL);

    if (binary) {
        const size_t VERTEX_SIZE = 15;
        size_t totalBytes = points.size() * VERTEX_SIZE;
        std::vector<uint8_t> payload(totalBytes);
        for (size_t i = 0; i < points.size(); ++i) {
            size_t off = i * VERTEX_SIZE;
            float x = static_cast<float>(points[i].x);
            float y = static_cast<float>(points[i].y);
            float z = static_cast<float>(points[i].z);
            std::memcpy(&payload[off], &x, 4);
            std::memcpy(&payload[off + 4], &y, 4);
            std::memcpy(&payload[off + 8], &z, 4);
            payload[off + 12] = points[i].r;
            payload[off + 13] = points[i].g;
            payload[off + 14] = points[i].b;
        }
        plyWriteAll(hFile, payload.data(), totalBytes);
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6);
        for (const auto& p : points) {
            oss << p.x << " " << p.y << " " << p.z << " "
                << static_cast<int>(p.r) << " "
                << static_cast<int>(p.g) << " "
                << static_cast<int>(p.b) << "\n";
        }
        std::string ascii = oss.str();
        plyWriteAll(hFile, ascii.data(), ascii.size());
    }

    FlushFileBuffers(hFile);
    CloseHandle(hFile);
    verifyPlyFile(filename, points.size(), 15);
    std::cout << "Saved " << points.size() << " points to PLY: " << filename << std::endl;
    return true;
}

bool FlsToPly::savePointCloudWithNormalsToPLY(const std::string& filename,
    const std::vector<LSPointXYZN>& points,
    bool binary) {
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open PLY file: " << filename << " error=" << GetLastError() << std::endl;
        return false;
    }

    std::string hdr = buildPlyHeader(binary, "property float nx\n", "property float ny\n", "property float nz\n", points.size());
    DWORD written = 0;
    WriteFile(hFile, hdr.data(), (DWORD)hdr.size(), &written, NULL);

    if (binary) {
        const size_t VERTEX_SIZE = 24;
        size_t totalBytes = points.size() * VERTEX_SIZE;
        std::vector<uint8_t> payload(totalBytes);
        for (size_t i = 0; i < points.size(); ++i) {
            size_t off = i * VERTEX_SIZE;
            float x = static_cast<float>(points[i].x);
            float y = static_cast<float>(points[i].y);
            float z = static_cast<float>(points[i].z);
            float nx = static_cast<float>(points[i].nX);
            float ny = static_cast<float>(points[i].nY);
            float nz = static_cast<float>(points[i].nZ);
            std::memcpy(&payload[off],      &x,  4);
            std::memcpy(&payload[off + 4],  &y,  4);
            std::memcpy(&payload[off + 8],  &z,  4);
            std::memcpy(&payload[off + 12], &nx, 4);
            std::memcpy(&payload[off + 16], &ny, 4);
            std::memcpy(&payload[off + 20], &nz, 4);
        }
        plyWriteAll(hFile, payload.data(), totalBytes);
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6);
        for (const auto& p : points) {
            oss << p.x << " " << p.y << " " << p.z << " "
                << p.nX << " " << p.nY << " " << p.nZ << "\n";
        }
        std::string ascii = oss.str();
        plyWriteAll(hFile, ascii.data(), ascii.size());
    }

    FlushFileBuffers(hFile);
    CloseHandle(hFile);
    verifyPlyFile(filename, points.size(), 24);
    std::cout << "Saved " << points.size() << " points with normals to PLY: " << filename << std::endl;
    return true;
}

bool FlsToPly::savePointCloudWithIntensityToPLY(const std::string& filename,
    const std::vector<LSPointXYZI>& points,
    bool binary) {
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open PLY file: " << filename << " error=" << GetLastError() << std::endl;
        return false;
    }

    std::string hdr = buildPlyHeader(binary, "property float intensity\n", nullptr, nullptr, points.size());
    DWORD written = 0;
    WriteFile(hFile, hdr.data(), (DWORD)hdr.size(), &written, NULL);

    if (binary) {
        const size_t VERTEX_SIZE = 16;
        size_t totalBytes = points.size() * VERTEX_SIZE;
        std::vector<uint8_t> payload(totalBytes);
        for (size_t i = 0; i < points.size(); ++i) {
            size_t off = i * VERTEX_SIZE;
            float x = static_cast<float>(points[i].x);
            float y = static_cast<float>(points[i].y);
            float z = static_cast<float>(points[i].z);
            float intensity = points[i].i;
            std::memcpy(&payload[off],      &x,  4);
            std::memcpy(&payload[off + 4],  &y,  4);
            std::memcpy(&payload[off + 8],  &z,  4);
            std::memcpy(&payload[off + 12], &intensity, 4);
        }
        plyWriteAll(hFile, payload.data(), totalBytes);
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6);
        for (const auto& p : points) {
            oss << p.x << " " << p.y << " " << p.z << " " << p.i << "\n";
        }
        std::string ascii = oss.str();
        plyWriteAll(hFile, ascii.data(), ascii.size());
    }

    FlushFileBuffers(hFile);
    CloseHandle(hFile);
    verifyPlyFile(filename, points.size(), 16);
    std::cout << "Saved " << points.size() << " points with intensity to PLY: " << filename << std::endl;
    return true;
}
