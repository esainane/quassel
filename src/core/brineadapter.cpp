
#ifndef HAVE_BRINE
#error Attempted to build brine adapter without libbrine available!
#endif

#include "brineadapter.h"

#include <brine.h>

#include "corenetworkbrineconnection.h"

BrineAdapter *brineAdapter;

BrineAdapter::BrineAdapter()
{
}

void BrineAdapter::connect(CoreNetworkBrineConnection *coreFacing)
{
    const Network::Server &server = coreFacing->_server;
    int index = server.host.indexOf(':');
    Q_ASSERT(index != -1 && specification.size() > index + 2);
    Q_ASSERT(specification[index + 1] == '/' && specification[index + 2] == '/');
    QByteArray protocol = server.host.left(index).toUtf8(),
            username = server.host.mid(index + 3).toUtf8(),
            password = server.password.toUtf8();
    coreFacing->_network.setConnectionState(Network::ConnectionState::Connecting);
    void *conn = brine_connect(protocol, username, password, 0);
    coreFacing->_network.setConnectionState(Network::ConnectionState::Initializing);
    coreFacing->data = conn;
    _connections.insert(conn, coreFacing);
    QString nick;
    auto it = coreFacing->_network.identityPtr()->nicks().begin();
    if (it == coreFacing->_network.identityPtr()->nicks().end()) {
        nick = "User";
    } else {
        nick = *it;
    }
    coreFacing->_network.setMyNick(nick);
}

void BrineAdapter::disconnect(CoreNetworkBrineConnection *coreFacing)
{
    void *conn = coreFacing->data;
    brine_disconnect(conn);
    _connections.remove(conn);
    coreFacing->data = 0;
}

bool BrineAdapter::compatible(QString specification)
{
    int index = specification.indexOf(':');
    if (index == -1 || specification.size() <= index + 2)
        return false;
    if (specification[index + 1] != '/' || specification[index + 2] != '/')
        return false;
    if (!brine_pluginexists(specification.left(index).toUtf8().data()))
        return false;
    return true;
}

int BrineAdapter::net_msgrecv(BrineConnection connection, const char *message, int type)
{
    auto it = brineAdapter->_connections.find(connection);
    if (it == brineAdapter->_connections.end()) return -1;
    CoreNetworkBrineConnection &conn = **it;
    Message::Type messageType = Message::Server;
    // IRC numerics. 400-600 is error range; 263 is the "try again" reply, which we also want to mark as an error.
    if (type == 263 || (type >= 400 && type < 600)) {
        messageType = Message::Error;
    }
    conn._network.displayMsg(messageType, BufferInfo::StatusBuffer, "", QString(message));
    return 0;
}
int BrineAdapter::user_msgrecv(BrineConnection connection, const char *message, const char *user)
{
    auto it = brineAdapter->_connections.find(connection);
    if (it == brineAdapter->_connections.end()) return -1;
    CoreNetworkBrineConnection &conn = **it;
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
