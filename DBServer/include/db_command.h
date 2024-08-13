/**
 * @brief db_command.h Contains definitions for db_command.cpp,
 * which is used to produce SQL requests from relational algebra commands
 *
 */
#pragma once
#include <unordered_map>
#include <string_view>
#include <string>
#include <string.h>
#include <vector>

constexpr char CMD_DELIM = ' ';

constexpr bool SEND_ASKNOLEGEMENT = true;
constexpr bool NO_ASKNOLEGEMENT = false;

/**
 * @brief Store an SQL request template, '%n' sequences are used as teplate args
 */
struct templ_n_flag
{
    const char *tmpl = NULL;
    bool flag = NO_ASKNOLEGEMENT;
};

/**
 * @brief Collection of SQL requests templates
 */
struct templ_n_flags
{
    std::unordered_map<std::string_view, templ_n_flag> t_n_fs;
    templ_n_flags();
    templ_n_flag &operator[](std::string_view key) { return t_n_fs[key]; }
};

/**
 * @brief Class constructor produce an SQL request from relational algebra command
 */
class command
{
private:
    void request_by_template(const std::vector<std::string> &args);
    void extract_args(std::vector<std::string> &args);
    std::string cmd;
    templ_n_flag templ_and_flag;
    static templ_n_flags templates;

public:
    command(const std::string s_cmd);
    std::string request;
    bool send_asknolegement() { return templ_and_flag.flag; };
};

/**
 * @brief Global collection of SQL templates
 */
inline templ_n_flags command::templates;

using cmdkey_char_t = char[sizeof(uint32_t)];

/**
 * @brief Substitutes template params from command arguments
 * @param s Resulting request
 * @param pattern Templste
 * @param relpacement Arguments
 * @return 
 */
int replace_pattern(std::string &s, std::string_view pattern, std::string_view relpacement);
