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

#include "downloaditem.h"
#include "ui_downloaditem.h"
#include "utils.h"

#include <QDebug>
#include <QDir>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QStandardPaths>

static const QString imageUrl("%1/%2/image?_version=00_09_000&platform=chihiro&w=124&h=124&bg_color=000000&opacity=100");

static const QString userAgent("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/35.0.1916.114 Safari/537.36");

static const QString gameTemplate("<html><head/><body>"
        "<p><span style=\" font-size:14pt; font-weight:600;\">%1</span></p>"
        "</body></html>");

static const QString sizeTemplate("<html><head/><body>"
        "<p><span style=\" font-size:12pt;\">%1</span></p>"
        "</body></html>");

static const QString infoTemplate("<html><head/><body>"
        "<p><span style=\" font-size:12pt;\">&nbsp;%1</span></p>"
        "</body></html>");

DownloadItem::DownloadItem(const TitleInfo &info, const QString &storeRoot, QWidget *parent) :
    QWidget(parent), m_info(info), m_storeRoot(storeRoot),
    m_reply(NULL), m_error(QNetworkReply::NoError),
    m_file(NULL), m_downloading(false), m_startOffset(0),
    m_downloaded(0),
    ui(new Ui::DownloadItem)
{
    ui->setupUi(this);
    init();
}

void DownloadItem::init()
{
    ui->gameLabel->setText(gameTemplate.arg(m_info.gameName));
    ui->sizeLabel->setText(sizeTemplate.arg(readable_size(m_info.packageSize, true)));

    QString consoleType;
    switch(m_info.consoleType) {
    case PS3:
        consoleType = "PS3 Content";
        break;
    case PSVITA:
        consoleType = "PS Vita Content";
        break;
    case PSP:
        consoleType = "PSP/PSOne Content";
        break;
    case PS4:
        consoleType = "PS4 Content";
        break;
    default:
        consoleType = "Playstation Title";
        break;
    }

    ui->infoLabel->setText(infoTemplate.arg(consoleType));

    connect(ui->copyButton, SIGNAL(clicked()), this, SLOT(clipboardCopy()));
    connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(downloadPackage()));
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(deletePackage()));

    m_pkgname = QFileInfo(QUrl(m_info.packageUrl).path()).fileName();
    m_pkginfo = QFileInfo(getPackageDir() + QDir::separator() + m_pkgname);

    m_startOffset = m_pkginfo.exists() ? m_pkginfo.size() : 0;
    updateDataTransferProgress(0, m_info.packageSize - m_startOffset);
    if(m_startOffset == m_info.packageSize)
        ui->downloadButton->setIcon(QIcon(":/main/resources/images/dialog-ok-apply.svg"));

    if(m_startOffset > 0)
        ui->deleteButton->setEnabled(true);
}

QString DownloadItem::getPackageDir()
{
    static const QString constant = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + QLatin1String("packages");
    return QSettings().value("downloadPath", constant).toString();
}

DownloadItem::~DownloadItem()
{
    delete ui;
}

const TitleInfo &DownloadItem::getInfo()
{
    return m_info;
}

void DownloadItem::downloadGameIcon()
{
    QString cache_path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    QFile file(cache_path + QDir::separator() + m_info.contentID + ".jpg");
    if(file.open(QIODevice::ReadOnly))
    {
        //qDebug() << "Loading from cache: " << file.fileName();
        ui->itemPicture->setMinimumWidth(96);
        QImage image = QImage::fromData(file.readAll());
        QPixmap pixmap = QPixmap::fromImage(image);
        ui->itemPicture->setPixmap(pixmap);
        return;
    }

    QString path = imageUrl.arg(m_storeRoot, m_info.contentID);

    //qDebug() << "Downloading thumbnail from " << path;
    QNetworkRequest request(path);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    m_reply = m_manager->get(request);

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(setLastError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), this, SLOT(loadGameIcon()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void DownloadItem::setLastError(QNetworkReply::NetworkError code)
{
    qDebug() << "Error in downloadItem reply: " << code;
    m_error = code;
}

void DownloadItem::loadGameIcon()
{
    QByteArray data;

    if(m_error != QNetworkReply::NoError)
    {
        qDebug() << "Error while downloading " << m_info.contentID << ": " << m_reply->errorString();
    }
    else
    {
        QVariant type = m_reply->header(QNetworkRequest::ContentTypeHeader);
        if(type.isValid())
        {
            if(type.toString().startsWith("application/json"))
            {
                qDebug() << "Image for " << m_info.contentID << "doesn't exists";
                return;
            }
        }
        data = m_reply->readAll();
    }

    QString cache_path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir::root().mkpath(cache_path);

    QFile file(cache_path + QDir::separator() + m_info.contentID + ".jpg");
    if(file.open(QIODevice::WriteOnly))
    {
        //qDebug() << "Saving to cache: " << file.fileName();
        file.write(data);
        file.close();
    }
    ui->itemPicture->setMinimumWidth(96);
    QImage image = QImage::fromData(data);
    QPixmap pixmap = QPixmap::fromImage(image);
    ui->itemPicture->setPixmap(pixmap);
}

void DownloadItem::clipboardCopy()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_info.packageUrl);
}

void DownloadItem::downloadPackage()
{
    if(!m_downloading)
    {
        m_downloading = true;
        ui->downloadButton->setIcon(QIcon(":/main/resources/images/media-playback-pause.svg"));
    }
    else
    {
        m_reply->abort();
        return;
    }

    if(m_pkginfo.exists())
    {
        m_startOffset = m_pkginfo.size();
    }
    else
    {
        QDir::root().mkpath(getPackageDir());
        m_startOffset = 0;
    }

    updateDataTransferProgress(0, m_info.packageSize - m_startOffset);

    m_file = new QFile(m_pkginfo.absoluteFilePath());

    if(!m_file->open(QIODevice::WriteOnly | QIODevice::Append))
    {
        resetDownloadState();
        return;
    }

    qDebug() << "Downloading " << m_info.gameName << ", " << m_startOffset << "-" << m_info.packageSize;

    QNetworkRequest request(m_info.packageUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    QByteArray rangeHeaderValue = "bytes=" + QByteArray::number(m_startOffset) + "-";
    request.setRawHeader("Range", rangeHeaderValue);
    m_reply = m_manager->get(request);

    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(readError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDataTransferProgress(qint64,qint64)));
    connect(m_reply, SIGNAL(readyRead()), this, SLOT(savePkgPart()));
    connect(m_reply, SIGNAL(finished()), this, SLOT(packageComplete()));
    connect(m_reply, SIGNAL(finished()), m_reply, SLOT(deleteLater()));
}

void DownloadItem::updateDataTransferProgress(qint64 readBytes, qint64 totalBytes)
{
    int percentage = (int)(((readBytes + m_startOffset) * 100) / (totalBytes + m_startOffset));
    ui->progressBar->setMaximum(100);
    ui->progressBar->setValue(percentage);
    m_downloaded = readBytes + m_startOffset;
    ui->downloadedLabel->setText(readable_size(m_downloaded, true));
}

void DownloadItem::readError(QNetworkReply::NetworkError code)
{
    if(code == QNetworkReply::OperationCanceledError)
        qDebug() << "Download canceled: " << m_info.gameName;
    else
        qDebug() << "Error while downloading " << m_info.gameName << ": " << code;
}

void DownloadItem::savePkgPart()
{
    QByteArray data = m_reply->readAll();
    m_file->write(data);
}

void DownloadItem::packageComplete()
{
    if(m_reply->isReadable())
    {
        QByteArray data = m_reply->readAll();
        m_file->write(data);
        m_startOffset = m_info.packageSize;        
        qDebug() << "Download complete: " << m_info.gameName;
    }
    else
    {
        qDebug() << "Download interrupted for " << m_info.gameName << ": " << m_downloaded << " bytes";
    }

    m_file->close();
    delete m_file;
    m_file = NULL;
    m_downloading = false;
    ui->downloadButton->setIcon(QIcon(":/main/resources/images/dialog-ok-apply.svg"));
    ui->deleteButton->setEnabled(true);
}

void DownloadItem::resetDownloadState()
{
    m_file->close();
    delete m_file;
    m_file = NULL;
    m_startOffset = 0;
    m_downloading = false;
    ui->downloadButton->setIcon(QIcon(":/main/resources/images/media-playback-start.svg"));
    ui->deleteButton->setEnabled(true);
}

void DownloadItem::deletePackage()
{
    int ret = QMessageBox::warning(this, tr("Confirmation"),
                                    tr("Are you sure to delete this downloaded package?"),
                                    QMessageBox::Ok | QMessageBox::Cancel,
                                    QMessageBox::Cancel);
    if(ret == QMessageBox::Ok)
    {
        if(m_pkginfo.exists())
        {
            if(QFile(m_pkginfo.absoluteFilePath()).remove())
            {
                m_startOffset = 0;
                updateDataTransferProgress(0, m_info.packageSize);
                ui->downloadButton->setIcon(QIcon(":/main/resources/images/media-playback-start.svg"));
            }
            else
                QMessageBox::warning(this, tr("Warning"), tr("Cannot delete the package file"), QMessageBox::Ok);
        }
        else
        {
            m_startOffset = 0;
            updateDataTransferProgress(0, m_info.packageSize);
            ui->downloadButton->setIcon(QIcon(":/main/resources/images/media-playback-start.svg"));
        }
    }
}

int DownloadItem::status()
{
   if(!m_downloading)
       if(m_downloaded == 0)
           return 1; // new
        else if(m_downloaded == m_info.packageSize)
           return 3; // complete
        else
           return 4; // paused
   else
       return 2; //downloading
}

void DownloadItem::setWaitingIcon(bool )
{
    //if(set)
    //    ui->requestDownloadButton->setIcon(QIcon(":/main/resources/images/appointment-new.svg"));
    //else
    //    ui->requestDownloadButton->setIcon(QIcon(":/main/resources/images/folder-download.svg"));
}
