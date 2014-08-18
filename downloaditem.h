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

#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H

#include "psnparser.h"
#include "psnparser.h"

#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QWidget>

namespace Ui {
class DownloadItem;
}

class DownloadItem : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadItem(const TitleInfo &info, const QString &storeRoot, QWidget *parent = 0);
    ~DownloadItem();

    const TitleInfo &getInfo();
    void downloadGameIcon();
    int status();

    static const QString &getPackageDir();
    static bool lessThan(const DownloadItem *s1, const DownloadItem *s2);

    QNetworkAccessManager *m_manager;

private:
    void init();    
    QByteArray downloadTask(const QString &path);
    void resetDownloadState();

    TitleInfo m_info;
    QString m_pkgname;
    QFileInfo m_pkginfo;
    QString m_storeRoot;
    QNetworkReply *m_reply;
    QNetworkReply::NetworkError m_error;
    QFile *m_file;
    bool m_downloading;
    qint64 m_startOffset;
    qint64 m_downloaded;
    Ui::DownloadItem *ui;

private slots:
    void loadGameIcon();
    void clipboardCopy();
    void setLastError(QNetworkReply::NetworkError code);
    void readError(QNetworkReply::NetworkError code);
    void updateDataTransferProgress(qint64 readBytes, qint64 totalBytes);
    void downloadPackage();
    void savePkgPart();
    void packageComplete();    
    void deletePackage();
};

#endif // DOWNLOADITEM_H
