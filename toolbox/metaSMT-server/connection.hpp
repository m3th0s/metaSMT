#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <boost/asio/ip/tcp.hpp>

#include <boost/smart_ptr.hpp>

#include <list>

#include "solver.hpp"

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

class Connection
{
public:
    Connection(socket_ptr socket);
    static void new_connection(socket_ptr socket);
private:
    std::string next_line();

    socket_ptr socket;
    boost::asio::streambuf buffer;
    void write(std::string string);

    std::list<SolverProcess*> solvers;
};

#endif
