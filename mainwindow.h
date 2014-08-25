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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include "psnrequest.h"
#include "proxyserver.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void refreshList();
    void requestList();
    void updateStatus(QString message);
    void getLoginStatus(int status_code, QString message);
    void setFailedStatus();
    void saveStoreRoot(QString storeRoot);
    void getDownloadList(QByteArray data);
    void onTextChanged(const QString &filter);
    void onCheckChanged(bool checked);
    void onComboChanged(int selected);
    void onStatusChanged(int status);

    void openPackageDir();
    void deleteDownloadList();
    void deletePackages();
    void deleteThumbnailCache();
    void deleteCookies();
    void showAboutQt();
    void showAboutDialog();
    void openOptions();
    void updateDownloadStatus();
    void processStatusList(QVariantList status_list);

private:
    void updateLoginStatus();
    void saveEntitlements(const QByteArray &data);
    QByteArray loadEntitlements();
    void loadGameList(const QByteArray &data);
    void checkListElement(const QString &filter, int console, bool plus, int status);

    Ui::MainWindow *ui;
    PSNRequest m_psn;
    QNetworkAccessManager m_manager;
    ProxyServer *m_proxy;
};

#endif // MAINWINDOW_H
