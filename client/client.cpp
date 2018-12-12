#include "client.h"

using boost::asio::ip::tcp;

static std::vector<std::string> split_string(std::string& str)
{
    std::string token;
    std::istringstream tokenStream(str);
    std::vector<std::string> ret;

    while (std::getline(tokenStream, token, ' '))
        ret.push_back(token);
    return ret;
}

void Client::start(){
    tcp::resolver resolver(io_service_);
    auto endpoint_iterator = resolver.resolve({ address_, port_ });
    do_connect(endpoint_iterator);

    boost::asio::io_service& io_ref = io_service_;
    io_service_tread = std::thread([&io_ref](){ io_ref.run(); });

    std::string buffer;
    while (std::getline(std::cin, buffer))
    {
        auto tokens = split_string(buffer);
        if (tokens.empty()) {
            std::cout << "Wrong type of request, use SET, SETRT, GET or GETT\n";
            continue;
        }

        for (auto& handler : request_handlers)
            handler->handle(tokens);
    }
    stop();
}

void Client::stop()
{
    close_socket();
    io_service_tread.join();
}

void Client::write(const Message &msg)
{
    io_service_.post(
                [this, msg]()
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
        {
            do_write();
        }
    });
}

void Client::close_socket()
{
    io_service_.post([this]() { socket_.close(); });
}

void Client::do_connect(tcp::resolver::iterator endpoint_iterator)
{
    boost::asio::async_connect(socket_, endpoint_iterator,
                               [this](boost::system::error_code ec, tcp::resolver::iterator)
    {
        if (!ec)
        {
            do_read_header();
        }
    });
}

void Client::do_read_header()
{
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.data(), Message::header_length),
                            [this](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec && read_msg_.decode_header())
        {
            do_read_body();
        }
        else
        {
            socket_.close();
        }
    });
}

void Client::do_read_body()
{
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec)
        {
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";
            do_read_header();
        }
        else
        {
            socket_.close();
        }
    });
}

void Client::do_write()
{
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_msgs_.front().data(),
                                                 write_msgs_.front().length()),
                             [this](boost::system::error_code ec, std::size_t /*length*/)
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
            socket_.close();
        }
    });
}
