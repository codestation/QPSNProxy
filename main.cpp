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

#include <QApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("codestation");
    app.setApplicationName("QPSNProxy");

    QTranslator translator;
    QString locale = QLocale().system().name();
    qDebug() << "Current locale:" << locale;

    if(app.arguments().contains("--set-locale")) {
        int index = app.arguments().indexOf("--set-locale");
        if(index + 1 < app.arguments().length()) {
            qDebug("Enforcing locale: %s", app.arguments().at(index + 1).toUtf8().data());
            locale = app.arguments().at(index + 1);
        }
    }

    if(translator.load("qpsnproxy_" + locale, ":/resources/translations")) {
        app.installTranslator(&translator);
    } else {
        qDebug() << "Cannot load translation for locale:" << locale;
    }

    QTranslator system_translator;
    system_translator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&system_translator);

    MainWindow w;
    w.show();

    return app.exec();
}
