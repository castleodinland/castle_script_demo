#include "IniParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

// The original, commented-out code block can be removed for clarity
// or left as is. It doesn't affect the final program.
#if 0
void IniParser::trim(std::string& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), str.end());
}

IniParser::Section IniParser::parseINI(const std::string& filename) {
    // ... (original implementation)
}
#endif

IniParser::IniParser(const std::string& filename) {
    // It's safer to open the file in binary mode to prevent any line ending translation,
    // although the Trim fix is the most direct solution.
    std::ifstream file(filename, std::ios::in);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open the INI file: " + filename);
    }
    Parse(file);
}

const IniParser::Section& IniParser::operator[](const std::string& section) const {
    return sections.at(section);
}

std::string IniParser::RemoveDuplicatePercent(std::string str) {
    size_t pos = 0;
    while ((pos = str.find("%%", pos)) != std::string::npos) {
        str.erase(pos, 1); // Remove one % where there are two %%
    }
    return str;
}

void IniParser::Parse(std::istream& stream) {
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed_line = Trim(line);
        if (trimmed_line.empty() || trimmed_line[0] == ';' || trimmed_line[0] == '#') {
            continue;  // Skip comments and empty lines
        }
        if (trimmed_line[0] == '[') {
            // New section
            size_t endpos = trimmed_line.find(']');
            if (endpos == std::string::npos) {
                // You might want to handle this error more gracefully
                continue; 
            }
            current_section = Trim(trimmed_line.substr(1, endpos - 1));
        } else {
            // Key-value pair
            size_t equal_pos = trimmed_line.find('=');
            std::string key = equal_pos == std::string::npos ? trimmed_line : trimmed_line.substr(0, equal_pos);
            std::string value = equal_pos == std::string::npos ? "" : trimmed_line.substr(equal_pos + 1);
            
            // The value should also be trimmed before processing
            value = Trim(value);
            value = RemoveDuplicatePercent(value);
            sections[current_section].push_back({Trim(key), value});
        }
    }
}

std::string IniParser::GetValue(const std::string& section, const std::string& key) const {
    auto sec_itr = sections.find(section);
    if (sec_itr != sections.end()) {
        const Section& sec = sec_itr->second;
        for (const auto& kv : sec) {
            if (kv.first == key) {
                return kv.second;
            }
        }
    }
    throw std::runtime_error("The key '" + key + "' or section '" + section + "' does not exist.");
}

std::vector<std::string> IniParser::GetGroup(const std::string& section) const {
    std::vector<std::string> output_vector;
    auto sec_itr = sections.find(section);
    if (sec_itr != sections.end()) {
        const Section& sec = sec_itr->second;
        for (const auto& kv : sec) {
            output_vector.push_back(kv.second);
        }
    }
    return output_vector;
}

std::string IniParser::Trim(const std::string& str) {
    // --- FIX ---
    // The original version only trimmed " \t".
    // This new version trims all standard whitespace characters,
    // including space, tab, newline, and importantly, carriage return (\r).
    const char* WHITESPACE = " \t\n\r\f\v";
    size_t first = str.find_first_not_of(WHITESPACE);
    if (first == std::string::npos) {
        return ""; // The string is all whitespace
    }
    size_t last = str.find_last_not_of(WHITESPACE);
    return str.substr(first, (last - first + 1));
}

bool IniParser::HasSection(const std::string& section) const {
    return sections.find(section) != sections.end();
}