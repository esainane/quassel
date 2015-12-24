
#ifndef HAVE_BRINE
#error Attempted to build brine adapter without libbrine available!
#endif

#include "brineadapter.h"

#include <brine.h>

BrineAdapter *brineAdapter;

BrineAdapter::BrineAdapter()
{
}


int BrineAdapter::net_msgrecv(const void *connection, const char *message, int type)
{
    return 0;
}
int BrineAdapter::user_msgrecv(const void *connection, const char *message, const char *user)
{
    return 0;
}
int BrineAdapter::chan_msgrecv(const void *connection, const char *message, const char *channel)
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
