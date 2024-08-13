/**
 * @brief join_server.h Contains definitions for join_server.cpp
 *        
 */
#pragma once

#include "db_server.h"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>

#include <iostream>
#include <tuple>
#include <string>
#include <list>
#include <memory>
#include <coroutine>
#include <utility>

namespace asio = boost::asio;
using port_t = asio::ip::port_type;
using socket_t = asio::ip::tcp::socket;

constexpr auto default_ip = "127.0.0.1";
constexpr port_t default_port = 4507;
constexpr unsigned char DISCONNECT = 0x4;

inline asio::io_context context;

/**
 * @brief Connection object, specifically contains a pointer to client's db 
 *        and a connection-initialized socket
 */
struct connection_t
{
    std::shared_ptr<db_t> pdbt;
    socket_t socket;
    asio::awaitable<void> read_requests();
    asio::awaitable<void> send_reply(std::string reply);

    connection_t(std::string _db_directory,
                 foreign_callback_t foreign_callback
                 );
    
    
};

using handle_t = std::shared_ptr<connection_t>;

/**
 * @brief A type storing a main join_server object
 */
class join_server_t
{
private:
    std::string ip_addr;
    port_t port;
    std::list<handle_t> connections; // Collection of connections
    friend void SIGINT_handler([[maybe_unused]] int _signal); // Ctrl-C signal handler
    friend asio::awaitable<void> run_server(asio::io_context &context);

public:
    void print_running()
    {
        std::cout << "join_server running at " + ip_addr << ":" << port << "\n";
    }

    std::string get_ip_addr() { return ip_addr; }
    port_t get_port() { return port; }

    void disconnect(connection_t *conn);

    join_server_t(const std::string _ip_addr, port_t _port);
    ~join_server_t();
};

/**
 * @brief Global join_server pointer
 */
inline std::unique_ptr<join_server_t> p_joinserver;

/**
 * @brief Handles CTRL-C signal to softly shutdown the server
 * @param _signal
 */
void SIGINT_handler([[maybe_unused]] int _signal)
{
    context.stop(); // stop the coro loop
}

void establish_SIGINT_handler()
{
    // Establish CTRL-C handler
    struct sigaction handler;
    handler.sa_handler = SIGINT_handler;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    sigaction(SIGINT, &handler, NULL);
}

/**
 * @brief Proceed command string args
 * @param argc 0 or 1
 * @param argv optional port number
 * @param server_port output parameter to store the port
 * @return true if args are viable
 */
inline bool get_params(int argc, char **argv, port_t &server_port)
{
    bool res = true;
    server_port = default_port;
    switch (argc)
    {
    case 1:
        server_port = default_port;
        break;
    case 2:
        server_port = std::atoi(argv[1]);
        break;
    default:
        std::cout << "The use is: join_server <port number>\n"
                     "or\tjoin_server\n";
        res = false;
        break;
    }
    return res;
}
