#ifndef JANK_NREPL_SERVER_HPP
#define JANK_NREPL_SERVER_HPP

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>

using HandlerFunction = std::function<void(std::istream &, std::ostream &)>;

class connection : public std::enable_shared_from_this<connection>
{
public:
  connection(boost::asio::ip::tcp::socket socket, HandlerFunction handler);
  void start();

private:
  void do_read();
  void do_write(std::string const &response);

  boost::asio::ip::tcp::socket socket_;
  HandlerFunction handler_;

  enum
  {
    max_length = 1024
  };

  char data_[max_length];
};

class server
{
public:
  server(boost::asio::io_context &io_context, short port, HandlerFunction handler);

private:
  void do_accept();

  boost::asio::ip::tcp::acceptor acceptor_;
  HandlerFunction handler_;
};

#endif // JANK_NREPL_SERVER_HPP
