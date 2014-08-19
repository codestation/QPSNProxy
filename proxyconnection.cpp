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

#include "proxyconnection.h"
#include "downloaditem.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

static const QStringList methods = QStringList() << "GET" << "POST" << "HEAD" << "PUT" << "DELETE" << "TRACE" << "OPTIONS";

ProxyConnection::ProxyConnection(QObject *parent) :
    QTcpSocket(parent), m_notifier(NULL), m_target(NULL), m_file(NULL), m_end_range(-1)
{
}

/**
 * @brief ProxyConnection::readProxyClient
 * Called when new data is available on the proxy to read
 */
void ProxyConnection::readProxyClient()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket *>(sender());

    // send data to real server if there is a connection in place
    if(m_target != NULL)
    {
        m_target->write(socket->readAll());
        return;
    }

    // read header from the proxy client is at least a line is available
    if(socket->canReadLine())
    {
        // read the first line and split it into method/path/protocol
        QStringList tokens = QString(socket->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));
        // read the rest of the headers
        QString headers = socket->readAll();

        // probably an HTTPS connection
        if(tokens[0] == QLatin1String("CONNECT"))
        {
            qDebug("CONNECT %s %s", qPrintable(tokens[1]), qPrintable(tokens[2]));
            // connect to remote server, path is host:port
            if(connectTarget(tokens[1]))
            {
                QTextStream os(socket);
                os.setAutoDetectUnicode(true);

                // send OK to proxy client
                os << "HTTP/1.1 200 Connection established\r\n"
                      "Proxy-agent: QPSNProxy/0.1\r\n"
                      "\r\n";

                // connect slots to read and delete the sockets
                connect(m_target, SIGNAL(readyRead()), this, SLOT(receiveData()));
                connect(m_target, SIGNAL(disconnected()), this, SLOT(closeConnections()));
            }
            else
            {
                // connect failed, close the socket and delete it if is disconnected
                socket->close();
                if (socket->state() == QTcpSocket::UnconnectedState)
                    delete socket;
            }
        }
        // HTTP method is GET, POST, etc
        else if(methods.contains(tokens[0]))
        {
            qDebug("%s %s %s", qPrintable(tokens[0]), qPrintable(tokens[1]), qPrintable(tokens[2]));

            // extract the host from the path
            QString host = tokens[1].mid(7);
            int pos = host.indexOf(QChar('/'));
            QString path = host.mid(pos);
            host = host.left(pos);

            //connect to remote server, using extracted host
            if(connectTarget(host))
            {
                QTextStream os(m_target);
                os.setAutoDetectUnicode(true);

                // send the original method/path/protocol to remote server
                os << tokens[0] << " " << path << " " << tokens[2] << "\r\n" << headers;

                // check if the requested file exist on disk
                if(tokens[0] == "GET")
                {
                    QStringList header_list = headers.split(QRegExp("[\r\n]"));
                    qint64 start_range = 0;

                    foreach(const QString &header, header_list)
                    {
                        if(header.startsWith("Range: "))
                        {
                            QRegExp valid_input("Range: bytes=(\\d+)-(\\d*)");
                            if(valid_input.indexIn(header) != -1)
                            {
                                start_range = valid_input.cap(1).toLongLong();
                                m_end_range = valid_input.cap(2).toLongLong();
                                if(m_end_range == 0)
                                    m_end_range = -1;
                            }
                        }
                    }
                    if(fileExists(path, start_range))
                    {
                        qDebug() << "Package found in cache";
                        connect(m_target, SIGNAL(readyRead()), this, SLOT(prepareFileTransfer()));
                    }
                    else
                    {
                        qDebug() << "Package not found in cache";
                        connect(m_target, SIGNAL(readyRead()), this, SLOT(receiveData()));
                    }
                }
                else
                {
                    qDebug() << "Package not found in cache";
                    connect(m_target, SIGNAL(readyRead()), this, SLOT(receiveData()));
                }

                // delete the remote socket on disconnection
                connect(m_target, SIGNAL(disconnected()), m_target, SLOT(deleteLater()));
        }
            else
            {
                // connect failed, close the socket and delete it if is disconnected
                socket->close();
                if (socket->state() == QTcpSocket::UnconnectedState)
                    delete socket;
            }
        }
    }
}

void ProxyConnection::closeConnections()
{
    m_target->close();
    m_target->deleteLater();
    close();
}

/**
 * @brief ProxyConnection::prepareFileTransfer
 * read the original request from the proxy client and use a local file
 * for the response
 */
void ProxyConnection::prepareFileTransfer()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket *>(sender());

    // read header from the proxy client is at least a line is available
    while(socket->canReadLine())
    {
        // read a line from the header
        QString header = QString(socket->readLine());

        // if the content length is read then change it with the size of the local file
        if(header.startsWith("Content-Length: ", Qt::CaseInsensitive))
        {
            QString newsize(QString("Content-length: %1\r\n").arg(m_file->size()));
            write(newsize.toLocal8Bit());
        }
        else
        {
            // send the header to the proxy client
            write(header.toLocal8Bit());
        }

        // check if the line is the end of the header
        if(header == "\r\n")
        {
            socket->close();
            m_notifier = new QSocketNotifier(m_file->handle(), QSocketNotifier::Read);
            connect(m_notifier, SIGNAL(activated(int)), this, SLOT(fileReadyRead(int)));
            connect(this, SIGNAL(disconnected()), this, SLOT(closeFileConnection()));
            break;
        }
    }
}

/**
 * @brief ProxyConnection::fileReadyRead
 * the file is ready to be read
 * @param socket identifier
 */
void ProxyConnection::fileReadyRead(int)
{
    char buffer[16 * 1024];

    int read_size;
    if(m_end_range != -1 && m_file->pos() + static_cast<int>(sizeof(buffer)) > m_end_range)
        read_size = m_end_range - m_file->pos();
    else
        read_size = sizeof(buffer);

    int read = m_file->read(buffer, read_size);
    if(read > 0 && read_size == sizeof(buffer))
        write(buffer, read);
    else
    {
        m_file->close();
        delete m_file;
        m_file = NULL;
        m_notifier->setEnabled(false);
        close();
    }
}

void ProxyConnection::closeFileConnection()
{
    if(m_file !=NULL)
    {
        m_file->close();
        delete m_file;
        m_file = NULL;
        m_notifier->setEnabled(false);
        m_notifier->deleteLater();
        close();
    }
}

/**
 * @brief ProxyConnection::fileExists
 * Check if the file exists, then leaves it open
 * @param path URL path of the file
 * @return true of the file was opened, false otherwise
 */
bool ProxyConnection::fileExists(const QString &path, qint64 start_range)
{
    QString data_path = DownloadItem::getPackageDir();
    QFileInfo info(QUrl(path).path());
    QFileInfo fileinfo(data_path + QDir::separator() + info.fileName());
    if(fileinfo.exists())
    {
        m_file = new QFile(fileinfo.absoluteFilePath());
        if(!m_file->open(QIODevice::ReadOnly))
        {
            delete m_file;
            return false;
        }
        m_file->seek(start_range);
        return true;
    }
    return false;
}

/**
 * @brief ProxyConnection::receiveData
 * read the data from the remote server, write it on the proxy client socket
 */
void ProxyConnection::receiveData()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    write(socket->readAll());
}

/**
 * @brief ProxyConnection::connectTarget
 * connect to the remote server
 * @param host hostname to connect
 * @return true if the connection was successful, false otherwise
 */
bool ProxyConnection::connectTarget(const QString &host)
{
    QString address;
    qint16 port;
    int pos = host.indexOf(QChar(':'));

    if(pos != -1)
    {
        address = host.left(pos);
        port = host.mid(pos+1).toShort();
    }
    else
    {
        address = host;
        port = 80;
    }

    m_target = new QTcpSocket();

    m_target->connectToHost(address, port);
    return m_target->waitForConnected();
}
