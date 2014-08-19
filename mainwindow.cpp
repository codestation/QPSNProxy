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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "downloaditem.h"
#include "authdialog.h"
#include "psnparser.h"
#include "utils.h"
#include "json.h"

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QStandardPaths>
#include <QThreadPool>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSettings settings;
    bool ok = false;

    int console = settings.value("selectedConsole", 0).toInt(&ok);
    ui->consoleComboBox->setCurrentIndex(ok ? console : 0);
    ui->plusCheckBox->setChecked(settings.value("onlyPlus", false).toBool());

    ui->downloadTableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->downloadTableWidget->horizontalHeader()->hide();

    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::refreshList);
    connect(ui->loginButton, &QPushButton::clicked, this, &MainWindow::requestList);
    connect(&m_psn, &PSNRequest::storeRootUrlReceived, this, &MainWindow::saveStoreRoot);
    connect(&m_psn, &PSNRequest::loginSucceeded, this, &MainWindow::updateLoginStatus);
    connect(&m_psn, &PSNRequest::loginFailed, this, &MainWindow::setFailedStatus);
    connect(&m_psn, &PSNRequest::loginStatusReceived, this, &MainWindow::getLoginStatus);
    connect(&m_psn, &PSNRequest::downloadListReceived, this, &MainWindow::getDownloadList);
    connect(&m_psn, &PSNRequest::networkErrorReceived, this, &MainWindow::updateStatus);

    connect(ui->consoleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboChanged(int)));
    connect(ui->plusCheckBox, SIGNAL(clicked(bool)), this, SLOT(onCheckChanged(bool)));
    connect(ui->downloadFilter, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(ui->statusBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onStatusChanged(int)));

    connect(ui->actionOpen_package_directory, SIGNAL(triggered()), this, SLOT(openPackageDir()));
    connect(ui->actionClear_downloaded_packages, SIGNAL(triggered()), this, SLOT(deletePackages()));
    connect(ui->actionClear_download_list_cache, SIGNAL(triggered()), this, SLOT(deleteDownloadList()));
    connect(ui->actionClear_PSN_login_cookies, SIGNAL(triggered()), this, SLOT(deleteCookies()));
    connect(ui->actionClear_thumbnail_cache, SIGNAL(triggered()), this, SLOT(deleteThumbnailCache()));

    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), this, SLOT(showAboutQt()));

    QThreadPool::globalInstance()->setMaxThreadCount(4);

    QByteArray data = loadEntitlements();
    if(!data.isEmpty())
        loadGameList(data);

    m_proxy = new ProxyServer;
    qint16 port = QSettings().value("proxyPort", 8888).toInt();
    if(!m_proxy->listen(QHostAddress::Any, port))
    {
        QMessageBox::warning(this, "QPSNProxy", tr("Cannot bind to port %1").arg(port), QMessageBox::Ok);
    }
}

void MainWindow::getLoginStatus(int status_code, QString message)
{
    if(status_code == PSNRequest::SUCCESS)
    {
        updateStatus(tr("Downloading list using previous session..."));
        m_psn.requestDownloadList();
    }
    else if(status_code == PSNRequest::NEED_REFRESH)
    {
        updateStatus(tr("Recovering previous session..."));
        m_psn.loginRefresh();
    }
    else if(status_code == PSNRequest::AUTH_REQUIRED)
    {
        updateStatus(tr("Login status: %1, retrying login...").arg(message));
        requestList();
    }
    else
        updateStatus(tr("Unknown error code %1: %2").arg(QString::number(status_code), message));
}

void MainWindow::requestList()
{
    AuthDialog dialog;
    if(dialog.exec())
    {
        QPair<QString, QString> auth = dialog.getAuth();
        updateStatus(tr("Logging in..."));
        m_psn.login(auth.first, auth.second);
    }
    else
    {
        updateStatus(tr("Login cancelled"));
    }
}

void MainWindow::setFailedStatus()
{
    updateStatus(tr("Login failed"));
}

void MainWindow::updateLoginStatus()
{
    updateStatus(tr("Login success, downloading list"));
    m_psn.requestStoreRootUrl();
}

void MainWindow::saveStoreRoot(QString storeRoot)
{
    QSettings settings;
    settings.setValue("storeRoot", storeRoot);
    m_psn.requestDownloadList();
}

void MainWindow::refreshList()
{
    m_psn.checkLogin();
}

void MainWindow::saveEntitlements(const QByteArray &data)
{
    QString data_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir(QDir::root()).mkpath(data_path);
    QFile json(data_path + QDir::separator() + "listing.json");
    if(json.open(QIODevice::WriteOnly))
    {
        json.write(data);
        json.close();
    }
}

QByteArray MainWindow::loadEntitlements()
{
    QByteArray data;
    QString data_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QFile json(data_path + QDir::separator() + "listing.json");
    if(json.open(QIODevice::ReadOnly))
    {
        data = json.readAll();
        json.close();
    }

    return data;
}

void MainWindow::getDownloadList(QByteArray data)
{
    updateStatus(tr("Download complete, %1 bytes received").arg(data.size()));
    saveEntitlements(data);
    loadGameList(data);
}

void MainWindow::loadGameList(const QByteArray &data)
{
    QMap<QString,QVariant> json = json_decode(data);
    QList<TitleInfo> title_list = parsePsnJson(json);

    QHeaderView *vert_header = ui->downloadTableWidget->verticalHeader();
    QHeaderView *horiz_header = ui->downloadTableWidget->horizontalHeader();
    horiz_header->setSectionResizeMode(QHeaderView::Stretch);
    vert_header->setUpdatesEnabled(false);

    QSettings settings;
    settings.setValue("selectedConsole", ui->consoleComboBox->currentIndex());
    settings.setValue("onlyPlus", ui->plusCheckBox->isChecked());
    QString storeRoot = settings.value("storeRoot").toString();

    ui->downloadTableWidget->setRowCount(title_list.size());
    int row = 0;

    foreach(const TitleInfo &title, title_list)
    {
        DownloadItem *item = new DownloadItem(title, storeRoot, this);
        item->m_manager = &m_manager;
        item->downloadGameIcon();
        ui->downloadTableWidget->setCellWidget(row, 0, item);
        vert_header->resizeSection(row, 114);
        ++row;
    }

    vert_header->setUpdatesEnabled(true);

    checkListElement(ui->downloadFilter->text(),
                     ui->consoleComboBox->currentIndex(),
                     ui->plusCheckBox->isChecked(),
                     0);
}

void MainWindow::onTextChanged(const QString &filter)
{
    checkListElement(filter,
                     ui->consoleComboBox->currentIndex(),
                     ui->plusCheckBox->isChecked(),
                     ui->statusBox->currentIndex()
                     );
}

void MainWindow::onCheckChanged(bool checked)
{
    QSettings().setValue("onlyPlus", checked);
    checkListElement(ui->downloadFilter->text(),
                     ui->consoleComboBox->currentIndex(),
                     checked,
                     ui->statusBox->currentIndex()
                     );
}

void MainWindow::onComboChanged(int selected)
{
    QSettings().setValue("selectedConsole", selected);
    checkListElement(ui->downloadFilter->text(),
                     selected,
                     ui->plusCheckBox->isChecked(),
                     ui->statusBox->currentIndex());
}

void MainWindow::onStatusChanged(int status)
{
    checkListElement(ui->downloadFilter->text(),
                     ui->consoleComboBox->currentIndex(),
                     ui->plusCheckBox->isChecked(),
                     status
                     );
}

void MainWindow::checkListElement(const QString &filter, int console, bool plus, int status)
{
    QStringList labels;
    int label_count = 0;
    int row_count = ui->downloadTableWidget->rowCount();

    for(int i = 0; i < row_count; ++i) {
        DownloadItem *item = (DownloadItem*) ui->downloadTableWidget->cellWidget(i, 0);
        const TitleInfo &info = item->getInfo();

        if((filter != tr("Filter") && !filter.isEmpty() && !info.gameName.contains(filter, Qt::CaseInsensitive)) ||
                (console != ALL && info.consoleType != console) ||
                (plus && info.onPlus != plus) ||
                (status != 0 && status != item->status()))
        {
            ui->downloadTableWidget->setRowHidden(i, true);
            labels << "0";
        }
        else
        {
            ui->downloadTableWidget->setRowHidden(i, false);
            labels << QString::number(++label_count);
        }
    }
    if(label_count > 0)
        ui->downloadTableWidget->setVerticalHeaderLabels(labels);

    ui->downloadTableWidget->verticalHeader()->setMinimumWidth(30);
    ui->downloadTableWidget->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    updateStatus(tr("Showing %n item(s)", 0, label_count));
}

void MainWindow::updateStatus(QString message)
{
    ui->statusBar->showMessage(message, 0);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_proxy;
}

void MainWindow::deleteCookies()
{
    QFile::remove(DownloadItem::getPackageDir() + QDir::separator() + QLatin1String("cookies.ini"));
    updateStatus(tr("Cookies deleted"));
}

void MainWindow::deleteThumbnailCache()
{
    QString cache_path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cache_path);
    dir.setNameFilters(QStringList() << "*.jpg");
    dir.setFilter(QDir::Files);
    foreach(QString dirFile, dir.entryList())
        dir.remove(dirFile);
    updateStatus(tr("Thumbnail cache deleted"));
}

void MainWindow::deletePackages()
{
    QDir dir(DownloadItem::getPackageDir());
    dir.setNameFilters(QStringList() << "*.pkg");
    dir.setFilter(QDir::Files);
    foreach(QString dirFile, dir.entryList())
        dir.remove(dirFile);
    updateStatus(tr("Packages deleted"));
}

void MainWindow::deleteDownloadList()
{
    QFile::remove(DownloadItem::getPackageDir() + QDir::separator() + QLatin1String("listing.json"));
    updateStatus(tr("Download list cache deleted"));
}

void MainWindow::openPackageDir()
{
    QDir::root().mkpath(DownloadItem::getPackageDir());
    QDesktopServices::openUrl(QUrl::fromLocalFile(DownloadItem::getPackageDir()));
}


void MainWindow::showAboutQt()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::showAboutDialog()
{
    QMessageBox about;

    about.setText(QString("QPSNProxy ") + QPSNPROXY_VER);
    about.setWindowTitle(tr("About QPSNProxy"));
#ifndef QPSNPROXY_BUILD_HASH
    about.setInformativeText(tr("Copyright (C) 2014  Codestation") + "\n");
#else
    about.setInformativeText(tr("Copyright (C) 2014  Codestation\n\nbuild hash: %1\nbuild branch: %2").arg(QPSNPROXY_BUILD_HASH).arg(QPSNPROXY_BUILD_BRANCH));
#endif
    about.setStandardButtons(QMessageBox::Ok);
    about.setIconPixmap(QPixmap(":/main/resources/images/qpsnproxy.png"));
    about.setDefaultButton(QMessageBox::Ok);

    // hack to expand the messagebox minimum size
    QSpacerItem* horizontalSpacer = new QSpacerItem(300, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)about.layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());

    about.show();
    about.exec();
}
