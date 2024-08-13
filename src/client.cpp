/**
 * @brief client.cpp 
 * The test client to request join_server
 *
 */

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>
#include <iostream>

namespace asio = boost::asio;

using socket_t = asio::ip::tcp::socket;

// Client parameters
constexpr int port = 4507;
constexpr auto host = "127.0.0.1";

// Disconnect from join_server symbol
constexpr unsigned char DISCONNECT = 0x4;

// Is received at the end of every whole reply
constexpr unsigned char END_OF_REPLY = '^';

/**
 * @brief Send commands to join_server and receive replys, disconnects from server at the end
 * @return
 */
int main()
{
    asio::io_context context;

    socket_t socket{context};
    socket.connect(
        asio::ip::tcp::endpoint{asio::ip::make_address_v4("127.0.0.1"), 4507});

    std::vector<std::string> lines{
        "INSERT A 0 lean\n",
        "INSERT A 0 understand\n",
        "INSERT A 1 sweater\n",
        "INSERT A 2 frank\n",
        "INSERT B 1 flour\n",
        "INSERT B 2 wonder\n",
        "INSERT B 8 selection\n",
        "INTERSECTION\n",
        "SYMMETRIC_DIFFERENCE\n",
        "TRUNCATE A\n"};

    boost::system::error_code ec;

    for (auto line : lines)
    {
        std::cout << line;

        auto send_n = socket.send(asio::buffer(line), {}, ec);
        assert(!ec);
        assert(send_n == line.size());

        std::string received_line(1024, '\0');
        bool eor;
        do
        {
            auto recv_n = socket.receive(asio::buffer(received_line), {}, ec);
            assert(!ec);
            assert(recv_n);

            eor = (received_line[recv_n - 1] == END_OF_REPLY);
            if (eor)
                received_line.erase(recv_n - 1);
            std::cout << received_line;
        } while (!eor);
    }
    socket.send(asio::buffer(std::string(1, DISCONNECT)), {}, ec);
}
