#include <iostream>
#include <filesystem>
#include <regex>
#include <string>
// 假设您已经安装了 fmt 库，这是一个非常优秀的 C++ 格式化库
// 如果没有，可以通过 vcpkg install fmt 安装
#include <fmt/format.h>

int main(int argc, char* argv[]) {
    try {
        // 获取程序所在的当前工作目录
        std::filesystem::path current_dir = std::filesystem::current_path();
        fmt::print("Current directory: {}\n", current_dir.string());

        // 获取当前目录的名称，用于构建新文件名
        std::string dir_name = current_dir.filename().string();
        fmt::print("Directory name: {}\n", dir_name);

        // 获取可执行文件本身的路径，以避免重命名程序自身
        std::filesystem::path exe_path;
        if (argc > 0) {
            exe_path = current_dir / std::filesystem::path(argv[0]).filename();
        }

        // 用于匹配 SxxExx 格式的正则表达式 (例如 S01E01, S1E1, S01E100)
        std::regex pattern(R"(S\d{1,3}E\d{1,3})", std::regex_constants::icase); // 添加 icase 使其不区分大小写

        // 遍历当前目录下的所有条目
        for (const auto& entry : std::filesystem::directory_iterator(current_dir)) {
            // 只处理常规文件，并且跳过可执行文件本身
            if (entry.is_regular_file() && (!exe_path.empty() && entry.path() != exe_path)) {
                std::string filename = entry.path().filename().string();
                std::smatch match;

                // 在文件名中搜索 SxxExx 模式
                if (std::regex_search(filename, match, pattern)) {
                    // 提取匹配到的季集部分 (例如 S01E01)
                    std::string season_episode = match[0];
                    std::string extension = entry.path().extension().string();
                    
                    // 构建基础的新文件名 (不包含后缀)
                    std::string base_new_filename = dir_name + "." + season_episode;
                    std::filesystem::path new_path = current_dir / (base_new_filename + extension);

                    // --- 关键修复：防止处理已重命名的文件 ---
                    // 如果计算出的新路径和当前文件路径完全一样，说明文件已经命名正确，直接跳过
                    if (entry.path() == new_path) {
                        fmt::print("文件已是正确格式，跳过: {}\n", filename);
                        continue; // 继续处理下一个文件
                    }

                    // --- 原有的优化逻辑 ---
                    // 检查目标文件是否已经存在
                    if (std::filesystem::exists(new_path)) {
                        try {
                            // 获取源文件和已存在文件的大小
                            uintmax_t source_size = std::filesystem::file_size(entry.path());
                            uintmax_t dest_size = std::filesystem::file_size(new_path);

                            // 如果大小相同，则认为是重复文件，删除源文件即可
                            if (source_size == dest_size) {
                                fmt::print("发现同名且同大小的重复文件，删除源文件: {}\n", filename);
                                std::filesystem::remove(entry.path());
                            } else {
                                // 如果大小不同，则添加后缀来重命名
                                int counter = 1;
                                std::filesystem::path unique_new_path;
                                do {
                                    // 构建带后缀的新文件名，例如: "文件名.(1).mkv"
                                    std::string unique_filename = fmt::format("{}({}){}", base_new_filename, counter++, extension);
                                    unique_new_path = current_dir / unique_filename;
                                } while (std::filesystem::exists(unique_new_path)); // 循环直到找到一个不存在的文件名

                                std::filesystem::rename(entry.path(), unique_new_path);
                                uintmax_t file_size_bytes = std::filesystem::file_size(unique_new_path);
                                double file_size_mb = static_cast<double>(file_size_bytes) / (1024 * 1024);
                                fmt::print("重命名(添加后缀): {} -> {} (大小: {:.2f} MB)\n", filename, unique_new_path.filename().string(), file_size_mb);
                            }
                        } catch (const std::filesystem::filesystem_error& fs_err) {
                            fmt::print(stderr, "处理文件冲突时出错 '{}': {}\n", filename, fs_err.what());
                        }
                    } else {
                        // 如果目标文件不存在，直接重命名
                        std::filesystem::rename(entry.path(), new_path);
                        uintmax_t file_size_bytes = std::filesystem::file_size(new_path);
                        double file_size_mb = static_cast<double>(file_size_bytes) / (1024 * 1024);
                        fmt::print("重命名: {} -> {} (大小: {:.2f} MB)\n", filename, new_path.filename().string(), file_size_mb);
                    }
                }
            }
        }
        fmt::print("\n所有文件处理完毕。\n");
    } catch (const std::filesystem::filesystem_error& e) {
        fmt::print(stderr, "文件系统错误: {}\n", e.what());
    } catch (const std::exception& e) {
        fmt::print(stderr, "发生错误: {}\n", e.what());
    }

    // 暂停程序，以便用户可以看到输出
    fmt::print("按 Enter键 退出...\n");
    std::cin.get();

    return 0;
}
