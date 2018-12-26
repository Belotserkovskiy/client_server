#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <iostream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <random>
#include "logger.h"
#include "message.h"
#include "abstractclient.h"
#include <../data_model/data_model.pb.h>

enum RequestType
{
    SET,
    SETRT,
    GET,
    GETT
};

struct ClockTip
{
    uint64_t timeout;
    uint64_t last_processed_time;
};

class CommandHandler
{
protected:
    AbstractClient *client_;
public:
    CommandHandler(AbstractClient* client) : client_(client){}
    virtual ~CommandHandler(){}

    // return code: 1 - appropriate handler, 0 - not appropriate handler, -1 - appropiate handler, but incorrect request
    virtual int handle(const std::vector<std::string>& parameters) = 0;
};

// SET <VARIABLE> <VALUE>
class SETHandler : public CommandHandler
{
public:
    SETHandler(AbstractClient* client) : CommandHandler(client) {}
    ~SETHandler(){}

    virtual int handle(const std::vector<std::string>& parameters) override;

private:
    void log(const tutorial::Request& request);
};

// GET <VARIABLE>
class GETHandler : public CommandHandler
{
public:
    GETHandler(AbstractClient* client) : CommandHandler(client) {}
    ~GETHandler(){}

    virtual int handle(const std::vector<std::string>& parameters) override;
};

// SETRT <VARIABLE> <TIMEOUT>  &&  SET <VARIABLE> <VALUE>
class SETRTHandler : public SETHandler
{
    std::mutex cond_mutex;
     std::condition_variable cond_var;
     std::map<std::string, ClockTip> write_infinite_requests_;
     bool done = false;

    std::thread infinite_tread;
public:
    SETRTHandler(AbstractClient* client) : SETHandler(client) {
        infinite_tread = std::thread(&SETRTHandler::infinite_loop, this);
        srand(time(0));
    }
    ~SETRTHandler() {
        {
            std::unique_lock<std::mutex> lock(cond_mutex);
            done = true;
        }
        infinite_tread.join();
    }

    virtual int handle(const std::vector<std::string>& parameters) override;

    void infinite_loop();
};

// GETT <VARIABLE> <TIMEOUT>
class GETTHandler : public GETHandler
{
    std::mutex cond_mutex;
     std::condition_variable cond_var;
     std::map<std::string, ClockTip> read_infinite_requests_;
     bool done = false;

    std::thread infinite_tread;
public:
    GETTHandler(AbstractClient* client) : GETHandler(client) {
        infinite_tread = std::thread(&GETTHandler::infinite_loop, this);
    }
    ~GETTHandler() {
        {
            std::unique_lock<std::mutex> lock(cond_mutex);
            done = true;
        }
        infinite_tread.join();
    }

    virtual int handle(const std::vector<std::string>& parameters) override;

    void infinite_loop();
};

#endif // COMMANDHANDLER_H
