#include "Code_WindowsEXE_FlsFileTransferV9_YYZS.h"
#include "ArgumentParser.h"
#include "StringConverter.h"
#include "FlsToPly.h"
#include "FlsToOff.h"

#include <iostream>
#include <filesystem>
#include <windows.h>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
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

static void exportScan(const ref_ptr<LSScan> scan, const std::string& outputPath, const std::string& format)
{
    const FormatEntry* entry = lookupFormat(format);
    if (!entry) {
        std::cerr << "Unsupported format: " << format << std::endl;
        return;
    }

    if (format == "ply") {
        FlsToPly::transToPlyRgb(scan, outputPath);
        return;
    }

    if (format == "off") {
        FlsToOff::transToOff(scan, outputPath);
        return;
    }

    // Show estimated point count from scan metadata (zero-cost)
    uint64_t estimated = estimatePointCount(scan);
    if (estimated > 0) {
        std::cout << "Exporting ~" << estimated << " points to " << format << " format..." << std::endl;
    } else {
        std::cout << "Exporting to " << format << " format..." << std::endl;
    }

    LSResult::Result result = scan->exportData(
        LSString::fromCharStr(outputPath.c_str()), entry->exportFmt);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to export to " << format
                  << ". Error code: " << result << std::endl;
    } else {
        std::cout << "Export completed: " << StringConverter::ansiToUtf8(outputPath) << std::endl;
    }
}

// ──────────────────────────────────────────────
// Process a single FLS file → output file
// ──────────────────────────────────────────────
static bool processFile(const std::string& inputFile, const std::string& outputFile,
                        const std::string& format, LSStandaloneContext* context)
{
    std::cout << "\n========================================\n";
    std::cout << "Processing: " << StringConverter::ansiToUtf8(inputFile) << std::endl;
    std::cout << "Output to:  " << StringConverter::ansiToUtf8(outputFile) << std::endl;
    std::cout << "========================================" << std::endl;

    std::string stem = fs::path(inputFile).stem().string();
    std::string tempDir = getTempProjectDir(stem);
    LSString tempProjectPath = LSString::fromCharStr(tempDir.c_str());

    removeDirectory(tempProjectPath);
    ensureParentDir(tempProjectPath);

    LSString projectName = LSString::fromCharStr(("Import_" + stem).c_str());
    LSResult::Result result = LSProject::createProject(tempProjectPath, projectName);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to create project. Error code: " << result << std::endl;
        return false;
    }

    ref_ptr<LSProject> project;
    result = LSProject::loadProject(tempProjectPath, project, false);
    if (result != LSResult::Ok) {
        std::cerr << "Failed to load project. Error code: " << result << std::endl;
        return false;
    }
    context->setCurrentWorkspace(project);

    bool importSuccess = project->importData(LSString::fromCharStr(inputFile.c_str()));
    if (!importSuccess) {
        std::cerr << "Failed to import FLS file." << std::endl;
        project.reset();
        removeDirectory(tempProjectPath);
        return false;
    }
    std::cout << "FLS file imported successfully." << std::endl;

    result = project->saveRevision(L"Imported FLS", L"API");
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
        return false;
    }
    std::cout << "Found " << scans.get().size() << " scan(s)." << std::endl;

    const FormatEntry* fmtEntry = lookupFormat(format);
    const char* ext = fmtEntry ? fmtEntry->ext : ".ply";

    bool multiScan = scans.get().size() > 1;
    std::string outBase = outputFile;
    if (multiScan) {
        outBase = outputFile.substr(0, outputFile.size() - fs::path(outputFile).extension().string().size());
    }

    for (size_t i = 0; i < scans.get().size(); ++i) {
        ref_ptr<LSScan> scan = scans.get()[i];
        std::cout << "\n--- Scan " << (i + 1) << " ---" << std::endl;

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

        // 卸载后重新加载，获取处理后的数据
        scan->unloadData();
        result = scan->loadData(LSScan::AUTO);
        if (result != LSResult::Ok) {
            std::cerr << "Failed to load scan data. Error code: " << result << std::endl;
            continue;
        }

        std::string scanOut = multiScan
            ? outBase + "_" + std::to_string(i + 1) + ext
            : outputFile;

        exportScan(scan, scanOut, format);
        scan->unloadData();
    }

    project.reset();
    removeDirectory(tempProjectPath);
    return true;
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

        // 输入本身就是一个以 .fls 结尾的文件夹时, 直接当成单个 fls 文件处理
        std::string inputLower = inputPath.filename().string();
        std::transform(inputLower.begin(), inputLower.end(), inputLower.begin(), ::tolower);
        bool inputIsFlsFolder = inputLower.ends_with(".fls");
        if (inputIsFlsFolder)
        {
            inputFiles.push_back(inputPath.string());
        }
        else
        {
            for (const auto& entry : fs::directory_iterator(inputPath))
            {
                if (entry.is_directory())
                {
                    // 以 .fls 结尾的文件夹, 当成单个 fls 文件处理
                    std::string name = entry.path().filename().string();
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    if (name.ends_with(".fls"))
                    {
                        inputFiles.push_back(entry.path().string());
                    }
                }
                else
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".fls" || ext == ".scn")
                    {
                        inputFiles.push_back(entry.path().string());
                    }
                }
            }
        }
        if (inputFiles.empty())
        {
            std::cerr << "No FLS/SCN files found in directory: " << StringConverter::ansiToUtf8(options.inputFile) << std::endl;
            return 1;
        }
        std::cout << "Found " << inputFiles.size() << " file(s) to process." << std::endl;
    }
    else if (fs::exists(inputPath))
    {
        inputFiles.push_back(options.inputFile);
    }
    else
    {
        std::cerr << "Input path does not exist: " << StringConverter::ansiToUtf8(options.inputFile) << std::endl;
        return 1;
    }

    // ── Create SCENE API context ──
    std::cout << "\nCreating SCENE API context..." << std::endl;
    LSStandaloneConfig config;
    config.setLicenseKey(apiKey);
    config.setLoggingEnabled(true);
    LSStandaloneContext* context = new LSStandaloneContext(config);
    std::cout << "SCENE API Context created successfully." << std::endl;

    const FormatEntry* fmtEntry = lookupFormat(options.format);
    const char* ext = fmtEntry ? fmtEntry->ext : ".ply";
    int successCount = 0;
    std::vector<std::string> convertedFiles;

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
                continue;
            }
            outputPath = options.outputPath;
        }

        std::string absOutput = fs::absolute(outputPath).string();
        if (processFile(absInput, absOutput, options.format, context))
        {
            ++successCount;
            convertedFiles.push_back(absOutput);
        }
    }

    // ── Cleanup ──
    delete context;

    std::cout << "\nDone! Processed " << successCount << "/" << inputFiles.size() << " files." << std::endl;

    if (!convertedFiles.empty())
    {
        std::cout << "\nConverted files:" << std::endl;
        for (const auto& f : convertedFiles)
        {
            std::cout << "  " << StringConverter::ansiToUtf8(f) << std::endl;
        }
    }

    return (successCount == static_cast<int>(inputFiles.size())) ? 0 : 1;
}
