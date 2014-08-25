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

#ifndef PSNREQUEST_H
#define PSNREQUEST_H

#include "authcookiejar.h"
#include <QByteArray>
#include <QObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QVariant>

class PSNRequest : public QObject
{
    Q_OBJECT
public:
    enum RequestStatus {SUCCESS, SESSION_EXPIRED = 0x5, AUTH_REQUIRED = 0x6, NEED_REFRESH = 0x99};

    explicit PSNRequest(QObject *parent = 0);
    ~PSNRequest();

    void checkLogin();
    void login(const QString &username, const QString &password);    
    void loginRefresh();
    void requestDownloadList();
    void requestStoreRootUrl();
    void requestUserInfo();
    void requestGameDownload(const QString &contentId, const QString &platform);
    void requestGameCancel(const QString &contentId, const QString &platform);
    void requestDownloadStatus(const QString &platform);

signals:    
    void downloadListReceived(QByteArray);
    void loginFailed();
    void loginStatusReceived(int, QString);    
    void requestStatusReceived(int, QString);
    void loginSucceeded();
    void networkErrorReceived(QString);
    void statusReceived(QVariantList);
    void storeRootUrlReceived(QString);
    void userInfoReceived(QString, QString);
    void userInfoRequestFail();

private slots:    
    void receiveDownloadList();
    void receiveLoginCompleteResponse();
    void receiveLoginResponse();
    void receiveMetadataResponse();
    void receiveRequestResponse();
    void receiveRootUrlReply();
    void receiveStatusList();
    void receiveUserInfo();
    void requestOauthLogin();
    void requestStoreLogin();
    void setLastError(QNetworkReply::NetworkError code);    

private:    
    AuthCookieJar m_cookieJar;
    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply;
    QNetworkReply::NetworkError m_error;
    QString m_authCode;
    QString m_storeReferer;
};

#endif // PSNREQUEST_H
