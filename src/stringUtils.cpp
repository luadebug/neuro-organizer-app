#include "stringUtils.h"

std::string utf8_fold(std::string_view s) {
    utf8proc_uint8_t* out = nullptr;
    const auto len = utf8proc_map(
        reinterpret_cast<const utf8proc_uint8_t*>(s.data()), static_cast<utf8proc_ssize_t>(s.size()), &out,
        static_cast<utf8proc_option_t>(UTF8PROC_CASEFOLD | UTF8PROC_STABLE | UTF8PROC_COMPOSE));
    if (len < 0)
        return {};
    std::string result(reinterpret_cast<char*>(out), static_cast<size_t>(len));
    free(out);
    return result;
}
