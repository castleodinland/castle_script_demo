#ifndef INIPARSER_H
#define INIPARSER_H

#include <string>
#include <unordered_map>
#include <string>
#include <vector>
#include <map>
#include <utility>


#if 0
class IniParser {
public:
    using Section = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;
    Section parseINI(const std::string& filename);

private:
    void trim(std::string& str);
};
#endif

class IniParser {
public:
    using KeyValue = std::pair<std::string, std::string>;
    using Section = std::vector<KeyValue>;
    using SectionMap = std::unordered_map<std::string, Section>;

    explicit IniParser(const std::string& filename);
    const Section& operator[](const std::string& section) const;
    std::string GetValue(const std::string& section, const std::string& key) const;
    std::vector<std::string> GetGroup(const std::string& section) const;
    bool HasSection(const std::string& section) const;
    
private:
    SectionMap sections;
    std::string current_section;

    void Parse(std::istream& stream);
    static std::string Trim(const std::string& str);
    std::string RemoveDuplicatePercent(std::string str);
};

#endif // INIPARSER_H
