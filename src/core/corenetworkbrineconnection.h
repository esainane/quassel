#ifndef CORENETWORKBRINECONNECTION_H
#define CORENETWORKBRINECONNECTION_H

#include "corenetworkconnection.h"

#include "message.h"

class CoreNetwork;
class CoreNetworkBrineConnection : public CoreNetworkConnection
{
    Q_OBJECT
public:
    CoreNetworkBrineConnection(CoreNetwork &network, Network::Server &server);
    virtual void performDisconnect(bool requested, bool withReconnect);
    virtual void socketDisconnected(const CoreIdentity *id);
    virtual void write(const QByteArray &data);
    virtual void init();
private:
    typedef const void *BrineConnection;
public slots:
    void setOwnNick(const QString &nick); /* TEST */
    void receiveNetworkMessage(const QString &message, int type);
    void receiveUserMessage(const QString &message, const QString &user);
    void receiveChannelMessage(const QString &message, const QString &sender, const QString &channel);
    void addUser(const QString &handle);
    void renameUser(const QString &handle, const QString &newhandle);
    void removeUser(const QString &handle);
    void addChannel(const QString &handle);
    void joinChannel(const QString &handle, const QString &channel);
signals:
    void putLine(const QByteArray &data);
    void disconnectFromBrine();
private:
    CoreNetwork &_network;
    const Network::Server &_server;

    friend class BrineAdapter;
};

#endif // CORENETWORKBRINECONNECTION_H
