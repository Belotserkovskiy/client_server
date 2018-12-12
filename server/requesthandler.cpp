#include "requesthandler.h"

const char* create_table_request = "CREATE TABLE IF NOT EXISTS VARIABLES("  \
                           "VAR_NAME TEXT PRIMARY KEY  NOT NULL," \
                           "VAR_VALUE TEXT             NOT NULL);";

static std::vector<std::string> split_string(const std::string& str)
{
    std::string token;
    std::istringstream tokenStream(str);
    std::vector<std::string> ret;

    while (std::getline(tokenStream, token, ' '))
        ret.push_back(token);
    return ret;
}

static std::string SQL_request_generate(const std::string& message)
{
    auto parameters = split_string(message);
    std::ostringstream output;
    if (parameters.front() == "GET" && !(parameters.size() < 2)) {
        output << "SELECT * from VARIABLES WHERE VAR_NAME = '" << parameters[1] << "';";
    } else if (parameters.front() == "SET" && !(parameters.size() < 3)) {
        output << "INSERT OR REPLACE INTO VARIABLES (VAR_NAME,VAR_VALUE) VALUES ('" << parameters[1] << "', '"
               << parameters[2] << "');";
    }
    return output.str();
}

RequestHandler::RequestHandler(std::string database_name) : database_name_(database_name) {
    error_code = sqlite3_open("test.db", &db); // todo handle code
    error_code = sqlite3_exec(db, create_table_request, nullptr, nullptr, &error_message);
}

void RequestHandler::handle(std::string message, RequestTransmitter *transmitter){
    transmitter_ = transmitter;
    auto SQL_request = SQL_request_generate(message);
    if (!SQL_request.empty()) {
        error_code = sqlite3_exec(db, SQL_request.c_str(), &RequestHandler::callback, this, &error_message);
        if (error_code && transmitter)
            transmitter->take_response(error_message);
    }
    else
        transmitter->take_response("cannot recognize message!");
}

int RequestHandler::callback(void *data, int argc, char **argv, char **azColName) {
    std::ostringstream output;
    for(int i = 0; i<argc; i++){
        output << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << "\n";
    }
    RequestHandler *self = reinterpret_cast<RequestHandler*>(data);
    self->sent_response(output.str());
    return 0;
}
