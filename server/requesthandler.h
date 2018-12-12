#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

class RequestTransmitter
{
public:
    virtual ~RequestTransmitter(){}
    virtual void take_response(std::string message) = 0;
};

class RequestHandler
{
    std::string database_name_;
    sqlite3 *db;
    char *error_message= 0;
    int error_code;
    RequestTransmitter *transmitter_ = nullptr;

public:
    RequestHandler(std::string database_name);
    ~RequestHandler(){ sqlite3_close(db); }

    void handle(std::string message, RequestTransmitter *transmitter);

    void sent_response(std::string response) {
        if (transmitter_)
            transmitter_->take_response(response);
    }

    static int callback(void *data, int argc, char **argv, char **azColName);
};


#endif // REQUESTHANDLER_H
