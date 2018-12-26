#ifndef CLIENT_H
#define CLIENT_H

#include <cstdlib>
#include <deque>
#include <list>
#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>
#include <memory>
#include <boost/asio.hpp>
#include "message.h"
#include "commandhandler.h"

using boost::asio::ip::tcp;

typedef std::deque<Message> message_queue;

class Client : public AbstractClient
{
public:

  Client(std::string address, std::string port) : address_(address), port_(port), socket_(io_service_) {
      request_handlers.push_back(std::shared_ptr<CommandHandler>(new SETRTHandler(this)));
      request_handlers.push_back(std::shared_ptr<CommandHandler>(new GETTHandler(this)));
  }

  ~Client(){
      stop();
  }

  void start() override;
  void stop() override;

  void write(const Message& msg) override;

private:
  void do_connect();

  void do_read_header();

  void do_read_body();

  void do_write();

//  void infinite_loop(); // for support SETRT and GETT requests

  void close_socket();

private:

  std::string address_;
  std::string port_;
  boost::asio::io_service io_service_;
  std::thread io_service_tread;
  tcp::socket socket_;
  Message read_msg_;
  message_queue write_msgs_;
  std::list<std::shared_ptr<CommandHandler>> request_handlers;
  tcp::resolver::iterator endpoint_iterator;

};

#endif // CLIENT_H
