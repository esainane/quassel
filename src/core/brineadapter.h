#ifndef BRINEADAPTER_H
#define BRINEADAPTER_H

#include <QThread>
#include <QHash>

#include <brine.h>

#include "corenetwork.h"


class CoreNetworkBrineConnection;

/* Basic helper object for forwarding information to the downstream network thread */
class BrineAdapterConnection : public QObject {
    Q_OBJECT
public:
    BrineAdapterConnection(CoreNetworkBrineConnection *net, brine_handle handle);

signals:
    void setOwnNick(const QString &nick);
    void receiveNetworkMessage(const QString &message, int type);
    void receiveUserMessage(const QString &message, const QString &user);
    void receiveChannelMessage(const QString &message, const QString &sender, const QString &channel);
    void addUser(const QString &handle);
    void renameUser(const QString &handle, const QString &newhandle);
    void removeUser(const QString &handle);
    void addChannel(const QString &handle);
    void joinChannel(const QString &handle, const QString &channel);
public slots:
    void putLine(const QByteArray &);
    void disconnectFromBrine();
private:
    brine_handle upstream;

    friend class BrineAdapter;
};

class BrineAdapter : public QThread
{
    typedef brine_handle BrineConnection;
public:
    BrineAdapter();

    void run();

    void connect(CoreNetworkBrineConnection *coreFacing);
    void disconnectFromUpstream(BrineConnection upstream);

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
    /* Do not use */
    void disconnect(CoreNetworkBrineConnection *coreFacing);

    template<typename Fn, Fn fn, typename... Args>
    static int wrap_brine(BrineConnection connection, Args... args);

    QHash<BrineConnection, BrineAdapterConnection *> _connections;
};

extern BrineAdapter *brineAdapter;

#endif // BRINEADAPTER_H
