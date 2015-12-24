
#ifndef HAVE_BRINE
#error Attempted to build brine adapter without libbrine available!
#endif

#include "brineadapter.h"

#include <brine.h>

BrineAdapter *brineAdapter;

BrineAdapter::BrineAdapter()
{
}


int BrineAdapter::net_msgrecv(BrineConnection connection, const char *message, int type)
{
    auto it = brineAdapter->_connections.find(connection);
    if (it == brineAdapter->_connections.end()) return -1;
    CoreNetwork &network = **it;
    Message::Type messageType = Message::Server;
    // IRC numerics. 400-600 is error range; 263 is the "try again" reply, which we also want to mark as an error.
    if (type == 263 || (type >= 400 && type < 600)) {
        messageType = Message::Error;
    }
    network.displayMsg(messageType, BufferInfo::StatusBuffer, "", QString(message));
    return 0;
}
int BrineAdapter::user_msgrecv(BrineConnection connection, const char *message, const char *user)
{
    auto it = brineAdapter->_connections.find(connection);
    if (it == brineAdapter->_connections.end()) return -1;
    CoreNetwork &network = **it;
    return 0;
}
int BrineAdapter::chan_msgrecv(BrineConnection connection, const char *message, const char *channel)
{
    return user_msgrecv(connection, message, channel); // FIXME
}

void BrineAdapter::run()
{
    struct brine b = {
        .net_msgrecv = net_msgrecv,
        .user_msgrecv = user_msgrecv,
        .chan_msgrecv = chan_msgrecv
    };
    brine_init(&b);
    brine_eventloop();
}
