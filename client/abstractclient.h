#ifndef ABSTRACTCLIENT_H
#define ABSTRACTCLIENT_H

#include "message.h"

class AbstractClient
{
public:
    AbstractClient(){}
    virtual void start() = 0; //blocking function
    virtual void stop() = 0;

    virtual void write(const Message& msg) = 0;
};

#endif // ABSTRACTCLIENT_H
