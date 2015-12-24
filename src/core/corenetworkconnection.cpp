#include "corenetworkconnection.h"

CoreNetworkConnection::CoreNetworkConnection() {
    connect(&_tokenBucketTimer, SIGNAL(timeout()), this, SLOT(fillBucketAndProcessQueue()));
}

void CoreNetworkConnection::fillBucketAndProcessQueue()
{
    if (_tokenBucket < _burstSize) {
        _tokenBucket++;
    }

    while (_msgQueue.size() > 0 && _tokenBucket > 0) {
        write(_msgQueue.takeFirst());
    }
}

bool CoreNetworkConnection::tryOrQueue(const QByteArray &forQueue) {
    if (_tokenBucket > 0) {
        --_tokenBucket;
        return true;
    }
    _msgQueue.append(forQueue);
    return false;
}

void CoreNetworkConnection::setup() {
    // TokenBucket to avoid sending too much at once
    _messageDelay = 2200;  // this seems to be a safe value (2.2 seconds delay)
    _burstSize = 5;
    _tokenBucket = _burstSize; // init with a full bucket
    _tokenBucketTimer.start(_messageDelay);
}

void CoreNetworkConnection::shutdown() {
    _msgQueue.clear();
    _tokenBucketTimer.stop();
}
