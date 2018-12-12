#include "server.h"

std::shared_ptr<RequestHandler> Server::get_request_handler(std::shared_ptr<Session> session) {
    if (sessions_.find(session) != sessions_.end() && !request_handlers_.empty()) {
        auto handler = request_handlers_.front();
        borrowers[session] = handler;
        return handler;
    }
    return nullptr;
}

void Server::do_accept()
{
    acceptor_.async_accept(socket_,
                           [this](boost::system::error_code ec)
    {
        if (!ec)
        {
            std::make_shared<Session>(std::move(socket_), *this)->start();
        }

        do_accept();
    });
}

void Session::take_response(std::string response)
{
    Message msg;
    msg.body_length(response.size());
    std::memcpy(msg.body(), response.data(), msg.body_length());
    msg.encode_header();

    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
        do_write();
    }
}

void Session::handle_message(const Message &msg)
{
    auto handler = supervisor_.get_request_handler(shared_from_this());
    if (handler)
        handler->handle(std::string(msg.body(), msg.body_length()), this);
    else {
        take_response("Database connectons expired\n");
    }
}

void Session::do_read_header()
{
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.data(), Message::header_length),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec && read_msg_.decode_header())
        {
            do_read_body();
        }
        else
        {
            supervisor_.leave(shared_from_this());
        }
    });
}

void Session::do_read_body()
{
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec)
        {
            handle_message(read_msg_);
            do_read_header();
        }
        else
        {
            supervisor_.leave(shared_from_this());
        }
    });
}

void Session::do_write()
{
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_msgs_.front().data(),
                                                 write_msgs_.front().length()),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec)
        {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
                do_write();
            }
        }
        else
        {
            supervisor_.leave(shared_from_this());
        }
    });
}
