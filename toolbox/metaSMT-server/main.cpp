#include <iostream>

#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/thread.hpp>

#include "connection.hpp"

using boost::asio::ip::tcp;

void server(boost::asio::io_service& io_service)
{
  int port = 1313;
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
  std::cout << "metaSMT-server started" << std::endl;
  std::cout << "Listening on port " << port << std::endl;

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
