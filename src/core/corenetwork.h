/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef CORENETWORK_H
#define CORENETWORK_H

#include "network.h"
#include "coreircchannel.h"
#include "coreircuser.h"

#include <QTimer>

#ifdef HAVE_QCA2
#  include "cipher.h"
#endif

#include "coresession.h"

#include <functional>

class CoreIdentity;
class CoreNetworkConnection;
class CoreUserInputHandler;
class CoreIgnoreListManager;
class Event;

class CoreNetwork : public Network
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreNetwork(const NetworkId &networkid, CoreSession *session);
    ~CoreNetwork();
    inline virtual const QMetaObject *syncMetaObject() const { return &Network::staticMetaObject; }

    inline CoreIdentity *identityPtr() const { return coreSession()->identity(identity()); }
    inline CoreSession *coreSession() const { return _coreSession; }
    inline CoreNetworkConfig *networkConfig() const { return coreSession()->networkConfig(); }

    inline CoreUserInputHandler *userInputHandler() const { return _userInputHandler; }
    inline CoreIgnoreListManager *ignoreListManager() { return coreSession()->ignoreListManager(); }

    //! Decode a string using the server (network) decoding.
    inline QString serverDecode(const QByteArray &string) const { return decodeServerString(string); }

    //! Decode a string using a channel-specific encoding if one is set (and use the standard encoding else).
    QString channelDecode(const QString &channelName, const QByteArray &string) const;

    //! Decode a string using an IrcUser-specific encoding, if one exists (using the standaed encoding else).
    QString userDecode(const QString &userNick, const QByteArray &string) const;

    //! Encode a string using the server (network) encoding.
    inline QByteArray serverEncode(const QString &string) const { return encodeServerString(string); }

    //! Encode a string using the channel-specific encoding, if set, and use the standard encoding else.
    QByteArray channelEncode(const QString &channelName, const QString &string) const;

    //! Encode a string using the user-specific encoding, if set, and use the standard encoding else.
    QByteArray userEncode(const QString &userNick, const QString &string) const;

    inline QString channelKey(const QString &channel) const { return _channelKeys.value(channel.toLower(), QString()); }

    inline QByteArray readChannelCipherKey(const QString &channel) const { return _cipherKeys.value(channel.toLower()); }
    inline void storeChannelCipherKey(const QString &channel, const QByteArray &key) { _cipherKeys[channel.toLower()] = key; }

    inline bool isAutoWhoInProgress(const QString &channel) const { return _autoWhoPending.value(channel.toLower(), 0); }

    inline UserId userId() const { return _coreSession->user(); }

    QList<QList<QByteArray>> splitMessage(const QString &cmd, const QString &message, std::function<QList<QByteArray>(QString &)> cmdGenerator);

public slots:
    virtual void setMyNick(const QString &mynick);

    virtual void requestConnect() const;
    virtual void requestDisconnect() const;
    virtual void requestSetNetworkInfo(const NetworkInfo &info);

    virtual void setUseAutoReconnect(bool);
    virtual void setAutoReconnectInterval(quint32);
    virtual void setAutoReconnectRetries(quint16);

    void setPingInterval(int interval);

    void connectToIrc(bool reconnecting = false);
    void disconnectFromIrc(bool requested = true, const QString &reason = QString(), bool withReconnect = false);

    void userInput(BufferInfo bufferInfo, QString msg);
    void putRawLine(QByteArray input);
    void putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix = QByteArray());
    void putCmd(const QString &cmd, const QList<QList<QByteArray>> &params, const QByteArray &prefix = QByteArray());

    void setChannelJoined(const QString &channel);
    void setChannelParted(const QString &channel);
    void addChannelKey(const QString &channel, const QString &key);
    void removeChannelKey(const QString &channel);

    // Blowfish stuff
#ifdef HAVE_QCA2
    Cipher *cipher(const QString &recipient);
    QByteArray cipherKey(const QString &recipient) const;
    void setCipherKey(const QString &recipient, const QByteArray &key);
    bool cipherUsesCBC(const QString &target);
#endif

    void setAutoWhoEnabled(bool enabled);
    void setAutoWhoInterval(int interval);
    void setAutoWhoDelay(int delay);

    bool setAutoWhoDone(const QString &channel);

    void updateIssuedModes(const QString &requestedModes);
    void updatePersistentModes(QString addModes, QString removeModes);
    void resetPersistentModes();

    Server usedServer() const;

    inline void resetPingTimeout() { _pingCount = 0; }

    inline void displayMsg(Message::Type msgType, BufferInfo::Type bufferType, const QString &target, const QString &text, const QString &sender = "", Message::Flags flags = Message::None)
    {
        emit displayMsg(networkId(), msgType, bufferType, target, text, sender, flags);
    }


signals:
    void recvRawServerMsg(QString);
    void displayStatusMsg(QString);
    void displayMsg(NetworkId, Message::Type, BufferInfo::Type, const QString &target, const QString &text, const QString &sender = "", Message::Flags flags = Message::None);
    void disconnected(NetworkId networkId);
    void connectionError(const QString &errorMsg);

    void quitRequested(NetworkId networkId);
    void sslErrors(const QVariant &errorData);

protected:
    inline virtual IrcChannel *ircChannelFactory(const QString &channelname) { return new CoreIrcChannel(channelname, this); }
    inline virtual IrcUser *ircUserFactory(const QString &hostmask) { return new CoreIrcUser(hostmask, this); }

protected slots:
    // TODO: remove cached cipher keys, when appropriate
    //virtual void removeIrcUser(IrcUser *ircuser);
    //virtual void removeIrcChannel(IrcChannel *ircChannel);
    //virtual void removeChansAndUsers();

private slots:
    void socketDisconnected();
    void networkInitialized();

    void sendPerform();
    void restoreUserModes();
    void doAutoReconnect();
    void sendPing();
    void enablePingTimeout(bool enable = true);
    void disablePingTimeout();
    void sendAutoWho();
    void startAutoWhoCycle();

private:
    CoreSession *_coreSession;

    CoreNetworkConnection *connection;

    CoreUserInputHandler *_userInputHandler;

    QHash<QString, QString> _channelKeys; // stores persistent channels and their passwords, if any

    QTimer _autoReconnectTimer;
    int _autoReconnectCount;

    /* this flag triggers quitRequested() once the socket is closed
     * it is needed to determine whether or not the connection needs to be
     * in the automatic session restore. */
    bool _quitRequested;
    QString _quitReason;

    bool _previousConnectionAttemptFailed;
    int _lastUsedServerIndex;

    QTimer _pingTimer;
    uint _lastPingTime;
    uint _pingCount;
    bool _sendPings;

    QStringList _autoWhoQueue;
    QHash<QString, int> _autoWhoPending;
    QTimer _autoWhoTimer, _autoWhoCycleTimer;

    QString _requestedUserModes; // 2 strings separated by a '-' character. first part are requested modes to add, the second to remove

    // List of blowfish keys for channels
    QHash<QString, QByteArray> _cipherKeys;

    friend class CoreNetworkIrcConnection;
};


#endif //CORENETWORK_H
