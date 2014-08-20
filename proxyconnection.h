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

#ifndef PROXYCONNECTION_H
#define PROXYCONNECTION_H

#include <QFile>
#include <QSocketNotifier>
#include <QTcpSocket>

class ProxyConnection : public QTcpSocket
{
    Q_OBJECT
public:
    explicit ProxyConnection(QObject *parent = 0);
    bool connectTarget(const QString &host);

private slots:
    void readProxyClient();
    void receiveData();
    void closeConnections();
    void prepareFileTransfer();
    void continueTransfer(qint64 bytes_written);
    void fileReadyRead(int socket);
    void closeFileConnection();

private:
    bool fileExists(const QString &path, qint64 start_range);
    void sendFile(const QString &path);
    qint64 transferFile();

    QSocketNotifier *m_notifier;
    QTcpSocket *m_target;
    QFile *m_file;
    qint64 m_end_range;
};

#endif // PROXYCONNECTION_H
