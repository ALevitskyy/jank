#include <jank/nrepl/server.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <utility>

using boost::asio::ip::tcp;

connection::connection(tcp::socket socket, HandlerFunction handler)
  : socket_(std::move(socket))
  , handler_(std::move(handler))
{
}

void connection::start()
{
  do_read();
}

void connection::do_read()
{
  auto self(shared_from_this());
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
                          [this, self](boost::system::error_code ec, std::size_t length) {
                            if(!ec)
                            {
                              boost::asio::streambuf buffer;
                              std::ostream os(&buffer);
                              os.write(data_, length);
                              std::istream input(&buffer);

                              std::ostringstream output;
                              handler_(input, output);

                              do_write(output.str());
                            }
                          });
}

void connection::do_write(std::string const &response)
{
  auto self(shared_from_this());
  auto buffer = std::make_shared<std::string>(response);

  boost::asio::async_write(
    socket_,
    boost::asio::buffer(*buffer),
    [this, self, buffer](boost::system::error_code ec, std::size_t /*length*/) {
      if(!ec)
      {
        do_read();
      }
    });
}

server::server(boost::asio::io_context &io_context, short port, HandlerFunction handler)
  : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  , handler_(std::move(handler))
{
  do_accept();
}

void server::do_accept()
{
  acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
    if(!ec)
    {
      std::make_shared<connection>(std::move(socket), handler_)->start();
    }

    do_accept();
  });
}
