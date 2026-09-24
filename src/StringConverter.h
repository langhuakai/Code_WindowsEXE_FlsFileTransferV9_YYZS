// StringConverter.h
#pragma once

#include <string>
#include <memory>

class StringConverter
{
public:
    // ���ַ���ת string
    static std::string toString(const wchar_t* wstr);
    static std::string toString(const std::wstring& wstr);

    // string to wstring
    static std::wstring toWstring(const std::string& str);

    // Convert a system-ANSI (ACP) encoded narrow string to UTF-8 (for display)
    static std::string ansiToUtf8(const std::string& str);

    // ����ָ��汾 - ʹ��Ĭ��ɾ����
    static std::unique_ptr<wchar_t[]> toWcharPtr(const std::string& str);
    static std::unique_ptr<char[]> toCharPtr(const std::wstring& wstr);
};