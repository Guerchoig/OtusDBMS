/**
 * @brief db_command.cpp
 * Realizes interface library to sqlite
 *
 */

#include "db_command.h"
#include "db_server.h"
#include "sqlite3.h"
#include <string>
#include <iostream>
#include <memory>
#include <utility>
#include <stdexcept>
#include <cassert>
#include <filesystem>

/**
 * @brief Throws exception on sqlite error code
 * @param code error code
 * @param msg error message
 */
void sqlite_throw(int code, const char *msg = "")
{
    throw std::runtime_error{
        std::string("SQL Method failed: ") + std::string(sqlite3_errstr(code)) + " " + std::string(msg)};
}

/**
 * @brief Callback function, called from sqlite3_exec for each result row
 * @param db_server pointer to sqlite3 object
 * @param nof_cols number of cols in result
 * @param cols_of_string results for each column
 * @param col_names column names
 * @return
 */
int db_callback(void *db_server, int nof_cols, char **cols_of_string,
                [[maybe_unused]] char **col_names)
{
    std::string res;
    for (int i = 0; i < nof_cols; ++i)
    {
        res += (cols_of_string[i] == NULL ? std ::string_view("") : std::string_view(cols_of_string[i]));
        res += (i == nof_cols - 1) ? "" : ",";
    }
    res.append(std::string(1, END_OF_CHUNK));

    auto pdb = static_cast<db_t *>(db_server);
    pdb->foreign_callback(pdb->handle, res);
    return 0;
}

/**
 * @brief Execute SQL request
 * @param cmd SQL request
 */
void db_t::execute_cmd(const std::string cmd)
{
    char *errmsg = NULL;
    command new_command(cmd);
    std::string request = new_command.request;
    auto ec = sqlite3_exec(pdb, request.c_str(),
                           db_callback /*printer*/, (void *)this, &errmsg);
    if (ec)
    {
        // send DB error
        std::string res = "Eror: code = " + std::to_string(ec) + " ";
        if (errmsg != NULL)
        {
            res.append(std::string(errmsg));
            sqlite3_free(errmsg);
        }

        foreign_callback(handle, res + std::string(1, END_OF_CHUNK) + std::string(1, END_OF_REPLY));
        return;
    }
    if (new_command.send_asknolegement())
    {
        foreign_callback(handle, std::string("OK") + std::string(1, END_OF_CHUNK) + std::string(1, END_OF_REPLY));
    }
}

/**
 * @brief A db object
 * @param _db_directory db directory
 * @param _foreign_callback the function to be called for each resul row from db_callback
 * @param _handle some external id, to store in the db object
 */
db_t::db_t(std::string _db_directory, foreign_callback_t _foreign_callback, void *_handle)
    : handle(_handle), foreign_callback(_foreign_callback)
{
    std::string db_directory;
    if (!_db_directory.size())
        db_directory = !_db_directory.size() ? default_db_directory : _db_directory;

    db_path = db_directory + "db_sqlite" + std::to_string(reinterpret_cast<uint64_t>(handle));

    auto ec = sqlite3_open(db_path.c_str(), &pdb);
    if (ec)
    {
        std::cerr << "Error while opening db" << '\n';
        quick_exit(1);
    }
}

/**
 * @brief db object destructor
 */
db_t::~db_t()
{
    auto ec = sqlite3_close_v2(pdb);
    assert(ec == SQLITE_OK);
}

/**
 * @brief Cleans db's file directory
 * @param _db_directory directory name
 */
void db_t::clean_directory(std::string _db_directory)
{
    std::string db_directory;
    if (!_db_directory.size())
        db_directory = !_db_directory.size() ? default_db_directory : _db_directory;

    using namespace std::filesystem;
    auto res = system("clear");
    (void)res;

    if (!exists(db_directory))
    {
        auto res = create_directory(db_directory);
        assert(res);
    }

    const std::filesystem::directory_iterator _end;
    for (std::filesystem::directory_iterator it(db_directory); it != _end; ++it)
        std::filesystem::remove(it->path());
}