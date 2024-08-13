
/**
 * @brief db_command.cpp
 * Code used to produce SQL requests from relational algebra commands
 *
 */
#include "db_command.h"
#include <cstring>
#include <cstdarg>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>

/**
 * @brief Command object constructor producing SQL requests from relational algebra commands
 * @param s_cmd relational algebra command
 */
command::command(const std::string s_cmd) : cmd(s_cmd)
{
    std::vector<std::string> args;

    auto key = std::string_view(s_cmd.begin(), s_cmd.begin() + sizeof(cmdkey_char_t) - 1);
    templ_and_flag = templates[key];
    // stateful_sv ssv_cmd(cmd);
    // ssv_cmd.fetch_next_word(CMD_DELIM); // skip keyword

    extract_args(args);
    request_by_template(args);
}

/**
 * @brief Extracts arguments from relational algebra command
 * @param args relational algebra command string
 */
void command::extract_args(std::vector<std::string> &args)
{
    if (cmd.empty())
        return;
    std::istringstream s(cmd);
    std::string arg;
    for (int i = 0; !s.eof(); ++i)
    {
        s >> arg;
        if (i == 0)
            continue; // args index is not increased here!
        if (i > 3)
        {
            args.at(2).append(std::string(" ") + std::move(arg));
            continue;
        }
        args.push_back(arg);
    }
}

/**
 * @brief Substitutes arguments from relational algebra command
 *        into SQL template
 * @param args relational algebra command
 */
void command::request_by_template(const std::vector<std::string> &args)
{
    request = std::move(std::string(templ_and_flag.tmpl));
    int i = 1;
    for (auto arg : args)
    {
        std::string pattern = "%" + std::to_string(i);
        auto cnt = replace_pattern(request, pattern, arg);
        if (!cnt)
            break;
        i++;
    }
}

/**
 * @brief Collection of SQL templates
 */
templ_n_flags::templ_n_flags()
{

    std::string_view
        create_key{"CRE"},
        truncate_key{"TRU"},
        insert_key{"INS"},
        intersection_key{"INT"},
        symmetric_difference_key{"SYM"};

    templ_n_flag create_templ = {/*"PRAGMA journal_mode=WAL;"
                                 "PRAGMA synchronous=NORMAL;"*/
                                 "DROP TABLE IF EXISTS A;"
                                 "DROP TABLE IF EXISTS B;"
                                 "CREATE TABLE A (id int PRIMARY KEY, name varchar(255) );"
                                 "CREATE TABLE B (id int PRIMARY KEY, name varchar(255) );",
                                 NO_ASKNOLEGEMENT};
    templ_n_flag truncate_templ = {"DELETE FROM %1 WHERE TRUE;",
                                   SEND_ASKNOLEGEMENT};
    templ_n_flag insert_templ = {"INSERT INTO %1 (id, name) VALUES (%2, '%3');",
                                 SEND_ASKNOLEGEMENT};
    templ_n_flag intersection_templ = {"SELECT A.id AS id,"
                                       "A.name AS Aname, "
                                       "B.name AS Bname "
                                       "FROM A AS A INNER JOIN B AS B "
                                       "ON(A.id = B.id) "
                                       "ORDER BY id;",
                                       SEND_ASKNOLEGEMENT};
    templ_n_flag symm_diff_templ = {"SELECT "
                                    "CASE "
                                    "   WHEN A.id IS NOT NULL THEN A.id "
                                    "   ELSE B.id "
                                    "END As id,"
                                    "A.name AS name,"
                                    "B.name AS Bname "
                                    "FROM A AS A FULL JOIN B AS B ON(A.id = B.id) "
                                    "WHERE A.id IS NULL OR B.id IS NULL "
                                    "ORDER BY CASE "
                                    "   WHEN A.id IS NOT NULL THEN A.id "
                                    "   ELSE B.id "
                                    "END;",
                                    SEND_ASKNOLEGEMENT};

    t_n_fs.emplace(std::pair{create_key, create_templ});
    t_n_fs.emplace(std::pair{truncate_key, truncate_templ});
    t_n_fs.emplace(std::pair{insert_key, insert_templ});
    t_n_fs.emplace(std::pair{intersection_key, intersection_templ});
    t_n_fs.emplace(std::pair{symmetric_difference_key, symm_diff_templ});
}

/**
 * @brief Replaces single placeholder by argument
 * @param s output string
 * @param pattern placeholder string
 * @param relpacement replacing arg
 * @return nof patterns replaced
 */
int replace_pattern(std::string &s, std::string_view pattern, std::string_view relpacement)
{
    size_t pos = 0, cnt = 0;

    while ((pos = s.find(pattern, pos)) != std::string::npos)
    {
        s.replace(pos, pattern.size(), relpacement);
        pos += relpacement.size();
        cnt++;
    }
    return cnt;
}
