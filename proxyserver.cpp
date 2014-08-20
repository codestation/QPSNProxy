/**
 * QPSNProxy is an open source software for downloading PSN packages
 * on a PC, then transfer them to a game console via a HTTP proxy.
 *
 *  Copyright (C) 2014 codestation
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "proxyserver.h"
#include "proxyconnection.h"
#include <QDebug>
#include <QTcpSocket>

ProxyServer::ProxyServer(QObject *parent) :
    QTcpServer(parent)
{    
}


void ProxyServer::incomingConnection(qintptr handle)
{
    ProxyConnection* s = new ProxyConnection(this);
    connect(s, SIGNAL(readyRead()), s, SLOT(readProxyClient()));
    connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
    s->setSocketDescriptor(handle);
}

void ProxyServer::discardClient()
{
    qDebug() << "Client disconnected";
    ProxyConnection* socket = (ProxyConnection*)sender();
    socket->deleteLater();
}
