#pragma once

#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

#ifndef UNICODE
#define _TEXT_PASTE(Quote) Quote
#else
#define _TEXT_PASTE(Quote) L##Quote
#endif // !UNICODE

#ifndef AUTO_TEXT
#define AUTO_TEXT(Quote) _TEXT_PASTE(Quote)
#endif // !TEXT

#include <cassert>
#define _check_2(expression, msg) \
    {                                                                                                                                                          \
        if (!(expression))                                                                                                                                     \
        {                                                                                                                                                      \
            std::cerr << "msg:" << msg << "\n"                                                                                                             \
                      << "file: " << __FILE__ << "\n"                                                                                                          \
                      << "line: " << __LINE__ << "\n";                                                                                                         \
            throw std::runtime_error(msg);                                                                                                                 \
        }                                                                                                                                                      \
    }

#define _check_1(expression) _check_2(expression, "Check failed.")

#define _check(_1, _2, _check_macro, ...) _check_macro

#define check(...) _check(__VA_ARGS__, _check_2, _check_1)(__VA_ARGS__)


#include "MemoryManager.h"
#include "BasicTypes.h"
#include "Platform.h"

#include "Containers/Container.h"
