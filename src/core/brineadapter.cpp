
#ifndef HAVE_BRINE
#error Attempted to build brine adapter without libbrine available!
#endif

#include "brineadapter.h"

#include "corenetworkbrineconnection.h"

BrineAdapter *brineAdapter;

BrineAdapterConnection::BrineAdapterConnection(CoreNetworkBrineConnection *net, brine_handle handle) : upstream(handle) {
    connect(this, SIGNAL(setOwnNick(QString)), net, SLOT(setOwnNick(QString)));
    connect(this, SIGNAL(receiveNetworkMessage(const QString &,int)), net, SLOT(receiveNetworkMessage(const QString&,int)));
    connect(this, SIGNAL(receiveUserMessage(const QString &,const QString &)), net, SLOT(receiveUserMessage(const QString&,const QString&)));
    connect(this, SIGNAL(receiveChannelMessage(const QString &,const QString &,const QString &)), net, SLOT(receiveChannelMessage(const QString&,const QString&,const QString&)));
    connect(this, SIGNAL(addUser(const QString &)), net, SLOT(addUser(const QString&)));
    connect(this, SIGNAL(renameUser(const QString &,const QString &)), net, SLOT(renameUser(const QString&,const QString&)));
    connect(this, SIGNAL(removeUser(const QString &)), net, SLOT(removeUser(const QString&)));
    connect(this, SIGNAL(addChannel(const QString &)), net, SLOT(addChannel(const QString&)));
    connect(this, SIGNAL(joinChannel(const QString &,const QString &)), net, SLOT(joinChannel(const QString&,const QString&)));
    connect(net, SIGNAL(putLine(const QByteArray &)), this, SLOT(putLine(const QByteArray &)));
    connect(net, SIGNAL(disconnectFromBrine()), this, SLOT(disconnectFromBrine()));
}

void BrineAdapterConnection::putLine(const QByteArray &line) {
    QByteArray dcopy = line;
    char *data = dcopy.data();
    brine_putline(upstream, data);
}

void BrineAdapterConnection::disconnectFromBrine() {
    brineAdapter->disconnectFromUpstream(upstream);
}

BrineAdapter::BrineAdapter()
{
}

/* FIXME: Quick hack */
struct set;
int set_setstr(struct set **head, const char *key, char *value);
static void config(struct set **set) {
    QByteArray tr = QString("true").toLocal8Bit();
    char *b = tr.data();
    set_setstr(set, "auto_join", b);
    set_setstr(set, "read_groups", b);
    set_setstr(set, "skypeconsole_receive", b);
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
    brine_handle conn = brine_conn_init(protocol, username, password, config);
    _connections.insert(conn, new BrineAdapterConnection(coreFacing, conn));
    coreFacing->_network.setConnectionState(Network::ConnectionState::Initializing);
    brine_conn_login(conn);

    if (coreFacing->_network.myNick().isEmpty()) {
        qWarning() << "Nick was not set during brine connection. Making best guess based on identity!";
        QString nick;
        auto it = coreFacing->_network.identityPtr()->nicks().begin();
        if (it == coreFacing->_network.identityPtr()->nicks().end()) {
            nick = "User";
        } else {
            nick = *it;
        }
        coreFacing->_network.setMyNick(nick);
    }
}

void BrineAdapter::disconnect(CoreNetworkBrineConnection *coreFacing)
{
    qCritical() << "BrineAdapter::disconnect called, this should never happen!" << coreFacing;
}

void BrineAdapter::disconnectFromUpstream(BrineConnection upstream)
{
    brine_disconnect(upstream);
    _connections.remove(upstream);
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

/* Basic belper to forward the C bindings to a member function */
/* We also get nice implicit conversions from char * to QString here */
template<typename Fn, Fn fn, typename... Args>
int BrineAdapter::wrap_brine(BrineConnection connection, Args... args) {
    auto it = brineAdapter->_connections.find(connection);
    if (it == brineAdapter->_connections.end()) {
        qWarning() << "BRINE ADAPTER: Was not able to find connection, dropping!";
        return -1;
    }
    BrineAdapterConnection &conn = **it;
    emit (conn.*fn)(std::forward<Args>(args)...);
    return 0;
}
#define WRAPPER(FUNC) wrap_brine<decltype(&FUNC), &FUNC>

void BrineAdapter::run()
{
    struct brine b = {
        .net_setnick = WRAPPER(BrineAdapterConnection::setOwnNick),
        .net_msgrecv = WRAPPER(BrineAdapterConnection::receiveNetworkMessage),
        .user_msgrecv = WRAPPER(BrineAdapterConnection::receiveUserMessage),
        .chan_msgrecv = WRAPPER(BrineAdapterConnection::receiveChannelMessage),
        .user_add = WRAPPER(BrineAdapterConnection::addUser),
        .user_rename = WRAPPER(BrineAdapterConnection::renameUser),
        .user_remove = WRAPPER(BrineAdapterConnection::removeUser),
        .chan_add = WRAPPER(BrineAdapterConnection::addChannel),
        .chan_join = WRAPPER(BrineAdapterConnection::joinChannel)
    };
    brine_init(&b);
    brine_eventloop();
}
