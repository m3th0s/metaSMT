#include <iostream>

#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/thread.hpp>

#include "connection.hpp"

using boost::asio::ip::tcp;

void server(boost::asio::io_service& io_service)
{
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 1313));

  for (;;)
  {
    socket_ptr sock(new tcp::socket(io_service));
    acceptor.accept(*sock);
    boost::thread t(boost::bind(Connection::new_connection, sock));
  }
}

int main(int argc, const char *argv[])
{
  try
  {
    boost::asio::io_service io_service;
    server(io_service);
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
