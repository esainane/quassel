#ifndef CORENETWORKBRINECONNECTION_H
#define CORENETWORKBRINECONNECTION_H

#include "corenetworkconnection.h"

class CoreNetwork;
class CoreNetworkBrineConnection : public CoreNetworkConnection
{
public:
    CoreNetworkBrineConnection(CoreNetwork &network, Network::Server &server);
    virtual void performDisconnect(bool requested, bool withReconnect);
    virtual void socketDisconnected(const CoreIdentity *id);
    virtual void write(const QByteArray &data);
private:
    CoreNetwork &_network;
    const Network::Server &_server;
    void *data; // Used by BrineAdapter internally

    friend class BrineAdapter;
};

#endif // CORENETWORKBRINECONNECTION_H
