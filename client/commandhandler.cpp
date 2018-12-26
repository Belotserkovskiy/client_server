#include "commandhandler.h"

static const char alphanum[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

static std::string get_random_string()
{
    std::string str;
    int length = rand()%20 + 5;
    for (int i = 0; i < length; ++i)
        str += alphanum[(rand()%(sizeof(alphanum)) - 1)];
    return str;
}


int SETHandler::handle(const std::vector<std::string> &parameters)
{
    if (parameters.front() == "SET") {
        if (parameters.size() < 3) {
            std::cout << "Wrong command format, usage: SET <VARIABLE> <VALUE>\n";
            return -1;
        }

        std::cout << "name - " << parameters[1] << " value - " << parameters[2] << "\n";

        tutorial::Request request;
        request.set_type(tutorial::Request::RequestType::Request_RequestType_SET);
        request.set_name(parameters[1]);
        request.set_value(parameters[2]);
        log(request);

        Message msg;
        msg.body_length(request.ByteSize());
        request.SerializeToArray(msg.body(), request.ByteSize());
        msg.encode_header();
        client_->write(msg);
        return 1;
    }
    return 0;
}

void SETHandler::log(const tutorial::Request &request)
{
    BOOST_LOG_SEV(my_logger::get(), logging::trivial::info) << "SET variable - " << request.name() << ", value - " << request.value();
}

int GETHandler::handle(const std::vector<std::string>& parameters)
{
    if (parameters.front() == "GET") {
        if (parameters.size() < 2) {
            std::cout << "Wrong command format, usage: GET <VARIABLE>\n";
            return -1;
        }
        tutorial::Request request;
        request.set_type(tutorial::Request::RequestType::Request_RequestType_GET);
        request.set_name(parameters[1]);

        Message msg;
        msg.body_length(request.ByteSize());
        request.SerializeToArray(msg.body(), request.ByteSize());
        msg.encode_header();
        client_->write(msg);
        return 1;
    }
    return 0;
}

int SETRTHandler::handle(const std::vector<std::string>& parameters)
{
    if (parameters.front() == "SETRT") { // SETRT <VARIABLE> <TIMEOUT>
        std::unique_lock<std::mutex> lock(cond_mutex);
        if (parameters.size() < 3) {
            std::cout << "Wrong command format, usage: SETRT <VARIABLE> <TIMEOUT>\n";
            return -1;
        }
        ClockTip tip{std::stoul(parameters[2]) * 1000000000, 0};
        int was_empty = write_infinite_requests_.empty();
        write_infinite_requests_[(parameters[1])] = tip;
        if (was_empty)
            cond_var.notify_one();
        return 1;
    } else if (parameters.front() == "SET") {  // SET <VARIABLE> <VALUE>
        std::unique_lock<std::mutex> lock(cond_mutex);
        if (write_infinite_requests_.find(parameters[1]) != write_infinite_requests_.end())
            write_infinite_requests_.erase(parameters[1]);
        return SETHandler::handle(parameters);
    } else if (parameters.front() == "stop") {
        std::unique_lock<std::mutex> lock(cond_mutex);
        write_infinite_requests_.clear();
    }

    return 0;
}

void SETRTHandler::infinite_loop()
{
    while (true) {
        {
            std::unique_lock<std::mutex> lock(cond_mutex);
            if (done)
                break;
            while (write_infinite_requests_.empty()) {  // loop to avoid spurious wakeups
                cond_var.wait(lock);
            }
            for (auto& request : write_infinite_requests_) {
                if (request.second.timeout <
                        (std::chrono::system_clock::now().time_since_epoch().count() - request.second.last_processed_time) ) {
                    SETHandler::handle(std::vector<std::string>{"SET", request.first, get_random_string()});
                    request.second.last_processed_time = std::chrono::system_clock::now().time_since_epoch().count();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int GETTHandler::handle(const std::vector<std::string> &parameters)
{
    if (parameters.front() == "GETT") { // GETT <VARIABLE> <TIMEOUT>
        std::unique_lock<std::mutex> lock(cond_mutex);
        if (parameters.size() < 3) {
            std::cout << "Wrong command format, usage: GETT <VARIABLE> <TIMEOUT>\n";
            return -1;
        }
        ClockTip tip{std::stoul(parameters[2].c_str()) * 1000000000, 0};
        int was_empty = read_infinite_requests_.empty();
        read_infinite_requests_[(parameters[1])] = tip;
        if (was_empty)
            cond_var.notify_one();
        return 1;
    } else if (parameters.front() == "GET") {  // GET <VARIABLE>
        std::unique_lock<std::mutex> lock(cond_mutex);
        if (read_infinite_requests_.find(parameters[1]) != read_infinite_requests_.end())
            read_infinite_requests_.erase(parameters[1]);
        return GETHandler::handle(parameters);
    } else if (parameters.front() == "stop") {
        std::unique_lock<std::mutex> lock(cond_mutex);
        read_infinite_requests_.clear();
    }

    return 0;
}

void GETTHandler::infinite_loop()
{
    while (true) {
        {
            std::unique_lock<std::mutex> lock(cond_mutex);
            if (done)
                break;
            while (read_infinite_requests_.empty()) {  // loop to avoid spurious wakeups
                cond_var.wait(lock);
            }
            for (auto& request : read_infinite_requests_) {
                if (request.second.timeout <
                        (std::chrono::system_clock::now().time_since_epoch().count() - request.second.last_processed_time) ) {
                    GETHandler::handle(std::vector<std::string>{"GET", request.first});
                    request.second.last_processed_time = std::chrono::system_clock::now().time_since_epoch().count();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
