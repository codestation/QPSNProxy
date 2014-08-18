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

#include "psnrequest.h"
#include "json.h"

#include <QDir>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

static const QString userAgent("Mozilla/5.0 (X11; Linux x86_64) "
                               "AppleWebKit/537.36 (KHTML, like Gecko) "
                               "Chrome/35.0.1916.114 Safari/537.36");

static const QString loginUrl("https://auth.api.sonyentertainmentnetwork.com/login.do");

static const QString oauthUrl("https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/"
                              "authorize?response_type=code&"
                              "scope=kamaji:commerce_native,kamaji:commerce_container&"
                              "client_id=f6c7057b-f688-4744-91c0-8179592371d2&"
                              "prompt=none&redirect_uri=%1");

static const QString storeUrl("https://store.sonyentertainmentnetwork.com/kamaji/api/chihiro/"
                              "00_09_000/user/session");

static const QString storeQuery("https://store.sonyentertainmentnetwork.com/kamaji/api/chihiro/"
                                "00_09_000/user/stores");

static const QString userQuery("https://store.sonyentertainmentnetwork.com/kamaji/api/chihiro/"
                               "00_09_000/user/account/name");

static const QString storeSignInReferer("https://store.sonyentertainmentnetwork.com/shared/html/"
                                        "signinRedirect_SSOAuth.html");

static const QString storeRefreshReferer("https://store.sonyentertainmentnetwork.com/shared/html/"
                                         "refreshRedirect_SSOAuth.html");

static const QString queryUrl("https://store.sonyentertainmentnetwork.com/kamaji/api/chihiro/"
                              "00_09_000/gateway/store/v1/users/me/internal_entitlements?"
                              "start=%1&size=%2&fields=game_meta,drm_def");

static const QString requestDownloadUrl("https://store.sonyentertainmentnetwork.com/kamaji/api/chihiro/00_09_000/user/notification/download");

static const QString requestCancelUrl("https://store.sonyentertainmentnetwork.com/kamaji/api/chihiro/00_09_000/user/notification/download/status");

static const QString jsonFilename("listing.json");

PSNRequest::PSNRequest(QObject *parent) :
    QObject(parent), m_manager(this), m_reply(NULL), m_error(QNetworkReply::NoError)
{
    m_cookieJar.load();
    m_manager.setCookieJar(&m_cookieJar);
    m_cookieJar.setParent(0);
}

PSNRequest::~PSNRequest()
{
    m_cookieJar.save();
}

void PSNRequest::setLastError(QNetworkReply::NetworkError code)
{
    qDebug() << "Error in psn network reply: " << code;
    m_error = code;
}

void PSNRequest::checkLogin()
{
    QNetworkRequest request((QUrl(storeQuery)));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    m_reply = m_manager.get(request);

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(receiveLoginResponse()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::receiveLoginResponse()
{
    if(m_error != QNetworkReply::NoError && m_error != QNetworkReply::AuthenticationRequiredError)
    {
        qDebug() << "PSNRequest::receiveLoginResponse error: " << m_error;
        emit networkErrorReceived(m_reply->errorString());
        return;
    }
    QByteArray data = m_reply->readAll();
    QMap<QString,QVariant> json = json_decode(data);
    QMap<QString,QVariant> store_header = json["header"].toMap();
    QString status_code = store_header["status_code"].toString();
    QString message_key = store_header["message_key"].toString();

    bool ok = false;
    int code = status_code.toInt(&ok, 16);

    if(ok && code == 0)
    {
        QMap<QString,QVariant> store_data = json["data"].toMap();
        QString storeRoot = store_data["root_url"].toString();
        qDebug() << "Got store root: " << storeRoot;
        emit storeRootUrlReceived(storeRoot);
    }

    qDebug() << "Login status: " << status_code << ", message: " << message_key;
    emit loginStatusReceived(code, message_key);
}

void PSNRequest::login(const QString &username, const QString &password)
{
    QByteArray postData;
    postData.append("j_username=" + username + "&");
    postData.append("j_password=" + password + "&");
    postData.append("rememberSignIn=on&");
    postData.append("request_locale=en_US&service_entity=psn&disableLinks=SENLink&hidePageElements=SENLogo");

    m_cookieJar.clear();

    QNetworkRequest request((QUrl(loginUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
    qDebug() << "Sending initial login POST";
    m_reply = m_manager.post(request, postData);

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(requestOauthLogin()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::loginRefresh()
{
    QNetworkRequest request((QUrl(oauthUrl.arg(storeRefreshReferer))));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    qDebug() << "Requesting oAuth refresh GET";
    m_reply = m_manager.get(request);
    m_storeReferer = storeRefreshReferer;

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(metaDataChanged()), this, SLOT(receiveMetadataResponse()));
    connect(m_reply, SIGNAL(finished()), this, SLOT(requestStoreLogin()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::requestOauthLogin()
{
    if(m_error != QNetworkReply::NoError)
    {
        qDebug() << "PSNRequest::requestOauthLogin error: " << m_error;
        emit networkErrorReceived(m_reply->errorString());
        return;
    }

    QNetworkRequest request((QUrl(oauthUrl.arg(storeSignInReferer))));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    qDebug() << "Requesting oAuth login GET";
    m_reply = m_manager.get(request);
    m_storeReferer = storeSignInReferer;

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(metaDataChanged()), this, SLOT(receiveMetadataResponse()));
    connect(m_reply, SIGNAL(finished()), this, SLOT(requestStoreLogin()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::receiveMetadataResponse()
{
    QByteArray code = m_reply->rawHeader("X-NP-GRANT-CODE");
    if(!code.isEmpty())
    {
        qDebug() << "Got auth code: " << code.data();
        m_authCode = QString(code);
    }
    else
        qDebug() << "Failed to get auth code";
}

void PSNRequest::requestStoreLogin()
{
    // ignore auth failed, we only need the auth code
    if(m_error != QNetworkReply::NoError && m_error != QNetworkReply::AuthenticationRequiredError)
    {
        qDebug() << "PSNRequest::requestStoreLogin error: " << m_error;
        emit networkErrorReceived(m_reply->errorString());
        return;
    }

    if(m_authCode.isEmpty())
    {
        qWarning() << "No auth code received";
        emit loginFailed();
        return;
    }

    QByteArray postData;
    postData.append("code=" + m_authCode);

    QNetworkRequest request((QUrl(storeUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setRawHeader("X-Alt-Referer", m_storeReferer.toUtf8());
    request.setRawHeader("X-Requested-By", "Chihiro");
    request.setRawHeader("XMLHttpRequest", "XMLHttpRequest");
    request.setRawHeader("Referer", "https://store.sonyentertainmentnetwork.com/");
    request.setRawHeader("Kamaji-Client", "Chihiro-Web");
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

    qDebug() << "Sending final login POST";
    m_reply = m_manager.post(request, postData);
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(receiveLoginCompleteResponse()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::receiveLoginCompleteResponse()
{
    if(m_error != QNetworkReply::NoError && m_error != QNetworkReply::AuthenticationRequiredError)
    {
        qDebug() << "PSNRequest::receiveLoginCompleteResponse error: " << m_error;
        emit networkErrorReceived(m_reply->errorString());
        return;
    }

    qDebug() << "Login OK";
    emit loginSucceeded();
}

void PSNRequest::requestStoreRootUrl()
{
    QNetworkRequest request((QUrl(storeQuery)));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    m_reply = m_manager.get(request);

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(receiveRootUrlReply()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::receiveRootUrlReply()
{
    if(m_error != QNetworkReply::NoError && m_error != QNetworkReply::AuthenticationRequiredError)
    {
        qDebug() << "PSNRequest::receiveRootUrlReply error: " << m_error;
        emit networkErrorReceived(m_reply->errorString());
        return;
    }

    QByteArray data = m_reply->readAll();
    QMap<QString,QVariant> json = json_decode(data);
    QMap<QString,QVariant> store_data = json["data"].toMap();
    QString storeRoot = store_data["root_url"].toString();
    qDebug() << "Got store root: " << storeRoot;
    emit storeRootUrlReceived(storeRoot);
}

void PSNRequest::requestUserInfo()
{
    QNetworkRequest request((QUrl(userQuery)));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    m_reply = m_manager.get(request);

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(receiveUserInfo()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::receiveUserInfo()
{
    if(m_error != QNetworkReply::NoError)
    {
        qDebug() << "PSNRequest::receiveUserInfo error: " << m_error;
        emit networkErrorReceived(m_reply->errorString());
        return;
    }

    QByteArray data = m_reply->readAll();
    qDebug() << "Received user data, " << data.size() << " bytes";
    QMap<QString,QVariant> json = json_decode(data);
    QMap<QString,QVariant> header = json["header"].toMap();

    bool ok;
    int code = header["status_code"].toInt(&ok);
    if(ok && code == 0)
    {
        QMap<QString,QVariant> user_data = json["data"].toMap();
        QString onlineId = user_data["onlineId"].toString();
        QString signId = user_data["signInId"].toString();
        qDebug() << "Got online ID: " << onlineId << ", sign ID: " << signId;
        emit userInfoReceived(onlineId, signId);
    }
    else
    {
        emit userInfoRequestFail();
    }
}

void PSNRequest::requestDownloadList()
{
    QUrl query_url(QString(queryUrl).arg("0", "10240"));

    QNetworkRequest request(query_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    qDebug() << "Requesting download list GET";
    m_reply = m_manager.get(request);

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(receiveDownloadList()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::receiveDownloadList()
{
    if(m_error != QNetworkReply::NoError && m_error != QNetworkReply::AuthenticationRequiredError)
    {
        qDebug() << "PSNRequest::receiveDownloadList error: " << m_error;
        emit networkErrorReceived(m_reply->errorString());
        return;
    }

    QByteArray data = m_reply->readAll();
    qDebug() << "Received list data, " << data.size() << " bytes";
    emit downloadListReceived(data);
}

void PSNRequest::requestGameDownload(const QString &contentId, const QString &platform)
{
    QNetworkRequest request((QUrl(requestDownloadUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setRawHeader("X-Requested-By", "Chihiro");
    request.setRawHeader("XMLHttpRequest", "XMLHttpRequest");
    request.setRawHeader("Referer", "https://store.sonyentertainmentnetwork.com/");
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));

    QVariantList requestList;
    QVariantMap requestMap;

    requestMap["platformString"] = platform;
    requestMap["contentId"] = contentId;
    requestList.append(requestMap);

    QByteArray postData;
    postData.append(json_encode(requestList));

    qDebug() << "Sending download request POST";
    m_reply = m_manager.post(request, postData);
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(receiveRequestResponse()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::requestGameCancel(const QString &contentId, const QString &platform)
{
    QNetworkRequest request((QUrl(requestCancelUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setRawHeader("X-Requested-By", "Chihiro");
    request.setRawHeader("XMLHttpRequest", "XMLHttpRequest");
    request.setRawHeader("Referer", "https://store.sonyentertainmentnetwork.com/");
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));

    QVariantList requestList;
    QVariantMap requestMap;

    requestMap["platformString"] = platform;
    requestMap["contentId"] = contentId;
    requestMap["status"] = QLatin1String("usercancelled");
    requestList.append(requestMap);

    QByteArray postData;
    postData.append(json_encode(requestList));

    qDebug() << "Sending download cancel POST";
    m_reply = m_manager.post(request, postData);
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(receiveRequestResponse()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void PSNRequest::receiveRequestResponse()
{
    if(m_error != QNetworkReply::NoError)
    {
        qDebug() << "PSNRequest::receiveRequestResponse error: " << m_error;
        emit networkErrorReceived(m_reply->errorString());
        return;
    }

    QNetworkReply *reply = reinterpret_cast<QNetworkReply *>(sender());
    QByteArray data = reply->readAll();

    QMap<QString,QVariant> json = json_decode(data);
    QMap<QString,QVariant> store_header = json["header"].toMap();
    QString status_code = store_header["status_code"].toString();
    QString message_key = store_header["message_key"].toString();

    bool ok = false;
    int code = status_code.toInt(&ok, 16);

    qDebug() << "Request status: " << status_code << ", message: " << message_key;
    emit requestStatusReceived(code, message_key);
}
