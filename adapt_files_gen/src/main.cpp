#include <iostream>
#include <boost/algorithm/string/replace.hpp>
#include "fmt/format.h"
#include "fmt/core.h"

#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include <unordered_map>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace regex = boost::regex_constants;

// 全局变量定义保持不变
std::string input_dir = ".";
std::string output_dir = ".";

boost::regex re_flow_c_file("^user_effect_flow_(.*).c$");
boost::regex re_flow_source_item(
	"^\\s*\\{(.*_(SOURCE_.*))\\s*,\\s*\\d+\\s*,\\s*(BITS_16|BITS_24)\\s*,\\s*(CH_MONO|CH_STEREO).*\\},.*"
);
boost::regex re_flow_sink_item(
	"^\\s*\\{(.*_(SINK_.*))\\s*,\\s*\\d+\\s*,\\s*(BITS_16|BITS_24)\\s*,\\s*(CH_MONO|CH_STEREO).*\\},.*"
);


std::string template_device_table = "const roboeffect_adapt_device_table {}_adapt_device_table = \n{{\n";
std::string template_device_table_declare = "extern const roboeffect_adapt_device_table {}_adapt_device_table;\n";

std::string declaration_str = "";

// 兼容性修复：添加一个辅助函数来移除字符串两端的空白字符，包括 \r
// 这对于处理从 Windows 文件读取到 Linux 的行至关重要
std::string trim_whitespace(const std::string& str) {
    const char* WHITESPACE = " \t\n\r\f\v";
    size_t first = str.find_first_not_of(WHITESPACE);
    if (first == std::string::npos) {
        return ""; // 字符串全是空白
    }
    size_t last = str.find_last_not_of(WHITESPACE);
    return str.substr(first, (last - first + 1));
}


static std::string GET_ADAPT_HEADER_C() {
	// 使用 std::chrono 获取当前时间，更现代化和安全
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &in_time_t);
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
#else
    struct tm timeinfo;
    localtime_r(&in_time_t, &timeinfo);
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
#endif

	return fmt::format(R"(/*###############################################################################
# @file    roboeffect_adapt.c
# @author  castle (Automatic generated)
# @date    {}
# @brief   
# @attention
#
# THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
# WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
# TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
# INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
# FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
# CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
#
# <h2><center>&copy; COPYRIGHT 2023 MVSilicon </center></h2>
#/
###############################################################################
*/


#include "stdio.h"
#include "type.h"
#include "roboeffect_api.h"
#include "roboeffect_adapt.h"



)", ss.str());
}


static std::string GET_ADAPT_HEADER_H() {
	return fmt::format(R"(#ifndef __ROBOEFFECT_ADAPT_H__
#define __ROBOEFFECT_ADAPT_H__


#include "stdio.h"
#include "type.h"
#include "roboeffect_api.h"


typedef struct _roboeffect_adapt_device_node
{{
	uint32_t io_id;
	uint32_t width;
	uint32_t ch;
	char name[32];
}}roboeffect_adapt_device_node;


typedef struct _roboeffect_adapt_device_table
{{
	uint32_t count;
	const roboeffect_adapt_device_node table[];
}}roboeffect_adapt_device_table;

{}
#endif/*__ROBOEFFECT_ADAPT_H__*/

)", declaration_str);
}

// 兼容性修复：修改 join_paths 以使用 generic_string()
// 这可以确保返回的路径字符串在所有平台上都使用正斜杠 '/'
std::string join_paths(std::initializer_list<std::string> paths) {
    fs::path result;
    for (const auto& path : paths) {
        if (!path.empty()) {
            if (result.empty()) {
                result = path;
            } else {
                result /= path;
            }
        }
    }
    return result.generic_string();
}

std::string replay_string(std::string in_str, std::string rep, std::string sub)
{
	return boost::algorithm::replace_all_copy(in_str, rep, sub);
}

int main(int argc, char** argv) {


	fmt::print("adapt files generater, ver=1.2 (Linux compatible)\n");

	/* 检查命令行参数 */
    try {
        po::options_description desc("Options");
        desc.add_options()
            ("help,h", "show help informations")
            ("inputdir,i", po::value<std::string>(), "input processing folder")
            ("output,o", po::value<std::string>(), "output processing folder");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("inputdir")) {
			input_dir = vm["inputdir"].as<std::string>();
        }

        if (vm.count("output")) {
			output_dir = vm["output"].as<std::string>();
        }


    } catch(const po::error &ex) {
        fmt::print(stderr, "Error: {}\n", ex.what());
        return 1;
    }

    // 将输入和输出路径转换为绝对路径，以增加稳健性
    fs::path absolute_input_dir = fs::absolute(input_dir);
    fs::path absolute_output_dir = fs::absolute(output_dir);
    
    // 确保输出目录存在
    if (!fs::exists(absolute_output_dir)) {
        fs::create_directories(absolute_output_dir);
    }
    
    // 使用可移植的路径拼接来创建输出文件
	std::ofstream api_c_file( (absolute_output_dir / "roboeffect_adapt.c").string() );
	if (!api_c_file.is_open()) {
        fmt::print(stderr, "Error: Cannot open output file roboeffect_adapt.c for writing.\n");
        return 1;
    }
	api_c_file << GET_ADAPT_HEADER_C();


	if (fs::exists(absolute_input_dir) && fs::is_directory(absolute_input_dir))
	{
		for (auto& entry : fs::recursive_directory_iterator(absolute_input_dir))
		{
			if(fs::is_regular_file(entry.path()))
			{
				std::string file_name = entry.path().filename().string();

				boost::smatch match_results;
				if (boost::regex_match(file_name, match_results, re_flow_c_file)) {
					fs::path absolute_path = fs::canonical(entry.path());
					fmt::print("Processing: {}\n", absolute_path.generic_string());

					std::string graphic_name = match_results[1].str();

					api_c_file << fmt::format("#include \"{}\"\n", replay_string(file_name, ".c", ".h"));
					api_c_file << fmt::format(template_device_table, graphic_name);

					declaration_str += fmt::format(template_device_table_declare, graphic_name);

					std::string items;
					int items_counter = 0;

					std::ifstream file_input(absolute_path.string());
					if (!file_input.is_open()) {
                        fmt::print(stderr, "Warning: Could not open file {}. Skipping.\n", absolute_path.generic_string());
                        continue;
                    }

					std::string line;
					while(std::getline(file_input, line))
					{
                        // 兼容性修复：在进行正则匹配前，清理行尾的 \r 和其他空白
                        std::string trimmed_line = trim_whitespace(line);
                        
						if (boost::regex_match(trimmed_line, match_results, re_flow_source_item))
						{
							// fmt::print("Match source: {}\n", match_results[1].str());
							items += fmt::format("\t\t{{\n");
							items += fmt::format("\t\t\t{},//{}\n", replay_string(match_results[1].str(), " ", ""), "io_id");
							items += fmt::format("\t\t\t{},//{}\n", replay_string(match_results[3].str(), " ", ""), "width");
							items += fmt::format("\t\t\t{},//{}\n", replay_string(match_results[4].str(), " ", ""), "channel");
							items += fmt::format("\t\t\t\"{}\",//{}\n", replay_string(match_results[2].str(), " ", ""), "name");
							items += fmt::format("\t\t}},\n");
							items_counter ++;
						}

						if (boost::regex_match(trimmed_line, match_results, re_flow_sink_item))
						{
							// fmt::print("Match sink: {}\n", match_results[1].str());
							items += fmt::format("\t\t{{\n");
							items += fmt::format("\t\t\t{},//{}\n", replay_string(match_results[1].str(), " ", ""), "io_id");
							items += fmt::format("\t\t\t{},//{}\n", replay_string(match_results[3].str(), " ", ""), "width");
							items += fmt::format("\t\t\t{},//{}\n", replay_string(match_results[4].str(), " ", ""), "channel");
							items += fmt::format("\t\t\t\"{}\",//{}\n", replay_string(match_results[2].str(), " ", ""), "name");
							items += fmt::format("\t\t}},\n");
							items_counter ++;
						}
					}
                    file_input.close();

					api_c_file << fmt::format("\t{},\n", items_counter);
					api_c_file << fmt::format("\t{{\n{}\t}}\n", items);
					api_c_file << fmt::format("\n}};\n\n");
				}
			}
		}
	} else {
        fmt::print(stderr, "Error: Input directory '{}' does not exist or is not a directory.\n", input_dir);
        return 1;
    }

	api_c_file.close();

    // 再次使用可移植的路径拼接
	std::ofstream api_h_file((absolute_output_dir / "roboeffect_adapt.h").string());
    if (!api_h_file.is_open()) {
        fmt::print(stderr, "Error: Cannot open output file roboeffect_adapt.h for writing.\n");
        return 1;
    }
	api_h_file << GET_ADAPT_HEADER_H();
	api_h_file.close();

    fmt::print("Adapt files generation complete.\n");
    return 0;
}
