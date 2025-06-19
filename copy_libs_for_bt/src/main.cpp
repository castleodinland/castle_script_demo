#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <map>
#include <unordered_map>
#include <stdexcept> // For runtime_error
#include <fmt/core.h>
#include <fmt/chrono.h>

#include <vector> // For example usage
#include <cctype> // For std::isspace, std::isalpha, std::isalnum
#include <stack>
#include <filesystem>  // For path operations (C++17)
#include <fstream>     // For file operations
#include <regex> // Standard Library regex
#include <algorithm> // For std::transform and std::toupper


// +------------------------------------------------------------------+
// | Utility Functions (replacing boost::algorithm)                   |
// +------------------------------------------------------------------+

#define VERSION_PLUS //support express as MACRO ><=!
// #define DEBUG_EXPRESS

//namespace fs = boost::filesystem;
// Use the C++17 standard filesystem library
namespace fs = std::filesystem;
namespace ba = boost::algorithm;
namespace pt = boost::property_tree;

using namespace std;

const std::string ver = "v3.1.1";
int onoff = 0;

/**
 * @brief Case-insensitive string comparison.
 * @param a The first string.
 * @param b The second string.
 * @return True if the strings are equal ignoring case, false otherwise.
 */
bool iequals(const std::string& a, const std::string& b) {
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
}

/**
 * @brief Trims whitespace from both ends of a string, in-place.
 * @param s The string to trim.
 */
void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::vector<std::string> define_libs_list;
std::string default_dest_lib;
std::string default_lib;
fs::path home_dir;
fs::path ini_config_file = "script.ini";
bool force_runall = false;

// std::map<std::string, int> global_vars;

void usage() {
    std::cout << "\nCopyLibs Tool " << ver << " for MVS BT_audio SDK\n";
    std::cout << "Written by CPP\n";
    std::cout << "Usage:\n";
    std::cout << "-s, --script: specify ini file as copy script\n";
}

std::string trim(const std::string& s) {
    auto wsfront = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto wsback = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (wsback <= wsfront) ? std::string() : std::string(wsfront, wsback);
}

void force_copy_file(const fs::path& src, const fs::path& dest) {
    // 处理路径字符串中的空格
    std::string src_str = src.string();
    std::string dest_str = dest.string();
    
    // 去除首尾空格（使用Boost的trim）
    ba::trim(src_str);
    ba::trim(dest_str);
    
    // 去除所有空格（如果需要）
    // ba::erase_all(src_str, " ");
    // ba::erase_all(dest_str, " ");
    
    // 转换回路径对象
    fs::path trimmed_src(src_str);
    fs::path trimmed_dest(dest_str);

    if (!fs::exists(trimmed_src)) {
        std::cerr << "File " << trimmed_src << " does not exist, exiting.\n";
        exit(1);
    }
    try {
        fs::copy_file(trimmed_src, trimmed_dest, fs::copy_options::overwrite_existing);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Copy failed: " << e.what() << "\n";
        exit(1);
    }
}



// +------------------------------------------------------------------+
// | ✨ 新类: DefineValueParser                                       |
// | 这个自包含的解析器用于处理宏定义中的算术表达式。                     |
// +------------------------------------------------------------------+
class DefineValueParser {
private:

    struct ValuePair {
        std::string str_value;
        int int_value;
    };
    // The global_vars map is now a private member of the class.
    std::map<std::string, int> m_vars;
    std::unordered_map<std::string, ValuePair> m_vars_mirror;

    
    // Internal iterator state for expression evaluation
    std::string::const_iterator it;
    std::string::const_iterator end;

    // --- Private methods for expression evaluation ---

    // 递归替换宏，以处理嵌套定义的情况
    std::string substitute_macros(std::string expr, const std::map<std::string, int>& macros) {
        // 限制迭代次数以防止无限循环
        for (int i = 0; i < 5; ++i) {
            bool replaced = false;
            for (const auto& pair : macros) {
                // Use std::regex for robust whole-word replacement
                std::regex word_regex(fmt::format(R"(\b{}\b)", pair.first));
                std::string new_expr = std::regex_replace(expr, word_regex, std::to_string(pair.second));
                if (new_expr != expr) {
                    expr = new_expr;
                    replaced = true;
                }
            }
            if (!replaced) break;
        }
        return expr;
    }

    // 跳过空白字符
    void skip_whitespace() {
        while (it != end && std::isspace(*it)) {
            ++it;
        }
    }

    // 解析一个数字（支持十进制和十六进制）
    int parse_number() {
        skip_whitespace();
        std::string num_str;
        // 支持十六进制 '0x' 前缀
        if (std::distance(it, end) >= 2 && *it == '0' && (*(it + 1) == 'x' || *(it + 1) == 'X')) {
            num_str += *it++;
            num_str += *it++;
        }
        while (it != end && (std::isalnum(*it))) {
            num_str += *it++;
        }
        if (num_str.empty()) {
            throw std::runtime_error("Expected a number");
        }
        // 使用 std::stoi 并将基数设为0，以自动检测进制
        return std::stoi(num_str, nullptr, 0);
    }

    // 解析一个因子（数字或带括号的表达式）
    int parse_factor() {
        skip_whitespace();
        if (it != end && *it == '(') {
            ++it; // 消耗 '('
            int result = parse_expression();
            skip_whitespace();
            if (it == end || *it != ')') {
                throw std::runtime_error("Mismatched parentheses: expected ')'");
            }
            ++it; // 消耗 ')'
            return result;
        }
        return parse_number();
    }

    // 解析一个项（处理 * 和 /）
    int parse_term() {
        int left = parse_factor();
        while (true) {
            skip_whitespace();
            if (it != end && *it == '*') {
                ++it; // 消耗 '*'
                left *= parse_factor();
            } else if (it != end && *it == '/') {
                ++it; // 消耗 '/'
                int right = parse_factor();
                if (right == 0) throw std::runtime_error("Division by zero");
                left /= right;
            } else {
                break;
            }
        }
        return left;
    }
    
    // 解析一个完整的表达式（处理 + 和 -）
    int parse_expression() {
        int left = parse_term();
        while (true) {
            skip_whitespace();
            if (it != end && *it == '+') {
                ++it; // 消耗 '+'
                left += parse_term();
            } else if (it != end && *it == '-') {
                ++it; // 消耗 '-'
                left -= parse_term();
            } else {
                break;
            }
        }
        return left;
    }


public:
    /**
     * @brief 解析表达式字符串并计算其整数值。(Stable Interface, Unchanged)
     * @param expr 待解析的表达式字符串。
     * @param macros 一个包含已知宏及其值的map，用于替换。
     * @return 计算出的整数结果。
     */
    int evaluate(const std::string& expr, const std::map<std::string, int>& macros) {
        // 首先，用已知宏的值替换表达式中的宏名称。
        std::string substituted_expr = substitute_macros(expr, macros);
        this->it = substituted_expr.begin();
        this->end = substituted_expr.end();
        try {
            int result = parse_expression();
            skip_whitespace();
            // 确保整个字符串都被消耗完毕
            if (it != end) {
                 throw std::runtime_error("Unexpected characters at the end of expression");
            }
            return result;
        } catch (const std::exception& e) {
            // 重新抛出异常，并附加上下文信息
            throw std::runtime_error(fmt::format("Failed to evaluate expression '{}': {}", expr, e.what()));
        }
    }

    /**
     * @brief 从一个多行字符串中解析 #define 语句。 (New function, integrated from the old global function)
     * @param express_define 包含宏定义的字符串。
     */
    void parse_defines_data(const std::string& express_define) {
        std::istringstream stream(express_define);
        std::string line;
        std::smatch match;

        while (std::getline(stream, line)) {
            auto comment_pos = line.find("//");
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }

            for (const auto& macro_name : define_libs_list) {
                // Use std::regex instead of boost::regex
                std::regex re_define(fmt::format(R"(^\s*#define\s+{}\b(?:\s+(.*))?$)", macro_name));

                if (std::regex_search(line, match, re_define)) {
                    int value = 1; // Default value if no value part is present

                    if (match[1].matched) {
                        std::string val_str = match[1].str();
                        trim(val_str); // Use our own trim function

                        if (val_str.empty()) {
                            value = 1;
                        } else if (iequals(val_str, "ENABLE")) { // Use our own iequals
                            value = 1;
                        } else if (iequals(val_str, "DISABLE")) {
                            value = 0;
                        } else {
                            try {
                               value = std::stoi(val_str, nullptr, 0);
                            } catch (const std::invalid_argument&) {
                                try {
                                    // Use the class's own evaluate method, and its own variable map
                                    value = this->evaluate(val_str, m_vars);
                                } catch (const std::exception& e) {
                                    fmt::print(stderr, "Warning: Could not parse value for macro {}. Value '{}' is not a valid integer, alias, or expression. Details: {}. Defaulting to 0.\n", macro_name, val_str, e.what());
                                    value = 0;
                                }
                            } catch (const std::out_of_range&) {
                                fmt::print(stderr, "Warning: Value '{}' for macro {} is out of integer range. Defaulting to 0.\n", val_str, macro_name);
                                value = 0;
                            }
                        }
                    }
                    
                    m_vars[macro_name] = value; // Store result in the private member

                    auto string_temp = match[0].str();
                    trim(string_temp); // Use our own trim function
                    m_vars_mirror[string_temp] = {macro_name, value};
                    break; 
                }
            }
        }
    }

    /**
     * @brief 从一个文件中解析 #define 语句。(New overloaded function)
     * @param file_path 指向包含宏定义文件的路径。
     */
    void parse_defines_from_file(const fs::path& file_path) {
        if (!fs::exists(file_path)) {
            throw std::runtime_error(fmt::format("File does not exist: {}", file_path.string()));
        }
        std::ifstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error(fmt::format("Failed to open file: {}", file_path.string()));
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        parse_defines_data(buffer.str());
    }

    /**
     * @brief 获取所有已解析的宏变量。(New accessor)
     * @return 一个包含所有宏及其值的常量引用map。
     */
    const std::map<std::string, int>& get_all_vars() const {
        return m_vars;
    }

    // 清空 m_vars 的接口
    void clear_all_vars() {
        m_vars.clear();
    }

    void set_all_vars(const std::map<std::string, int>& vars) {
        m_vars = vars;//此处为深度拷贝
    }

    void print_all_vars(void) {
        fmt::print("Parsed values:\n");
        for (const auto& pair : m_vars_mirror) {
            fmt::print("[{}] -- > {} = {}\n", pair.first, pair.second.str_value, pair.second.int_value);
        }
        fmt::print("--------------------\n");
    }
};


/******************************************************************************/


/**
 * @brief 表达式求值器 (Stable Class, Unchanged)
 * 中缀转后缀 → [A, B, And, Not] 中缀适合人阅读，后缀(逆波兰式)适合计算机处理
 * 调度场算法:
 * 核心逻辑
    当扫描到当前运算符时：
    持续弹出栈顶优先级更高或相等的运算符到输出队列，直到条件不满足，然后当前token进栈
    读到左括号：直接入栈；
    读到右括号：输出栈顶运算符并使之出栈，直到栈顶为左括号为止；令左括号出栈。
 */
class ExpressionEvaluator {
public:
    ExpressionEvaluator(const std::map<std::string, int>& macros) : macros_(macros) {}
/*
 * → 词法分析 → [Not, LeftParen, A, And, B, RightParen]
 * → 中缀转后缀 → [A, B, And, Not] 
 * → 后缀求值 → !(1&&0) → true
*/
    bool evaluate(const std::string& expression) {
        try {
            auto tokens = tokenize(expression);
            auto rpn = to_rpn(tokens); // 转换为逆波兰表达式
            return evaluate_rpn(rpn);  // 对逆波兰表达式求值
        } catch (const std::exception& e) {
            fmt::print(stderr, "Evaluation error for expression \"{}\": {}\n", expression, e.what());
            return false;
        }
    }

private:
    const std::map<std::string, int>& macros_;

    // <<< MODIFICATION START: 扩展Token类型，增加算术运算符 >>>
    // Token类型 ?
    enum class TokenType {
        UNKNOWN,
        NUMBER,
        IDENTIFIER,
        OPERATOR_ARITH,   // 新增: +, -, *, /
        OPERATOR_LOGIC,   // and, or
        OPERATOR_COMP,    // >, <, >=, <=, ==, !=
        OPERATOR_UNARY,   // not
        LPAREN,           // (
        RPAREN,           // )
        END
    };
    // <<< MODIFICATION END >>>


    // Token结构体
    struct Token {
        TokenType type;
        std::string text;
        int precedence; // 运算符优先级
        bool is_left_associative; // 运算符结合性
    };

    /**
     * @brief 词法分析器，将输入字符串转换为Token序列
     */
    std::vector<Token> tokenize(const std::string& input) {
        std::vector<Token> tokens;
        for (size_t i = 0; i < input.length(); ) {
            char c = input[i];
            if (std::isspace(c)) {
                i++;
                continue;
            }

            if (std::isdigit(c)) {
                std::string num_str;
                while (i < input.length() && std::isdigit(input[i])) {
                    num_str += input[i++];
                }
                tokens.push_back({TokenType::NUMBER, num_str, 0, true});
            } else if (std::isalpha(c) || c == '_') {
                std::string ident_str;
                while (i < input.length() && (std::isalnum(input[i]) || input[i] == '_')) {
                    ident_str += input[i++];
                }
                
                // <<< MODIFICATION START: 调整所有运算符的优先级以便正确处理混合运算 >>>
                /*
                * 优先级顺序 (高到低):
                * 5: not (一元)
                * 4: *, /
                * 3: +, -
                * 2: >, <, >=, <=, ==, !=
                * 1: and
                * 0: or
                */
                if (iequals(ident_str, "and")) {
                    tokens.push_back({TokenType::OPERATOR_LOGIC, "and", 1, true});
                } else if (iequals(ident_str, "or")) {
                    tokens.push_back({TokenType::OPERATOR_LOGIC, "or", 0, true});
                } else if (iequals(ident_str, "not")) {
                    tokens.push_back({TokenType::OPERATOR_UNARY, "not", 5, false});
                } else {
                    tokens.push_back({TokenType::IDENTIFIER, ident_str, 0, true});
                }
                // <<< MODIFICATION END >>>

            } else if (c == '(') {
                tokens.push_back({TokenType::LPAREN, "(", 0, true}); i++;
            } else if (c == ')') {
                tokens.push_back({TokenType::RPAREN, ")", 0, true}); i++;
            // <<< MODIFICATION START: 增加对算术运算符的识别 >>>
            } else if (std::string("+-*/").find(c) != std::string::npos) {
                std::string op_str(1, c);
                int precedence = (c == '*' || c == '/') ? 4 : 3;
                tokens.push_back({TokenType::OPERATOR_ARITH, op_str, precedence, true}); i++;
            // <<< MODIFICATION END >>>
            } else if (std::string("><=!").find(c) != std::string::npos) {
                std::string op_str;
                op_str += c; i++;
                if (i < input.length() && input[i] == '=') {
                    op_str += input[i++];
                }
                // <<< MODIFICATION: 调整比较运算符的优先级 >>>
                tokens.push_back({TokenType::OPERATOR_COMP, op_str, 2, true});
            } else {
                throw std::runtime_error(fmt::format("Unknown character in expression: {}", c));
            }
        }
        tokens.push_back({TokenType::END, "", 0, true});
        return tokens;
    }



    // 使用Shunting-yard算法将中缀表达式转换为逆波兰表达式 (RPN)
    std::vector<Token> to_rpn(const std::vector<Token>& tokens) {
        std::vector<Token> output_queue;
        std::vector<Token> operator_stack;

        for (const auto& token : tokens) {
            switch (token.type) {
                case TokenType::NUMBER:
                case TokenType::IDENTIFIER:
                    output_queue.push_back(token);
                    break;

                // <<< MODIFICATION START: 将算术运算符加入处理逻辑 >>>
                case TokenType::OPERATOR_ARITH:
                // <<< MODIFICATION END >>>
                case TokenType::OPERATOR_LOGIC:
                case TokenType::OPERATOR_COMP:
                case TokenType::OPERATOR_UNARY:
                    while (!operator_stack.empty() && operator_stack.back().type != TokenType::LPAREN) {
                        const auto& op2 = operator_stack.back();
                        if ((token.is_left_associative && token.precedence <= op2.precedence) ||
                            (!token.is_left_associative && token.precedence < op2.precedence)) {
                            output_queue.push_back(op2);
                            operator_stack.pop_back();
                        } else {
                            break;
                        }
                    }
                    operator_stack.push_back(token);
                    break;
                case TokenType::LPAREN:
                    operator_stack.push_back(token);
                    break;
                case TokenType::RPAREN:
                    while (!operator_stack.empty() && operator_stack.back().type != TokenType::LPAREN) {
                        output_queue.push_back(operator_stack.back());
                        operator_stack.pop_back();
                    }
                    if (operator_stack.empty()) throw std::runtime_error("Mismatched parentheses.");
                    operator_stack.pop_back(); // Pop the left parenthesis
                    break;
                default: break; // Ignore END token
            }
        }

        while (!operator_stack.empty()) {
            if (operator_stack.back().type == TokenType::LPAREN) throw std::runtime_error("Mismatched parentheses.");
            output_queue.push_back(operator_stack.back());
            operator_stack.pop_back();
        }

        return output_queue;
    }

    // 对逆波兰表达式 (RPN) 进行求值
    /*
    * 后缀求值： 不停得从vector里读取内容，
    * 如果是有效变量，压栈 ；否则返回false
    * 如果是运算符，从栈顶pop两个变量，进行运算，然后把结果压栈
    * 直到队列读完，返回栈内得运算结果
    */
    bool evaluate_rpn(const std::vector<Token>& rpn_tokens) {
        std::vector<long long> value_stack; // 使用 long long 防止中间计算溢出

        for (const auto& token : rpn_tokens) {
            if (token.type == TokenType::NUMBER) {
                value_stack.push_back(std::stoll(token.text));
            } else if (token.type == TokenType::IDENTIFIER) {
                auto it = macros_.find(token.text);
                if (it == macros_.end()) {
                    fmt::print(stderr, "Warning: Macro '{}' not defined, defaulting to 0.\n", token.text);
                    value_stack.push_back(0);
                } else {
                    value_stack.push_back(it->second);
                }
            } else { // 是一个运算符
                if (token.type == TokenType::OPERATOR_UNARY) {
                    if (value_stack.empty()) throw std::runtime_error("Syntax error for unary operator.");
                    long long operand = value_stack.back(); value_stack.pop_back();
                    if (token.text == "not") value_stack.push_back(!operand);
                } else { // 是二元运算符
                    if (value_stack.size() < 2) throw std::runtime_error("Syntax error for binary operator.");
                    long long right = value_stack.back(); value_stack.pop_back();
                    long long left = value_stack.back(); value_stack.pop_back();

                    // <<< MODIFICATION START: 增加对不同类型运算符的处理 >>>
                    switch (token.type) {
                        case TokenType::OPERATOR_ARITH:
                            if (token.text == "+") value_stack.push_back(left + right);
                            else if (token.text == "-") value_stack.push_back(left - right);
                            else if (token.text == "*") value_stack.push_back(left * right);
                            else if (token.text == "/") {
                                if (right == 0) throw std::runtime_error("Division by zero");
                                value_stack.push_back(left / right);
                            }
                            break;
                        case TokenType::OPERATOR_COMP:
                            if (token.text == ">") value_stack.push_back(left > right);
                            else if (token.text == "<") value_stack.push_back(left < right);
                            else if (token.text == ">=") value_stack.push_back(left >= right);
                            else if (token.text == "<=") value_stack.push_back(left <= right);
                            else if (token.text == "==") value_stack.push_back(left == right);
                            else if (token.text == "!=") value_stack.push_back(left != right);
                            break;
                        case TokenType::OPERATOR_LOGIC:
                            if (token.text == "and") value_stack.push_back(left && right);
                            else if (token.text == "or") value_stack.push_back(left || right);
                            break;
                        default: throw std::runtime_error("Unknown binary operator in RPN evaluation.");
                    }
                    // <<< MODIFICATION END >>>
                }
            }
        }
        if (value_stack.size() != 1) throw std::runtime_error("The final value stack has an incorrect number of values.");
        return static_cast<bool>(value_stack.back());
    }
};

/******************************************************************************/

int main(int argc, char* argv[]) {
    std::time_t now = std::time(nullptr);
    fmt::print("{}\n", std::ctime(&now));
    fmt::print("CopyLibs Tool version {} for MVS BT_audio SDK\n", ver);
    fmt::print("Written by CPP\n");
    fmt::print("Current path: {}\n", fs::current_path().string());

#ifdef DEBUG_EXPRESS
    define_libs_list = {
        "FEATURE_A", "FEATURE_B", "FEATURE_C", "MAX_USERS", "MAX_PLAY", 
        "MAX_MACHIN", "MAX_LEADER", "DEBUG_MODE", "UNRELATED_DEFINE", 
        "OUT_OF_RANGE_TEST", "INVALID_VALUE_TEST", "ANOTHER_FEATURE", "CAstle",
    };


    std::string define_string = R"(
        // 这是一个宏定义赋值文件
        #define FEATURE_A       ENABLE      // 功能A开关
        #define FEATURE_B                   // 功能B，无值，默认为1
        #define FEATURE_C       0           //
        #define MAX_USERS       (0x56+1+(5*6))    // 最大用户数 (87)
        #define MAX_MACHIN      (150)       // 最大机器数
        #define MAX_PLAY        MAX_USERS   // 最大玩家数 (87)
        #define MAX_LEADER      (MAX_USERS + MAX_PLAY) // 最大领导数 (174)
        
        #define DEBUG_MODE      DISABLE     // 调试模式关闭

        // 一些无关的定义
        #define VERSION "1.0.0"
        #define UNRELATED_DEFINE 999 

        // 测试无效值
        #define INVALID_VALUE_TEST  HelloWorld  
        #define OUT_OF_RANGE_TEST   99999999999999999999

        // 多余的空格和注释
              #define ANOTHER_FEATURE 123  // 这个也会被解析
		#define CAstle (FEATURE_C + MAX_LEADER)
    )";

    // 1. Create an instance of the parser
    DefineValueParser parser_tst;

    // 2. 调用函数进行解析
    fmt::print("Starting parsing...\n");
    parser_tst.parse_defines_data(define_string);
    fmt::print("Parsing finished.\n");
    fmt::print("--------------------\n");

    // 3. 打印结果以验证
	parser_tst.print_all_vars();

    // 2. 创建求值器实例
    ExpressionEvaluator evaluator(parser_tst.get_all_vars());
    
    // 3. 定义需要测试的表达式
    std::vector<std::string> expressions_to_test = {
        // 来自您需求的示例
        "not FEATURE_A and not (FEATURE_B > 3)", 
        "not DEBUG_MODE", 
        "not FEATURE_A and FEATURE_B", 
        "MAX_PLAY != MAX_USERS", 
        // 更多测试用例
        "MAX_USERS == MAX_MACHIN",
        "MAX_USERS > 100",
        "UNDEFINED_MACRO == 0", // 测试未定义宏的情况
        "(not FEATURE_A and FEATURE_B) or (ANOTHER_FEATURE >= 100)", 
        "FEATURE_A == FEATURE_C", 
        
        // 您要求的核心测试用例
        "MAX_LEADER == (MAX_USERS + MAX_PLAY)",
        "MAX_LEADER == (MAX_USERS - MAX_PLAY)",

        // 更多混合运算的复杂测试用例
        "MAX_LEADER > (MAX_USERS + 10)",
        "MAX_LEADER > MAX_USERS + 10", // 无括号版本
        "(MAX_USERS * 2) > (MAX_LEADER - 50)",
        "FEATURE_A and (MAX_USERS + MAX_PLAY > MAX_LEADER - 1)",
        "DEBUG_MODE and (MAX_USERS + MAX_PLAY > MAX_LEADER - 1)",
    };
    
    // 4. 对表达式进行求值并打印结果
    for(const auto& expr : expressions_to_test) {
        bool result = evaluator.evaluate(expr);
        // Use fmt::print with boolean formatting
        fmt::print("Expression: \"{}\"  -->  Result: {}\n", expr, result);
    }

    return 0;

#endif

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            usage();
            return 0;
        } else if ((arg == "-s" || arg == "--script") && i + 1 < argc) {
            ini_config_file = argv[++i];
        }
    }

    // 解析INI配置文件
    pt::ptree config;
    try {
        pt::read_ini(ini_config_file.string(), config);
    } catch (const pt::ini_parser_error& e) {
        std::cerr << "Error reading config file: " << e.what() << "\n";
        return 1;
    }

    default_dest_lib = config.get<std::string>("main_config.default_dest_lib", "");
    default_lib = config.get<std::string>("main_config.default_lib", "");
    home_dir = config.get<std::string>("main_config.home_dir", "");

    auto value_opt = config.get_optional<int>("main_config.force_runall");
    if (value_opt && value_opt > 0) {
        // 参数存在
        force_runall = true;
    } else {
        // 参数不存在，使用默认值
        force_runall = false;
    }
    fmt::print("force run all:  {}\n", force_runall);

    // 设置工作目录
    if (!home_dir.empty()) {
        fs::current_path(home_dir);
        fmt::print("Home directory: {}\n", fs::current_path().string());
    }

    // 初始化宏定义
    std::map<std::string, int> temp_globle_vars;
    for (const auto& item : config.get_child("macro_declare")) {
        temp_globle_vars[item.first] = item.second.get_value<int>();
        define_libs_list.push_back(item.first);
    }

    // 从定义文件中解析宏值
    DefineValueParser parser;
    parser.set_all_vars(temp_globle_vars);//set default
    for (const auto& item : config.get_child("define_files")) {
        parser.parse_defines_from_file(item.second.get_value<std::string>());
    }

    parser.print_all_vars();
    
    // 检查有效的表达式
    std::vector<std::string> matched_expression;
    try{
        for (const auto& item : config.get_child("macro_express")) {
            // fmt::print("macro_express: {}\n", item.first);
            // if (evaluate_expression(item.first)) {

            ExpressionEvaluator evaluator(parser.get_all_vars());

            fmt::print("Expression: \"{}\"  -->  Result: {}\n", item.first, evaluator.evaluate(item.first));

            if (evaluator.evaluate(item.first)){
                matched_expression.push_back(item.first);
                fmt::print("matched express: {}\n", item.first);
                if(!force_runall)//if not force run all, break here.
                    break;
            }
        }
    } catch (const std::exception& e) {
        fmt::print("express error: {}\n", e.what());
    }

    if (!matched_expression.empty()) {
        for (const auto& expr : matched_expression)
        {
            if(!expr.empty())//not ""
            {
                fmt::print("Executing: {}\n", expr);
                std::string path_spec = config.get<std::string>("macro_express." + expr);
                trim(path_spec);

                if (path_spec.find('>') != std::string::npos) {
                    std::vector<std::string> paths;
                    boost::split(paths, path_spec, boost::is_any_of(">"));
                    fs::path src = paths[0];
                    fs::path dest = paths[1];
                    
                    fmt::print("Copying {}\n", src.filename().string());
                    force_copy_file(src, dest);
                } else if (!default_dest_lib.empty()) {
                    fs::path src = path_spec;
                    fmt::print("Copying {}\n", src.filename().string());
                    force_copy_file(src, default_dest_lib);
                } else {
                    std::cout << "No default destination specified.\n";
                }
            }
        }
    }
    else {
        fmt::print("Performing default copy operations.\n");
        for (const auto& item : config.get_child("default_copy")) {
            std::string path_spec = item.second.get_value<std::string>();
            trim(path_spec);
            
            std::vector<std::string> paths;
            boost::split(paths, path_spec, boost::is_any_of(">"));
            fs::path src = paths[0];
            fs::path dest = paths[1];
            
            fmt::print("Copying {}\n", src.filename().string());
            force_copy_file(src, dest);
        }
    }

    std::cout << "Copy operations completed.\n";
    return 0;
}