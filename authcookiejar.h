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

#ifndef AUTHCOOKIEJAR_H
#define AUTHCOOKIEJAR_H

#include <QList>
#include <QNetworkCookie>
#include <QNetworkCookieJar>

class AuthCookieJar : public QNetworkCookieJar
{
public:    
    QList<QNetworkCookie> getAllCookies();
    void save();
    void load();
    void clear();

private:
    void purgeOldCookies();
};

#endif // AUTHCOOKIEJAR_H
