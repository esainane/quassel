#ifndef CORENETWORKCONNECTION_H
#define CORENETWORKCONNECTION_H

#include "network.h"

#include <QTimer>

class CoreIdentity;
class CoreNetworkConnection : public QObject
{
    Q_OBJECT
public:
    CoreNetworkConnection();
    virtual ~CoreNetworkConnection() {}
    virtual void performConnect(Network::Server &server) = 0;
    virtual void performDisconnect(bool requested, bool withReconnect) = 0;
    virtual void socketDisconnected(const CoreIdentity *id) = 0;
    virtual void write(const QByteArray &data) = 0;
protected:
    bool tryOrQueue(const QByteArray &forQueue);
    void setup();
    void shutdown();
private slots:
    void fillBucketAndProcessQueue();
private:
    QTimer _tokenBucketTimer;
    int _messageDelay;      // token refill speed in ms
    int _burstSize;         // size of the token bucket
    int _tokenBucket;       // the virtual bucket that holds the tokens
    QList<QByteArray> _msgQueue;
};

#endif // CORENETWORKCONNECTION_H
