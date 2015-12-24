#ifndef BRINEADAPTER_H
#define BRINEADAPTER_H

#include <QThread>
#include <QHash>

#include "corenetwork.h"

class CoreNetworkBrineConnection;
class BrineAdapter : public QThread
{
public:
    BrineAdapter();

    void run();

    void connect(CoreNetworkBrineConnection *coreFacing);
    void disconnect(CoreNetworkBrineConnection *coreFacing);

    /**
     * Check the parameter is a valid protocol specification, of the form
     * protocol://args, and the procotol is an available plugin.
     *
     * If so, the caller should probably create a brine connection instead
     * of an irc connection.
     *
     * @param specification Server host specification, eg. steam://username
     * @return True if the specification matches an available plugin
     */
    bool compatible(QString specification);
private:
    typedef const void *BrineConnection;
    QHash<BrineConnection, CoreNetworkBrineConnection *> _connections;
    static int net_msgrecv(BrineConnection connection, const char *message, int type);
    static int user_msgrecv(BrineConnection connection, const char *message, const char *user);
    static int chan_msgrecv(BrineConnection connection, const char *message, const char *channel);
};

extern BrineAdapter *brineAdapter;

#endif // BRINEADAPTER_H
