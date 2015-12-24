#include "corenetworkbrineconnection.h"

#include "brineadapter.h"

CoreNetworkBrineConnection::CoreNetworkBrineConnection(CoreNetwork &network, Network::Server &server)
    : _network(network), _server(server)
{
    brineAdapter->connect(this);
}

void CoreNetworkBrineConnection::performDisconnect(bool requested, bool withReconnect)
{

}

void CoreNetworkBrineConnection::socketDisconnected(const CoreIdentity *id)
{

}

void CoreNetworkBrineConnection::write(const QByteArray &data)
{

}
