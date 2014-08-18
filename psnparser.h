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

#ifndef PSNPARSER_H
#define PSNPARSER_H

#include <QList>
#include <QMap>
#include <QVariant>

enum ConsoleType {ALL, PSVITA, PSP, PS3, PS4, UNKNOWN};
enum ContentType {GAME_DLC, PLUS, OTHER};

class TitleInfo
{
public:
    TitleInfo(
            const QString &id,
            const QString &name,
            qlonglong size,
            const QString &url,
            ConsoleType type,
            bool plus) :
        contentID(id), gameName(name), packageSize(size), packageUrl(url), consoleType(type), onPlus(plus)
        {}

    static bool lessThan(const TitleInfo &s1, const TitleInfo &s2)
    {
        return s1.gameName.compare(s2.gameName) < 0;
    }

    QString contentID;
    QString gameName;
    qlonglong packageSize;
    QString packageUrl;
    ConsoleType consoleType;
    bool onPlus;
};

QList<TitleInfo> parsePsnJson(const QMap<QString,QVariant> &json);

#endif // PSNPARSER_H
