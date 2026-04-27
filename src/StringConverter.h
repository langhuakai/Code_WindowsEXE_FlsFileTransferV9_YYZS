// StringConverter.h
#pragma once

#include <string>
#include <memory>

class StringConverter
{
public:
    // 宽字符串转 string
    static std::string toString(const wchar_t* wstr);
    static std::string toString(const std::wstring& wstr);

    // string 转宽字符串
    static std::wstring toWstring(const std::string& str);

    // 智能指针版本 - 使用默认删除器
    static std::unique_ptr<wchar_t[]> toWcharPtr(const std::string& str);
    static std::unique_ptr<char[]> toCharPtr(const std::wstring& wstr);
};