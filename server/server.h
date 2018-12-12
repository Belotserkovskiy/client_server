#ifndef SERVER_H
#define SERVER_H

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "message.h"
#include "requesthandler.h"

using boost::asio::ip::tcp;

typedef std::deque<Message> chat_message_queue;
class Session;

class SessionSupervisor {
public:
    virtual ~SessionSupervisor(){}
    virtual void join(std::shared_ptr<Session>) = 0;
    virtual void leave(std::shared_ptr<Session>) = 0;
    virtual std::shared_ptr<RequestHandler> get_request_handler(std::shared_ptr<Session> session) = 0;
};

class Session :
    public std::enable_shared_from_this<Session>, public RequestTransmitter
{
    SessionSupervisor& supervisor_;
public:
  Session(tcp::socket socket, SessionSupervisor& supervisor)
    : socket_(std::move(socket)), supervisor_(supervisor) {}

  void start()
  {
    supervisor_.join(shared_from_this());
    do_read_header();
  }

  void take_response(std::string response) override;

private:
  void handle_message(const Message& msg);

  void do_read_header();

  void do_read_body();

  void do_write();

  tcp::socket socket_;
  Message read_msg_;
  chat_message_queue write_msgs_;
};

class Server : public SessionSupervisor
{
public:
  Server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
  {
      for (int i = 0; i < 10; ++i) // todo: need to request from ini file
        request_handlers_.push_back(std::make_shared<RequestHandler>("test.db"));
      do_accept();
  }

  void join(std::shared_ptr<Session> session) override {
      sessions_.insert(session);
  }
  void leave(std::shared_ptr<Session> session) override {
      sessions_.erase(session);
  }

  std::shared_ptr<RequestHandler> get_request_handler(std::shared_ptr<Session> session) override;

private:
  void do_accept();

  std::map<std::shared_ptr<Session>, std::shared_ptr<RequestHandler>> borrowers;
  std::deque<std::shared_ptr<RequestHandler>> request_handlers_;
  std::set<std::shared_ptr<Session>> sessions_;
  tcp::acceptor acceptor_;
  tcp::socket socket_;
};

#endif // SERVER_H
