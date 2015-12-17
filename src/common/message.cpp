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

#include "message.h"

#include "util.h"

#include <QDataStream>

Message::Message(const BufferInfo &bufferInfo, Type type, const QString &contents, const QString &sender, Flags flags)
    : _timestamp(QDateTime::currentDateTime().toUTC()),
    _bufferInfo(bufferInfo),
    _contents(contents),
    _sender(sender),
    _type(type),
    _flags(flags)
{
}


Message::Message(const QDateTime &ts, const BufferInfo &bufferInfo, Type type, const QString &contents, const QString &sender, Flags flags)
    : _timestamp(ts),
    _bufferInfo(bufferInfo),
    _contents(contents),
    _sender(sender),
    _type(type),
    _flags(flags)
{
}


QDataStream &operator<<(QDataStream &out, const Message &msg)
{
    return msg.write(out);
}

QDataStream &Message::write(QDataStream &out) const
{
    out << msgId() << (quint32)timestamp().toTime_t() << (quint32)type() << (quint8)flags()
        << bufferInfo() << sender().toUtf8() << contents().toUtf8();
    return out;
}


QDataStream &operator>>(QDataStream &in, Message &msg)
{
    return msg.read(in);
}

QDataStream &Message::read(QDataStream &in)
{
    quint8 f;
    quint32 t;
    quint32 ts;
    QByteArray s, m;
    BufferInfo buf;
    in >> _msgId >> ts >> t >> f >> buf >> s >> m;

    _type = (Message::Type)t;
    _flags = (Message::Flags)f;
    _bufferInfo = buf;
    _timestamp = QDateTime::fromTime_t(ts);
    _sender = QString::fromUtf8(s);
    _contents = QString::fromUtf8(m);
    return in;
}


QDebug operator<<(QDebug dbg, const Message &msg)
{
    dbg.nospace() << qPrintable(QString("Message(MsgId:")) << msg.msgId()
    << qPrintable(QString(",")) << msg.timestamp()
    << qPrintable(QString(", Type:")) << msg.type()
    << qPrintable(QString(", Flags:")) << msg.flags() << qPrintable(QString(")"))
    << msg.sender() << ":" << msg.contents();
    return dbg;
}

DEFINE_VIRTUAL_METATYPE(Message)

FlairedMessage::FlairedMessage(const BufferInfo &bufferInfo, Type type, const QString &contents, const QString &sender, Flags flags, QChar flair)
    : Message(bufferInfo, type, contents, sender, flags) {
    _flair = flair;
}
FlairedMessage::FlairedMessage(const QDateTime &ts, const BufferInfo &bufferInfo, Type type, const QString &contents, const QString &sender, Flags flags, QChar flair)
    : Message(ts, bufferInfo, type, contents, sender, flags), _flair(flair)
{
}

QDataStream &FlairedMessage::write(QDataStream &out) const
{
    Message::write(out);
    out << _flair;
    return out;
}
QDataStream &FlairedMessage::read(QDataStream &in)
{
    Message::read(in);
    in >> _flair;
    return in;
}

QDebug operator<<(QDebug dbg, const FlairedMessage &msg)
{
    dbg.nospace() << qPrintable(QString("FlairedMessage(MsgId:")) << msg.msgId()
    << qPrintable(QString(",")) << msg.timestamp()
    << qPrintable(QString(", Type:")) << msg.type()
    << qPrintable(QString(", Flags:")) << msg.flags() << qPrintable(QString(")"))
    << msg.flair() << msg.sender() << ":" << msg.contents();
    return dbg;
}

DEFINE_LEAF_METATYPE(FlairedMessage)
