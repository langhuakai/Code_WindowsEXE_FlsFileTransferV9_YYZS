// StringConverter.cpp
#include "StringConverter.h"
#include <windows.h>
#include <cstring>
#include <memory>

// 1. wchar_t* ת string
std::string StringConverter::toString(const wchar_t* wstr)
{
    if (!wstr) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len == 0) return "";

    std::string str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], len, NULL, NULL);

    str.pop_back();
    return str;
}

// 2. wstring ת string
std::string StringConverter::toString(const std::wstring& wstr)
{
    if (wstr.empty()) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
        NULL, 0, NULL, NULL);
    if (len == 0) return "";

    std::string str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
        &str[0], len, NULL, NULL);

    return str;
}

// 3. string ת wstring
std::wstring StringConverter::toWstring(const std::string& str)
{
    if (str.empty()) return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(),
        NULL, 0);
    if (len == 0) return L"";

    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(),
        &wstr[0], len);

    return wstr;
}

// system ANSI (ACP) to UTF-8
std::string StringConverter::ansiToUtf8(const std::string& str)
{
    if (str.empty()) return "";

    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(),
        NULL, 0);
    if (len == 0) return str;

    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(),
        &wstr[0], len);

    return toString(wstr);
}

// 4. string ת wchar_t* (����ָ��)
std::unique_ptr<wchar_t[]> StringConverter::toWcharPtr(const std::string& str)
{
    if (str.empty())
    {
        return std::unique_ptr<wchar_t[]>(nullptr);
    }

    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size() + 1,
        NULL, 0);
    if (len == 0) return nullptr;

    std::unique_ptr<wchar_t[]> wstr(new wchar_t[len]);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size() + 1,
        wstr.get(), len);

    return wstr;
}

// 5. wstring ת char* (����ָ��)
std::unique_ptr<char[]> StringConverter::toCharPtr(const std::wstring& wstr)
{
    if (wstr.empty())
    {
        return std::unique_ptr<char[]>(nullptr);
    }

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size() + 1,
        NULL, 0, NULL, NULL);
    if (len == 0) return nullptr;

    std::unique_ptr<char[]> str(new char[len]);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size() + 1,
        str.get(), len, NULL, NULL);

    return str;
}