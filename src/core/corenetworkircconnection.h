#ifndef CORENETWORKIRCCONNECTION_H
#define CORENETWORKIRCCONNECTION_H

#include "corenetworkconnection.h"

#ifdef HAVE_SSL
# include <QSslSocket>
# include <QSslError>
#else
# include <QTcpSocket>
#endif

class CoreNetwork;
class Event;

class CoreNetworkIrcConnection : public CoreNetworkConnection
{
    Q_OBJECT
public:
    CoreNetworkIrcConnection(CoreNetwork &network, Network::Server &server);
    ~CoreNetworkIrcConnection();

    void performDisconnect(bool requested, bool withReconnect);
    void socketDisconnected(const CoreIdentity *id);

    inline QAbstractSocket::SocketState socketState() const { return socket.state(); }
    inline bool socketConnected() const { return socket.state() == QAbstractSocket::ConnectedState; }
    inline QHostAddress localAddress() const { return socket.localAddress(); }
    inline QHostAddress peerAddress() const { return socket.peerAddress(); }
    inline quint16 localPort() const { return socket.localPort(); }
    inline quint16 peerPort() const { return socket.peerPort(); }
signals:
    void socketInitialized(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort);
    void socketDisconnected(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort);

    void newEvent(Event *event);
public slots:
    void write(const QByteArray &data);
private slots:
    void socketStateChanged(QAbstractSocket::SocketState);
    void socketHasData();
    void socketError(QAbstractSocket::SocketError);
    void socketInitialized();
    inline void socketCloseTimeout() { socket.abort(); }
#ifdef HAVE_SSL
    void sslErrors(const QList<QSslError> &errors);
#endif
private:
    void encodeAndWrite(const QString &data);
#ifdef HAVE_SSL
    QSslSocket socket;
#else
    QTcpSocket socket;
#endif
    CoreNetwork &_network;

    QTimer _socketCloseTimer;
};

#endif // CORENETWORKIRCCONNECTION_H
