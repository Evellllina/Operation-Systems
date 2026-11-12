#include "process.h"

std::string remove_vowels(const std::string& str) {
    const std::string vowels = "aeiouyAEIOUYаоуыэяёюеиАОУЫЭЯЁЮЕИ";
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        if (vowels.find(c) == std::string::npos) {
            result += c;
        }
    }
    return result;
}