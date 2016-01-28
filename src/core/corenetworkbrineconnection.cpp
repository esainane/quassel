#include "corenetworkbrineconnection.h"

#include "brineadapter.h"

CoreNetworkBrineConnection::CoreNetworkBrineConnection(CoreNetwork &network, Network::Server &server)
    : _network(network), _server(server)
{
}

void CoreNetworkBrineConnection::init()
{
    brineAdapter->connect(this);
}

void CoreNetworkBrineConnection::performDisconnect(bool requested, bool withReconnect)
{
    emit disconnectFromBrine();
}

void CoreNetworkBrineConnection::socketDisconnected(const CoreIdentity *id)
{
    emit disconnectFromBrine();
}

void CoreNetworkBrineConnection::write(const QByteArray &data)
{
    qWarning() << "BRINE: QUASSEL WANTS TO WRITE:" << data;
    emit putLine(data);
}

static QString nickToFakeMask(const QString &handle) {
    return handle;
}

void CoreNetworkBrineConnection::setOwnNick(const QString &nick)
{
    qWarning() << "### QUASSEL ### Setting my nick:" << nick;
    _network.setMyNick(nick);
}

void CoreNetworkBrineConnection::receiveNetworkMessage(const QString &message, int type)
{
    qWarning() << "### QUASSEL ### net_msgrecv called:" << message << "'";
    Message::Type messageType = Message::Server;
    // IRC numerics. 400-600 is error range; 263 is the "try again" reply, which we also want to mark as an error.
    if (type == 263 || (type >= 400 && type < 600)) {
        messageType = Message::Error;
    }
    _network.displayMsg(messageType, BufferInfo::StatusBuffer, "", message);
}
void CoreNetworkBrineConnection::receiveUserMessage(const QString &message, const QString &user)
{
    qWarning() << "### QUASSEL ### user_msgrecv called:" << message;
}
void CoreNetworkBrineConnection::receiveChannelMessage(const QString &message, const QString &sender, const QString &channel)
{
    qWarning() << "### QUASSEL ### chan_msgrecv called:" << message;
    return receiveUserMessage(message, channel); // FIXME
}

void CoreNetworkBrineConnection::addUser(const QString &handle)
{
    qWarning() << "### QUASSEL ### user_add called:" << handle;
    _network.newIrcUser(nickToFakeMask(handle));
    qWarning() << "QUASSEL Added user:" << handle;
}
void CoreNetworkBrineConnection::renameUser(const QString &handle, const QString &newhandle)
{
    qWarning() << "### QUASSEL ### user_rename called:" << handle << " -> " << newhandle;
    IrcUser *u = _network.newIrcUser(nickToFakeMask(handle));
    u->setNick(newhandle);
    _network.coreSession()->renameBuffer(_network.networkId(), newhandle, handle);
    qWarning() << "QUASSEL Renamed user:" << handle << " -> " << newhandle;
}
void CoreNetworkBrineConnection::removeUser(const QString &handle)
{
    qWarning() << "### QUASSEL ### user_remove called:" << handle;
    IrcUser *u = _network.newIrcUser(nickToFakeMask(handle));
    u->quit();
    qWarning() << "QUASSEL Removed user:" << handle;
}

void CoreNetworkBrineConnection::addChannel(const QString &handle)
{
    qWarning() << "### QUASSEL ### chan_add called:" << handle;
    _network.newIrcChannel(QString(handle));
    _network.me()->joinChannel(QString(handle));
    _network.setChannelJoined(QString(handle));
    _network.displayMsg(Message::Server, BufferInfo::ChannelBuffer, handle, "Connected to channel.");
    qWarning() << "QUASSEL Added channel:" << handle;
}
void CoreNetworkBrineConnection::joinChannel(const QString &handle, const QString &channel)
{
    qWarning() << "### QUASSEL ### chan_join called:" << handle << channel;
    IrcUser *u = _network.newIrcUser(nickToFakeMask(handle));
    if (!u) {
        qWarning() << "CoreNetworkBrineConnection::joinChannel: NULL POINTER for " << handle << "!";
    }
    _network.ircChannel(QString(channel))->joinIrcUser(u);
    qWarning() << "QUASSEL Joined:" << handle << "to channel" << channel;
}
