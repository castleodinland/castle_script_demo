#include <stdio.h>
#include <stdint.h> // 用于 uint64_t, uint8_t 等精确宽度整数类型
#include <stddef.h> // 用于 size_t 类型
#include <string.h> // 用于 strlen (仅在 main 函数示例中使用)
#include <iostream>
#include <string>
#include <vector>

// FNV-1a 算法 64位版本的核心常量
// FNV-1a 64-bit prime: 1099511628211
const uint64_t FNV_PRIME_64 = 1099511628211ULL;
// FNV-1a 64-bit offset basis: 14695981039346656037
const uint64_t FNV_OFFSET_BASIS_64 = 14695981039346656037ULL;

/**
 * @brief 使用 FNV-1a 算法计算给定数据的64位哈希值。
 * * @param data 指向需要计算哈希值的数据的指针。它可以是任何字节序列。
 * @param len  要计算哈希值的数据长度（以字节为单位）。
 * @return     返回一个64位（8字节）的哈希值。
 *
 * @note 这个函数是线程安全的（只要输入数据不被其他线程修改）。
 * @note 它不分配任何动态内存，非常适合嵌入式系统。
 */
uint64_t fnv1a_64(const void* data, size_t len) {
    // 将通用指针转换为字节指针，以便按字节处理
    const uint8_t* bytes = (const uint8_t*)data;

    // 1. 初始化哈希值为64位的偏移基准值
    uint64_t hash = FNV_OFFSET_BASIS_64;

    // 2. 遍历数据的每一个字节
    for (size_t i = 0; i < len; ++i) {
        // 2a. 先将当前字节与哈希值进行异或(XOR)操作
        hash ^= (uint64_t)bytes[i];
        
        // 2b. 再将结果乘以FNV的64位质数
        hash *= FNV_PRIME_64;
    }

    // 3. 返回最终计算得到的哈希值
    return hash;
}


// --- main 函数用于演示和测试 ---
int main() {

    std::vector<std::string> strs = {
        "MSG_MUSIC_BASS_DW",
        "MSG_MUSIC_TREB_DW",
        "MSG_MIC_BASS_DW",
        "MSG_EFFECT_BASS_DW",
        "MSG_MUSIC_TREB_DW",
        "MSG_MUSIC_BASS_DW",
        "MSG_MUSIC_EFFECT_DW",
        "MSG_MIC_EFFECT_DW",
    };

    for (const auto& str : strs) {
        std::cout << "str: " << str << ", fnv1a_64: " << fnv1a_64(str.c_str(), strlen(str.c_str())) << std::endl;
    }

    return 0;
}
