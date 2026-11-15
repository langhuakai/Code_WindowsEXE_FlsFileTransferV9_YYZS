#include "Code_WindowsEXE_FlsFileTransferV9_YYZS.h"
#include "ArgumentParser.h"
#include "StringConverter.h"
#include "FlsToPly.h"
#include "FlsToOff.h"
#include "FlsToImage.h"

#include <iostream>
#include <filesystem>
#include <windows.h>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <utility>
#include <core_api/lsglobaldefinitions.h>
#include <core_api/lsproject.h>
#include <core_api/lsworkspace.h>
#include <core_api/lsobject.h>
#include <core_api/lsscan.h>
#include <core_api/lsprocessing.h>
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

static std::string getExeDir()
{
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    return fs::path(buf).parent_path().string();
}

static std::string loadLicenseKey()
{
    fs::path licensePath = fs::path(getExeDir()) / "LICENSE.txt";
    std::ifstream ifs(licensePath, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error: Cannot open license file: " << licensePath.string() << std::endl;
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    return content;
}

static std::string getTempProjectDir(const std::string& stem)
{
    return ".\\fls_transfer\\" + stem + "_" + std::to_string(GetCurrentProcessId());
}

// ──────────────────────────────────────────────
// Format mapping tables
// ──────────────────────────────────────────────
struct FormatEntry {
    const char* name;
    const char* ext;
    LSScan::ExportFormat exportFmt;
};

static const FormatEntry FORMAT_TABLE[] = {
    { "dxf", ".dxf", LSScan::DXF },
    { "e57", ".e57", LSScan::E57 },
    { "fls", ".fls", LSScan::FLS },
    { "igs", ".igs", LSScan::IGS },
    { "pod", ".pod", LSScan::POD },
    { "pts", ".pts", LSScan::PTS },
    { "ptx", ".ptx", LSScan::PTX },
    { "wrl", ".wrl", LSScan::WRL },
    { "xyb", ".xyb", LSScan::XYB },
    { "xyz", ".xyz", LSScan::XYZ },
    { "cpe", ".cpe", LSScan::CPE },
    { "bmp", ".bmp", LSScan::BMP },
    { "jpg", ".jpg", LSScan::JPG },
    { "png", ".png", LSScan::PNG },
    { "tif", ".tif", LSScan::TIF },
    { "las", ".las", LSScan::LAS },
    { "ply", ".ply", LSScan::DXF }, // not used, handled separately
    { "off", ".off", LSScan::DXF }, // not used, handled separately
};

static const size_t FORMAT_COUNT = sizeof(FORMAT_TABLE) / sizeof(FORMAT_TABLE[0]);

static const FormatEntry* lookupFormat(const std::string& name)
{
    for (size_t i = 0; i < FORMAT_COUNT; ++i) {
        if (name == FORMAT_TABLE[i].name)
            return &FORMAT_TABLE[i];
    }
    return nullptr;
}

// ──────────────────────────────────────────────
// Supported input formats
// ──────────────────────────────────────────────
static std::string toLower(const std::string& s)
{
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}

static bool isSupportedInputFile(const fs::path& path)
{
    return std::find(INPUT_EXTENSIONS.begin(), INPUT_EXTENSIONS.end(),
                     toLower(path.extension().string())) != INPUT_EXTENSIONS.end();
}

// 以 .fls 结尾的文件夹是 FARO 扫描仪工程目录, 当成单个 fls 文件处理
static bool isFlsProjectFolder(const fs::path& path)
{
    return toLower(path.filename().string()).ends_with(".fls");
}

// ──────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────
static bool removeDirectory(const LSString& path)
{
    try {
        if (fs::exists(path.toWcharStr())) {
            std::error_code ec;
            fs::remove_all(path.toWcharStr(), ec);
            if (ec) {
                std::cerr << "Warning: could not remove " << StringConverter::ansiToUtf8(path.toCharStr()) << ": " << ec.message() << std::endl;
                return false;
            }
#ifdef _WIN32
            _sleep(100);
#else
            usleep(100000);
#endif
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception removing directory: " << e.what() << std::endl;
        return false;
    }
}

static bool ensureParentDir(const LSString& path)
{
    try {
        fs::path parent = fs::path(path.toWcharStr()).parent_path();
        if (!parent.empty() && !fs::exists(parent)) {
            fs::create_directories(parent);
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create directory: " << e.what() << std::endl;
        return false;
    }
}

static uint64_t estimatePointCount(const ref_ptr<LSScan> scan)
{
    // Structured FARO scans: cols × rows gives an instant estimate
    int cols = scan->getNumCols();
    int rows = scan->getNumRows();
    if (cols > 0 && rows > 0) {
        return static_cast<uint64_t>(cols) * rows;
    }
    return 0;
}

static bool isImageFormat(const std::string& format)
{
    return format == "bmp" || format == "jpg" || format == "png" || format == "tif";
}

// 汇总里给出可读的错误名, 而不是光一个数字
static const char* resultName(LSResult::Result r)
{
    switch (r) {
    case LSResult::Ok:                  return "Ok";
    case LSResult::Internal:            return "Internal";
    case LSResult::Unauthorized:        return "Unauthorized";
    case LSResult::InvalidRequest:      return "InvalidRequest";
    case LSResult::DuplicateName:       return "DuplicateName";
    case LSResult::IncompatibleVersion: return "IncompatibleVersion";
    case LSResult::ReadWrite:           return "ReadWrite";
    case LSResult::Network:             return "Network";
    case LSResult::Compatibility:       return "Compatibility";
    }
    return "Unknown";
}

// 颜色与反射强度同时加载, 否则导出会丢掉反射强度; 不支持组合加载时回退 AUTO
static LSResult::Result loadScanDataForExport(const ref_ptr<LSScan>& scan)
{
    scan->unloadData();
    LSResult::Result result = scan->loadData(LSScan::COLOR_AND_REFLECTION);
    if (result == LSResult::Ok)
        return result;
    scan->unloadData();
    return scan->loadData(LSScan::AUTO);
}

// 返回主输出是否写成功; 附带的 png 失败只告警, 不影响返回值
// failReason 非空时回填失败原因, 供最终汇总打印
static bool exportScan(const ref_ptr<LSScan> scan, const std::string& outputPath,
                       const std::string& format, bool withImage,
                       std::string* imagePath = nullptr,
                       std::string* failReason = nullptr)
{
    const FormatEntry* entry = lookupFormat(format);
    if (!entry) {
        std::cerr << "Unsupported format: " << format << std::endl;
        if (failReason) *failReason = "unsupported format: " + format;
        return false;
    }

    bool ok = false;
    if (format == "ply") {
        ok = !FlsToPly::transToPlyRgb(scan, outputPath, failReason).empty();
    }
    else if (format == "off") {
        ok = !FlsToOff::transToOff(scan, outputPath, failReason).empty();
    }
    else {
        // Show estimated point count from scan metadata (zero-cost)
        uint64_t estimated = estimatePointCount(scan);
        if (estimated > 0) {
            std::cout << "Exporting ~" << estimated << " points to " << format << " format..." << std::endl;
        } else {
            std::cout << "Exporting to " << format << " format..." << std::endl;
        }

        LSResult::Result result = scan->exportData(
            LSString::fromCharStr(outputPath.c_str()), entry->exportFmt);
        ok = (result == LSResult::Ok);
        if (!ok) {
            std::cerr << "Failed to export to " << format
                      << ". Error code: " << result << std::endl;
            if (failReason) *failReason = std::string("exportData failed: ") + resultName(result);
        } else {
            std::cout << "Export completed: " << StringConverter::ansiToUtf8(outputPath) << std::endl;
        }
    }

    // 点云导出完成后额外输出一张同名 png 图片; 主输出本身就是图像时跳过
    if (withImage && !isImageFormat(format)) {
        std::string written = FlsToImage::exportPng(scan, outputPath);
        if (imagePath) {
            *imagePath = written;
        }
    }
    return ok;
}

// ──────────────────────────────────────────────
// Process a single input scan file → output file
// ──────────────────────────────────────────────
// 单个输入文件的结果: 是否全部成功、实际写出的文件、失败原因
struct FileResult {
    bool ok = false;
    std::vector<std::string> outputs;
    std::string failReason;
};

static FileResult processFile(const std::string& inputFile, const std::string& outputFile,
                              const std::string& format, bool withImage,
                              LSStandaloneContext* context)
{
    FileResult res;

    std::cout << "\n========================================\n";
    std::cout << "Processing: " << StringConverter::ansiToUtf8(inputFile) << std::endl;
    std::cout << "Output to:  " << StringConverter::ansiToUtf8(outputFile) << std::endl;
    std::cout << "========================================" << std::endl;

    // 输出目录可能尚不存在, 先建好再导入; 否则写点云时 CreateFileA 会报 error=3 (路径不存在)
    if (!ensureParentDir(LSString::fromCharStr(outputFile.c_str()))) {
        std::cerr << "Failed to create output directory for: "
                  << StringConverter::ansiToUtf8(outputFile) << std::endl;
        res.failReason = "cannot create output directory: " + outputFile;
        return res;
    }

    std::string stem = fs::path(inputFile).stem().string();
    std::string tempDir = getTempProjectDir(stem);
    LSString tempProjectPath = LSString::fromCharStr(tempDir.c_str());

    removeDirectory(tempProjectPath);
    ensureParentDir(tempProjectPath);

    LSString projectName = LSString::fromCharStr(("Import_" + stem).c_str());
    LSResult::Result result = LSProject::createProject(tempProjectPath, projectName);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to create project. Error code: " << result << std::endl;
        res.failReason = std::string("createProject failed: ") + resultName(result);
        return res;
    }

    ref_ptr<LSProject> project;
    result = LSProject::loadProject(tempProjectPath, project, false);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to load project. Error code: " << result << std::endl;
        res.failReason = std::string("loadProject failed: ") + resultName(result);
        return res;
    }
    context->setCurrentWorkspace(project);

    bool importSuccess = project->importData(LSString::fromCharStr(inputFile.c_str()));
    if (!importSuccess) {
        std::cerr << "Failed to import input file: " << StringConverter::ansiToUtf8(inputFile) << std::endl;
        project.reset();
        removeDirectory(tempProjectPath);
        res.failReason = "importData failed (unsupported or damaged input)";
        return res;
    }
    std::cout << "Input file imported successfully." << std::endl;

    result = project->saveRevision(L"Imported scan", L"API");
    if (result != LSResult::Ok) {
        std::cerr << "Warning: Failed to save project revision. Error code: " << result << std::endl;
    } else {
        std::cout << "Project saved successfully." << std::endl;
    }

    LSPod<std::vector<ref_ptr<LSScan>>> scans = project->getScans();
    if (scans.get().empty()) {
        std::cerr << "No scans found in project." << std::endl;
        project.reset();
        removeDirectory(tempProjectPath);
        res.failReason = "no scans found after import";
        return res;
    }
    std::cout << "Found " << scans.get().size() << " scan(s)." << std::endl;

    const FormatEntry* fmtEntry = lookupFormat(format);
    const char* ext = fmtEntry ? fmtEntry->ext : ".ply";

    bool multiScan = scans.get().size() > 1;
    std::string outBase = outputFile;
    if (multiScan) {
        outBase = outputFile.substr(0, outputFile.size() - fs::path(outputFile).extension().string().size());
    }

    bool allOk = true;
    for (size_t i = 0; i < scans.get().size(); ++i) {
        ref_ptr<LSScan> scan = scans.get()[i];
        std::cout << "\n--- Scan " << (i + 1) << " ---" << std::endl;

        // 多扫描时给原因加上序号, 单扫描不加噪音
        std::string tag = multiScan ? ("scan " + std::to_string(i + 1) + ": ") : "";

        // 尝试着色处理（需要项目已保存）
        LSResult::Result procResult = processScan(*scan, true);
        if (procResult == LSResult::Ok) {
            std::cout << "Scan processed (colorized)." << std::endl;
        } else {
            std::cout << "processScan: " << procResult
                      << ", trying colorizeScan..." << std::endl;
            LSResult::Result cr = colorizeScan(*scan);
            std::cout << "colorizeScan: " << cr << std::endl;
        }

        // 卸载后重新加载，获取处理后的数据（颜色 + 反射强度）
        result = loadScanDataForExport(scan);
        if (result != LSResult::Ok) {
            std::cerr << "Failed to load scan data. Error code: " << result << std::endl;
            if (!res.failReason.empty()) res.failReason += "; ";
            res.failReason += tag + "loadData failed: " + resultName(result);
            allOk = false;
            continue;
        }

        std::string scanOut = multiScan
            ? outBase + "_" + std::to_string(i + 1) + ext
            : outputFile;

        std::string why;
        if (exportScan(scan, scanOut, format, withImage, nullptr, &why)) {
            res.outputs.push_back(scanOut);
        } else {
            std::cerr << "Failed to export scan " << (i + 1) << " to: "
                      << StringConverter::ansiToUtf8(scanOut) << std::endl;
            if (!res.failReason.empty()) res.failReason += "; ";
            res.failReason += tag + "export to " + format + " failed: "
                            + (why.empty() ? std::string("unknown error") : why);
            allOk = false;
        }
        scan->unloadData();
    }

    project.reset();
    removeDirectory(tempProjectPath);
    res.ok = allOk;
    return res;
}

// ──────────────────────────────────────────────
// Console output helpers: 通过 WriteConsoleW 直接输出 Unicode,
// 避免控制台代码页 (chcp) 与字体导致的乱码
// ──────────────────────────────────────────────
// 注意: 这里刻意不调用 SetConsoleOutputCP(CP_UTF8)。
// 把控制台代码页切到 65001 在部分 Windows 控制台 (尤其非 TrueType 字体)
// 会导致中文整段乱码, 而 WriteConsoleW 直接写入 Unicode, 与代码页无关。
class ConsoleOutputBuf : public std::streambuf
{
public:
    explicit ConsoleOutputBuf(HANDLE consoleHandle)
        : m_handle(consoleHandle)
    {
    }

    ~ConsoleOutputBuf()
    {
        flush();
    }

protected:
    int_type overflow(int_type ch) override
    {
        if (!traits_type::eq_int_type(ch, traits_type::eof()))
        {
            m_pending.push_back(static_cast<char>(ch));
            if (ch == '\n')
                flush();
        }
        return traits_type::not_eof(ch);
    }

    int sync() override
    {
        flush();
        return 0;
    }

private:
    void flush()
    {
        if (m_pending.empty())
            return;

        int wideLen = ::MultiByteToWideChar(
            CP_UTF8, 0, m_pending.data(), static_cast<int>(m_pending.size()), nullptr, 0);
        if (wideLen > 0)
        {
            std::vector<wchar_t> wideBuf(wideLen);
            ::MultiByteToWideChar(CP_UTF8, 0,
                m_pending.data(), static_cast<int>(m_pending.size()),
                wideBuf.data(), wideLen);
            DWORD written = 0;
            if (::WriteConsoleW(m_handle, wideBuf.data(),
                    static_cast<DWORD>(wideLen), &written, nullptr))
            {
                m_pending.clear();
                return;
            }
        }

        // WriteConsoleW 失败时, 退回为原始字节输出, 避免丢内容
        DWORD written = 0;
        ::WriteFile(m_handle, m_pending.data(),
            static_cast<DWORD>(m_pending.size()), &written, nullptr);
        m_pending.clear();
    }

    HANDLE m_handle;
    std::vector<char> m_pending;
};

// 当 stdout/stderr 指向真实控制台时, 将其替换为 Unicode 输出流
static void installUnicodeConsoleOutput()
{
    const auto wrap = [](DWORD stdHandleId, std::basic_ostream<char>& stream)
    {
        HANDLE handle = ::GetStdHandle(stdHandleId);
        DWORD mode = 0;
        if (handle != INVALID_HANDLE_VALUE && ::GetConsoleMode(handle, &mode))
        {
            stream.rdbuf(new ConsoleOutputBuf(handle));
        }
    };

    wrap(STD_OUTPUT_HANDLE, std::cout);
    wrap(STD_ERROR_HANDLE, std::cerr);
}

// ──────────────────────────────────────────────
// Entry point
// ──────────────────────────────────────────────
int main(int argc, char* argv[])
{
    installUnicodeConsoleOutput();

    ArgumentParser parser;
    auto options = parser.parse(argc, argv);

    if (options.help)
    {
        ArgumentParser::showHelp(argv[0]);
        return 0;
    }

    if (!options.valid)
    {
        std::cerr << std::endl;
        ArgumentParser::showHelp(argv[0]);
        return 1;
    }

    if (!lookupFormat(options.format))
    {
        std::cerr << "错误: 不支持的格式 '" << options.format << "'" << std::endl;
        return 1;
    }

    // ── Collect input files ──
    std::vector<std::string> inputFiles;
    fs::path inputPath(options.inputFile);

    if (fs::is_directory(inputPath))
    {
        std::cout << "Scanning directory: " << StringConverter::ansiToUtf8(options.inputFile) << std::endl;

        // 输入本身就是一个 fls 工程文件夹时, 直接当成单个 fls 文件处理
        if (isFlsProjectFolder(inputPath))
        {
            inputFiles.push_back(inputPath.string());
        }
        else
        {
            for (const auto& entry : fs::directory_iterator(inputPath))
            {
                if (entry.is_directory())
                {
                    // fls 工程文件夹当成单个 fls 文件处理
                    if (isFlsProjectFolder(entry.path()))
                    {
                        inputFiles.push_back(entry.path().string());
                    }
                }
                else if (isSupportedInputFile(entry.path()))
                {
                    inputFiles.push_back(entry.path().string());
                }
            }
        }
        if (inputFiles.empty())
        {
            std::cerr << "No supported input file found in directory: "
                      << StringConverter::ansiToUtf8(options.inputFile) << std::endl;
            std::cerr << "Supported input formats: " << supportedInputFormatsText() << std::endl;
            return 1;
        }
        std::cout << "Found " << inputFiles.size() << " file(s) to process." << std::endl;
    }
    else if (fs::exists(inputPath))
    {
        if (!isSupportedInputFile(inputPath))
        {
            std::cerr << "Unsupported input format: " << StringConverter::ansiToUtf8(options.inputFile) << std::endl;
            std::cerr << "Supported input formats: " << supportedInputFormatsText() << std::endl;
            return 1;
        }
        inputFiles.push_back(options.inputFile);
    }
    else
    {
        std::cerr << "Input path does not exist: " << StringConverter::ansiToUtf8(options.inputFile) << std::endl;
        return 1;
    }

    // ── Create SCENE API context ──
    std::cout << "\nCreating SCENE API context..." << std::endl;
    std::string licenseKey = loadLicenseKey();
    if (licenseKey.empty()) {
        std::cerr << "Failed to load license key from LICENSE.txt" << std::endl;
        return 1;
    }
    LSStandaloneConfig config;
    config.setLicenseKey(LSString::fromCharStr(licenseKey.c_str()));
    config.setLoggingEnabled(true);
    LSStandaloneContext* context = new LSStandaloneContext(config);
    std::cout << "SCENE API Context created successfully." << std::endl;

    const FormatEntry* fmtEntry = lookupFormat(options.format);
    const char* ext = fmtEntry ? fmtEntry->ext : ".ply";
    if (options.withImage) {
        std::cout << "Image output enabled: an extra .png will be written next to the point cloud." << std::endl;
    }
    int successCount = 0;
    std::vector<std::pair<std::string, FileResult>> succeededFiles;
    std::vector<std::pair<std::string, FileResult>> failedFiles;

    // ── Process each input file ──
    for (const auto& inputFile : inputFiles)
    {
        std::string absInput = fs::absolute(inputFile).string();
        std::string outputPath;

        if (options.outputIsDir)
        {
            std::string stem = fs::path(absInput).stem().string();
            outputPath = options.outputPath + "\\" + stem + ext;
        }
        else
        {
            if (inputFiles.size() > 1)
            {
                std::cerr << "Single file output mode is not supported for multiple inputs." << std::endl;
                std::cerr << "Please use a directory for -o when batch processing." << std::endl;
                FileResult skipped;
                skipped.failReason = "single file output mode is not supported for multiple inputs, use a directory for -o";
                failedFiles.emplace_back(absInput, std::move(skipped));
                continue;
            }
            outputPath = options.outputPath;
        }

        std::string absOutput = fs::absolute(outputPath).string();
        FileResult result = processFile(absInput, absOutput, options.format, options.withImage, context);
        if (result.ok)
        {
            ++successCount;
            succeededFiles.emplace_back(absInput, std::move(result));
        }
        else
        {
            failedFiles.emplace_back(absInput, std::move(result));
        }
    }

    // ── Cleanup ──
    delete context;

    std::cout << "\nDone! Processed " << successCount << "/" << inputFiles.size() << " files." << std::endl;

    if (!succeededFiles.empty())
    {
        std::cout << "\nSucceeded (" << succeededFiles.size() << "):" << std::endl;
        for (const auto& entry : succeededFiles)
        {
            std::cout << "  " << StringConverter::ansiToUtf8(entry.first) << std::endl;
            for (const auto& out : entry.second.outputs)
            {
                std::cout << "    -> " << StringConverter::ansiToUtf8(out) << std::endl;
            }
        }
    }

    if (!failedFiles.empty())
    {
        std::cout << "\nFailed (" << failedFiles.size() << "):" << std::endl;
        for (const auto& entry : failedFiles)
        {
            std::cout << "  " << StringConverter::ansiToUtf8(entry.first) << std::endl;
            std::cout << "    reason: " << StringConverter::ansiToUtf8(entry.second.failReason) << std::endl;
            // 多扫描时可能部分成功, 把已经写出的文件也列出来
            for (const auto& out : entry.second.outputs)
            {
                std::cout << "    partial output: " << StringConverter::ansiToUtf8(out) << std::endl;
            }
        }
    }

    return (successCount == static_cast<int>(inputFiles.size())) ? 0 : 1;
}
