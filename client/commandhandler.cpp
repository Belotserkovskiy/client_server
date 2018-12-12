#include "commandhandler.h"
#include <random>

static std::string get_random_string()
{
    std::string str;
    srand(time(0));
    int length = rand()%25;
    for (int i = 0; i < length; ++i)
        str += (char)(rand()%(48 + 126) + 48);
    return str;
}


int SETHandler::handle(const std::vector<std::string> &parameters)
{
    if (parameters.front() == "SET") {
        if (parameters.size() < 3) {
            std::cout << "Wrong command format, usage: SET <VARIABLE> <VALUE>\n";
            return -1;
        }
        std::string message_str = std::string("SET ") + parameters[1] + std::string(" ")+ parameters[2];
        Message msg;
        msg.body_length(message_str.size());
        std::memcpy(msg.body(), message_str.data(), msg.body_length());
        msg.encode_header();
        client_->write(msg);
        return 0;
    }
    return -1;
}

int GETHandler::handle(const std::vector<std::string>& parameters)
{
    if (parameters.front() == "GET") {
        if (parameters.size() < 2) {
            std::cout << "Wrong command format, usage: SET <VARIABLE> <VALUE>\n";
            return -1;
        }
        std::string message_str = std::string("GET ") + parameters[1];
        Message msg;
        msg.body_length(message_str.size());
        std::memcpy(msg.body(), message_str.data(), msg.body_length());
        msg.encode_header();
        client_->write(msg);
        return 0;
    }
    return -1;
}

int SETRTHandler::handle(const std::vector<std::string>& parameters)
{
    if (parameters.front() == "SETRT") { // SETRT <VARIABLE> <TIMEOUT>
        std::unique_lock<std::mutex> lock(cond_mutex);
        if (parameters.size() < 3) {
            std::cout << "Wrong command format, usage: SET <VARIABLE> <VALUE>\n";
            return -1;
        }
        ClockTip tip{std::stoul(parameters[2]), 0};
        int was_empty = write_infinite_requests_.empty();
        write_infinite_requests_[(parameters[1])] = tip;
        if (was_empty)
            cond_var.notify_one();
    } else if (parameters.front() == "SET" && !(parameters.size() < 3)) {  // SET <VARIABLE> <VALUE>
        std::unique_lock<std::mutex> lock(cond_mutex);
        if (write_infinite_requests_.find(parameters[1]) != write_infinite_requests_.end())
            write_infinite_requests_.erase(parameters[1]);
        SETHandler::handle(parameters);
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
        ClockTip tip{std::stoul(parameters[2].c_str()), 0};
        int was_empty = read_infinite_requests_.empty();
        read_infinite_requests_[(parameters[1])] = tip;
        if (was_empty)
            cond_var.notify_one();
    } else if (parameters.front() == "GET" && !(parameters.size() < 2)) {  // GET <VARIABLE>
        std::unique_lock<std::mutex> lock(cond_mutex);
        if (read_infinite_requests_.find(parameters[1]) != read_infinite_requests_.end())
            read_infinite_requests_.erase(parameters[1]);
        GETHandler::handle(parameters);
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
