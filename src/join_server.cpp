/**
 * @brief join_server.cpp 
 * This is a relational algebra operation server,
 * accessible by network
 */
#include "db_server.h"
#include "join_server.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <coroutine>
#include <cstdlib>
#include <memory>
#include <utility>
#include <string>
#include <unordered_map>

/**
 * @brief A coroutine, called by database callback for every line of db result
 * and sending this line to client
 * @param reply the string to be sent
 * @return special asio coro type 
 */
asio::awaitable<void> connection_t::send_reply(std::string reply)
{
    try
    {
        std::string line(reply);
        auto n_sent = co_await socket.async_send(
            asio::buffer(line),
            asio::use_awaitable);
        assert(n_sent == line.size());
    }
    catch (const boost::system::system_error &e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        quick_exit(1);
    }
    co_return;
}

/**
 * @brief Intent to be called from db_server library at every line of result
 * @param _pconn Points to the connection_t structure, related to this client
 * @param reply The string to be passed to the client
 */
void join_server_callback(void *_pconn, std::string reply)
{
    auto pconn = static_cast<connection_t *>(_pconn);
    asio::co_spawn(context, pconn->send_reply(reply), asio::detached);
}

/**
 * @brief Reads relational algebra commands, sent by client
 * @return special asio coro type
 */
asio::awaitable<void> connection_t::read_requests()
{

    constexpr size_t buf_size = 1024;
    std::string cmd;
    std::string line(buf_size, '\0');

    try
    {
        while (true)
        {

            auto n_read = co_await socket.async_read_some(
                asio::buffer(line),
                asio::use_awaitable);
            if (!n_read)
            {
                std::cerr << "Zero bytes read " << "\n";
                quick_exit(1);
            }

            // Begin processing \n - delimited input string
            std::string_view s(line.begin(), line.begin() + n_read);

            // Process DISCONNECT symbol, received from client
            auto pos = line.find(DISCONNECT);
            bool disconnect = (pos != std::string::npos);
            if (disconnect)
                s = s.substr(0, pos);

            // There can be several \n - delimited commands in the input string
            // or/and an unfinished command whith no delimiter at the end
            auto prev_pos = pos = 0;

            while (prev_pos < s.size())
            {
                pos = s.find("\n", prev_pos);
                if (pos == std::string::npos)
                {
                    cmd.append(s, prev_pos, s.size() - prev_pos);
                    break;
                }
                cmd.append(s, prev_pos, pos - prev_pos);
                prev_pos = pos + 1;
                pdbt->execute_cmd(cmd);

                cmd.clear();
            }
            // On DISCONNECT close socket and return
            if (disconnect)
            {
                p_joinserver->disconnect(this);
                co_return;
            }
        }
    }
    catch (const boost::system::system_error &e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        quick_exit(1);
    }
}

/**
 * @brief Listen to clients connections, creates a connection, a database 
 *         and a reading commands session for each client connected
 * @param context asio io context
 * @return special asio coro type
 */
asio::awaitable<void> run_server(asio::io_context &context)
{
    try
    {

        asio::ip::tcp::acceptor acceptor(context, asio::ip::tcp::endpoint{asio::ip::make_address_v4(p_joinserver->get_ip_addr()), p_joinserver->get_port()});
        while (true)
        {
            // Some parts of a connected socket can not be moved to another location
            // so we preliminary prepare the needed place for creating there a connected socket
            auto handle = std::make_shared<connection_t>(std::string(""),
                                                         join_server_callback);
            co_await acceptor.async_accept(handle->socket, asio::use_awaitable);

            p_joinserver->connections.push_back(handle);

            handle->pdbt->execute_cmd("CREATE");

            std::cout << "connected " << "\n";

            // Start reading coro for this connection, 
            // initialized socket is already being in connection_t body
            asio::co_spawn(context, handle->read_requests(), asio::detached);
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << "\n";
        quick_exit(1);
    }
}

/**
 * @brief join_server_t object consructor
 * @param _ip_addr ip address of 'join server'
 * @param _port port of join server
 */
join_server_t::join_server_t(const std::string _ip_addr, port_t _port)
    : ip_addr(_ip_addr), port(_port)
{
    db_t::clean_directory("");
}

/**
 * @brief join_server_t destructor - default in this version
 */
join_server_t::~join_server_t() {}

/**
 * @brief Disconnect a client. Is called when disconnection character is received
 * @param conn points to connection to be closed
 */
void join_server_t::disconnect(connection_t *conn)
{
    try
    {
        conn->socket.close();
        for (auto i = connections.begin(); i != connections.end(); ++i)
            if (i->get() == conn)
            {
                connections.erase(i);
                break;
            }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << "\n";
        quick_exit(1);
    }

    std::cout << "disconnected " << "\n";
}
/**
 * @brief Connection object constructor, constructs underlying db as well
 * @param _db_directory The directory, where all the client's databases lies
 * the names of db's include connection handle representation
 * @param foreign_callback 
 */
connection_t::connection_t(std::string _db_directory,
                           foreign_callback_t foreign_callback)
    : pdbt(std::make_shared<db_t>(_db_directory,
                                  foreign_callback,
                                  this)),
      socket{context} {}

/**
 * @brief main join_server func
 * @param argc 0 or 1
 * @param argv the server port can be specified 
 * @return 0 
 */
int main(int argc, char **argv)
{
    port_t port;
    std::srand(std::time(nullptr));
    if (!get_params(argc, argv, port))
        return 0;
    p_joinserver = std::make_unique<join_server_t>(default_ip, port);
    p_joinserver->print_running();

    // Start server coro
    asio::co_spawn(context, run_server(context), asio::detached);

    establish_SIGINT_handler();

    // Starts coro loop
    context.run();
    return 0;
}
