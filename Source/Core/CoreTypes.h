#pragma once

#include <iostream>
#include <fstream>

#include <functional>
#include <optional>

using AnsiChar = char;
using WideChar = wchar_t;

#ifndef UNICODE
using AChar = char;
using AString = std::string;
using AStringStream = std::stringstream;
using AFileStream = std::fstream;
#else
using AChar = wchar_t;
using AString = std::wstring;
using AStringStream = std::wstringstream;
using AFileStream = std::wfstream;
#endif // !UNICODE

template <typename Fty>
using TFunction = std::function<Fty>;

template <typename Ty>
using TOptional = std::optional<Ty>;
