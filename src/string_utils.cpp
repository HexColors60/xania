#include "string_utils.hpp"

#include <algorithm>
#include <cstring>

using namespace std::literals;

namespace {

int from_chars(std::string_view sv) {
    if (sv.empty())
        return 0;
    bool is_neg = sv.front() == '-';
    if (sv.front() == '-' || sv.front() == '+')
        sv.remove_prefix(1);
    if (sv.empty())
        return 0;
    unsigned long long intermediate = 0;
    for (auto c : sv) {
        if (!isdigit(c))
            return 0;
        intermediate = intermediate * 10u + (c - '0');
    }
    return static_cast<int>(intermediate) * (is_neg ? -1 : 1);
}

}

bool is_number(const char *arg) {
    if (*arg == '\0')
        return false;

    if (*arg == '+' || *arg == '-')
        arg++;

    for (; *arg != '\0'; arg++) {
        if (!isdigit(*arg))
            return false;
    }

    return true;
}

std::pair<int, const char *> number_argument(const char *argument) {
    std::string_view sv(argument);
    if (auto dot = sv.find_first_of('.'); dot != std::string_view::npos)
        return {from_chars(sv.substr(0, dot)), sv.substr(dot + 1).data()};
    return {1, argument};
}

int number_argument(const char *argument, char *arg) {
    // LEGACY FUNCTION TODO: remove
    auto &&[number, remainder] = number_argument(argument);
    strcpy(arg, remainder);
    return number;
}

std::string smash_tilde(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    std::transform(begin(str), end(str), std::back_inserter(result), [](char c) { return c == '~' ? '-' : c; });
    return result;
}

std::string skip_whitespace(std::string_view str) {
    return std::string(std::find_if(str.begin(), str.end(), [](auto ch) { return !std::isspace(ch); }), str.end());
}

std::string remove_last_line(std::string_view str) {
    static constexpr auto line_ending = "\n\r"sv;
    // If long enough to have a crlf...
    if (str.length() > line_ending.size()) {
        // If it ends with CRLF, ignore that in looking for a previous line.
        if (str.substr(str.length() - line_ending.size()) == line_ending)
            str.remove_suffix(2);
        if (auto last_crlf = str.rfind(line_ending); last_crlf != std::string_view::npos)
            return std::string(str.substr(0, last_crlf + line_ending.size()));
    }
    // Else, no crlf at all, return empty.
    return "";
}
