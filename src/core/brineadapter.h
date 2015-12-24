#ifndef BRINEADAPTER_H
#define BRINEADAPTER_H

#include <QThread>
#include <QHash>

#include "corenetwork.h"

class BrineAdapter : public QThread
{
public:
    BrineAdapter();

    void run();
private:
    QHash<const void *, CoreNetwork *> _connections;
    static int net_msgrecv(const void *connection, const char *message, int type);
    static int user_msgrecv(const void *connection, const char *message, const char *user);
    static int chan_msgrecv(const void *connection, const char *message, const char *channel);
};

extern BrineAdapter *brineAdapter;

#endif // BRINEADAPTER_H
