//
// Created by Ytinifni-zc on 2018/03/22
//

#ifndef UTILS_SPLIT_HPP
#define UTILS_SPLIT_HPP

#include <vector>
#include <algorithm>
#include <string>
#include <iterator>

/**
 * Split string by a signle character.
 * @param s The string.
 * @param delimiter Single character delimiter
 * @return splited string.
 */
inline std::vector<std::string> string_split(std::string s, const char delimiter) {
    size_t start = 0;
    size_t end = s.find_first_of(delimiter);
    std::vector<std::string> output;

    while (end <= std::string::npos) {
        output.emplace_back(s.substr(start, end - start));

        if (end == std::string::npos)
            break;

        start = end + 1;
        end = s.find_first_of(delimiter, start);
    }
    return output;
}

#endif // UTILS_SPLIT_HPP
