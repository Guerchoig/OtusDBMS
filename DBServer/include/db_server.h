/**
 * @brief db_server.cpp interface and definitions
 *
 */
#pragma once
#include "db_command.h"
#include "sqlite3.h"

constexpr auto default_db_directory = "./db/";
/**
 * @brief Symbol to add at the end of each db result
 */
constexpr unsigned char END_OF_REPLY = '^';

/**
 * @brief Symbol to add at the end of each row of db result
 */
constexpr unsigned char END_OF_CHUNK = '\n';

typedef void (*foreign_callback_t)(void *, std::string);

/**
 * @brief db object
 */
class db_t
{
private:
    sqlite3 *pdb;
    void *handle; // external id to store in db obj
    std::string db_path;

    foreign_callback_t foreign_callback;           // external callback to call for each row of result
    friend int db_callback(void *db, int nof_cols, // internal callback to call for each row of result
                           char **cols_of_string,
                           [[maybe_unused]] char **col_names);

public:
    void execute_cmd(const std::string cmd);
    static void clean_directory(std::string _db_directory);
    db_t(std::string _db_directory,
         foreign_callback_t, void *handle);
    ~db_t();
};
