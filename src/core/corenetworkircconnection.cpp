#include "corenetworkircconnection.h"


#include <QHostInfo>

#include "core.h"
#include "corenetwork.h"
#include "coreuserinputhandler.h"
#include "eventmanager.h"
#include "networkevent.h"

CoreNetworkIrcConnection::CoreNetworkIrcConnection(CoreNetwork &network, Network::Server &server)
    : _network(network)
{
    connect(&_socketCloseTimer, SIGNAL(timeout()), this, SLOT(socketCloseTimeout()));

    connect(&socket, SIGNAL(connected()), this, SLOT(socketInitialized()));
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));
#ifdef HAVE_SSL
    connect(&socket, SIGNAL(encrypted()), this, SLOT(socketInitialized()));
    connect(&socket, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors(const QList<QSslError> &)));
#endif

    connect(this, SIGNAL(newEvent(Event *)), network.coreSession()->eventManager(), SLOT(postEvent(Event *)));

    if (Quassel::isOptionSet("oidentd")) {
        connect(this, SIGNAL(socketInitialized(const CoreIdentity*, QHostAddress, quint16, QHostAddress, quint16)), Core::instance()->oidentdConfigGenerator(), SLOT(addSocket(const CoreIdentity*, QHostAddress, quint16, QHostAddress, quint16)), Qt::BlockingQueuedConnection);
        connect(this, SIGNAL(socketDisconnected(const CoreIdentity*, QHostAddress, quint16, QHostAddress, quint16)), Core::instance()->oidentdConfigGenerator(), SLOT(removeSocket(const CoreIdentity*, QHostAddress, quint16, QHostAddress, quint16)));
    }

    if (server.useProxy) {
        QNetworkProxy proxy((QNetworkProxy::ProxyType)server.proxyType, server.proxyHost, server.proxyPort, server.proxyUser, server.proxyPass);
        socket.setProxy(proxy);
    }
    else {
        socket.setProxy(QNetworkProxy::NoProxy);
    }

    _network.enablePingTimeout();

    // Qt caches DNS entries for a minute, resulting in round-robin (e.g. for chat.freenode.net) not working if several users
    // connect at a similar time. QHostInfo::fromName(), however, always performs a fresh lookup, overwriting the cache entry.
    QHostInfo::fromName(server.host);

#ifdef HAVE_SSL
    if (server.useSsl) {
        CoreIdentity *identity = _network.identityPtr();
        if (identity) {
            socket.setLocalCertificate(identity->sslCert());
            socket.setPrivateKey(identity->sslKey());
        }
        socket.connectToHostEncrypted(server.host, server.port);
    }
    else {
        socket.connectToHost(server.host, server.port);
    }
#else
    socket.connectToHost(server.host, server.port);
#endif
}

CoreNetworkIrcConnection::~CoreNetworkIrcConnection()
{
    disconnect(&socket, 0, this, 0); // this keeps the socket from triggering events during clean up
}

void CoreNetworkIrcConnection::performDisconnect(bool requested, bool withReconnect) {
    shutdown();
    if (socket.state() == QAbstractSocket::UnconnectedState) {
        _network.socketDisconnected();
    } else {
        if (socket.state() == QAbstractSocket::ConnectedState) {
            _network.userInputHandler()->issueQuit(_network._quitReason);
        } else {
            socket.close();
        }
        if (requested || withReconnect) {
            // the irc server has 10 seconds to close the socket
            _socketCloseTimer.start(10000);
        }
    }
}

void CoreNetworkIrcConnection::socketDisconnected(const CoreIdentity *id)
{
    shutdown();
    _socketCloseTimer.stop();
    emit socketDisconnected(id, localAddress(), localPort(), peerAddress(), peerPort());
}

void CoreNetworkIrcConnection::socketHasData()
{
    while (socket.canReadLine()) {
        QByteArray s = socket.readLine();
        if (s.endsWith("\r\n"))
            s.chop(2);
        else if (s.endsWith("\n"))
            s.chop(1);
        NetworkDataEvent *event = new NetworkDataEvent(EventManager::NetworkIncoming, &_network, s);
        event->setTimestamp(QDateTime::currentDateTimeUtc());
        emit newEvent(event);
    }
}

void CoreNetworkIrcConnection::socketError(QAbstractSocket::SocketError error)
{
    if (_network._quitRequested && error == QAbstractSocket::RemoteHostClosedError)
        return;

    _network._previousConnectionAttemptFailed = true;
    qWarning() << qPrintable(tr("Could not connect to %1 (%2)").arg(_network.networkName(), socket.errorString()));
    _network.displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Connection failure: %1").arg(socket.errorString()));
    _network.emitConnectionError(socket.errorString());
    if (socket.state() < QAbstractSocket::ConnectedState) {
        _network.socketDisconnected();
    }
}

void CoreNetworkIrcConnection::socketInitialized()
{
    CoreIdentity *identity = _network.identityPtr();
    if (!identity) {
        qCritical() << "Identity invalid!";
        _network.disconnectFromIrc();
        return;
    }

    Network::Server server = _network.usedServer();

#ifdef HAVE_SSL
    // Non-SSL connections enter here only once, always emit socketInitialized(...) in these cases
    // SSL connections call socketInitialized() twice, only emit socketInitialized(...) on the first (not yet encrypted) run
    if (!server.useSsl || !socket.isEncrypted()) {
        emit socketInitialized(identity, localAddress(), localPort(), peerAddress(), peerPort());
    }

    if (server.useSsl && !socket.isEncrypted()) {
        // We'll finish setup once we're encrypted, and called again
        return;
    }
#else
    emit socketInitialized(identity, localAddress(), localPort(), peerAddress(), peerPort());
#endif

    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);

    setup();

    if (_network.networkInfo().useSasl) {
        encodeAndWrite(QString("CAP REQ :sasl"));
    }
    if (!server.password.isEmpty()) {
        encodeAndWrite(QString("PASS %1").arg(server.password));
    }
    QString nick;
    if (identity->nicks().isEmpty()) {
        nick = "quassel";
        qWarning() << "CoreNetwork::socketInitialized(): no nicks supplied for identity Id" << identity->id();
    }
    else {
        nick = identity->nicks()[0];
    }
    encodeAndWrite(QString("NICK :%1").arg(nick));
    encodeAndWrite(QString("USER %1 8 * :%2").arg(identity->ident(), identity->realName()));
}

void CoreNetworkIrcConnection::socketStateChanged(QAbstractSocket::SocketState socketState)
{
    Network::ConnectionState state;
    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        state = Network::Disconnected;
        _network.socketDisconnected();
        break;
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
        state = Network::Connecting;
        break;
    case QAbstractSocket::ConnectedState:
        state = Network::Initializing;
        break;
    case QAbstractSocket::ClosingState:
        state = Network::Disconnecting;
        break;
    default:
        state = Network::Disconnected;
    }
    _network.setConnectionState(state);
}

#ifdef HAVE_SSL
void CoreNetworkIrcConnection::sslErrors(const QList<QSslError> &sslErrors)
{
    Q_UNUSED(sslErrors)
    socket.ignoreSslErrors();
    // TODO errorhandling
}
#endif  // HAVE_SSL

void CoreNetworkIrcConnection::encodeAndWrite(const QString &data)
{
    write(_network.serverEncode(data));
}

void CoreNetworkIrcConnection::write(const QByteArray &data)
{
    if (tryOrQueue(data)) {
        socket.write(data);
        socket.write("\r\n");
    }
}
