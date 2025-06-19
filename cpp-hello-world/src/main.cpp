#include <map>
#include <iostream>
#include <fmt/core.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <string>
#include <vector>

int main() {
    // boost::to_upper 示例
    std::string text = "hello, boost!";
    boost::to_upper(text); // 转为大写
    fmt::print("Boost upper: {}\n", text);

    // boost::regex 示例：提取IP地址
    std::vector<std::string> lines = {
        "user1 192.168.1.1 login",
        "no ip here",
        "server at 10.0.0.5",
        "another line 255.255.255.255",
        "invalid 999.999.999.999"
    };
    boost::regex ip_regex(R"((\b\d{1,3}(?:\.\d{1,3}){3}\b))");
    for (const auto& line : lines) {
        boost::smatch match;
        if (boost::regex_search(line, match, ip_regex)) {
            fmt::print("Found IP: {}\n", match[0].str());
        }
    }

    return 0;
}