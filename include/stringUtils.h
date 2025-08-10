#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#pragma once
#include <string>
#include <string_view>

#include <utf8proc.h>

std::string utf8_fold(std::string_view s);

#endif   // STRINGUTILS_H
