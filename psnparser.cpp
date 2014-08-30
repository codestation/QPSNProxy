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

#include "psnparser.h"
#include <QDebug>

QList<TitleInfo> parsePsnJson(const QVariantMap &json)
{
    QList<TitleInfo> item_list;

    bool ok = false;
    int total_results = json["total_results"].toInt(&ok);
    if(!ok) {
        return item_list;
    }

    qDebug() << "Total entitlements: " << total_results;
    QVariantList entitlements = json["entitlements"].toList();

    for(QVariantList::const_iterator it = entitlements.begin(); it != entitlements.end(); ++it)
    {
        QVariantMap entitlement = it->toMap();

        QString contentId;
        QString game_name;
        qlonglong package_size;
        QString package_url;
        ConsoleType console;
        int plus;

        if(entitlement["entitlement_type"].toInt() == 2) // PSP/PS3/VITA
        {
            contentId = entitlement["product_id"].toString();

            if(!entitlement.contains("drm_def"))
                continue;

            QMap<QString,QVariant> drm_def = entitlement["drm_def"].toMap();
            QVariantList drmContents = drm_def["drmContents"].toList();
            QMap<QString,QVariant> drmContent = drmContents[0].toMap();

            if(drm_def.contains("contentName"))
                game_name = drm_def["contentName"].toString();
            else
                game_name = drmContent["titleName"].toString();

            package_size = drmContent["contentSize"].toLongLong();
            package_url = drmContent["contentUrl"].toString();
            qlonglong platformIds = drmContent["platformIds"].toLongLong();
            plus = drmContent["gracePeriod"].toInt() > 0;

            switch(platformIds)
            {
            case 0x80000000: // ps3 game
            case 0x80800000: // ps3 addon
                console = PS3;
                break;
            case 0x08000000: // psm assistant and soul sacrifice demo
            case 0x88000000: // ps vita game
            case 0xFE100000:
                console = PSVITA;
                break;
            case 0xF8100000: // psp game
            case 0xF0100000: // demo
                console = PSP;
                break;
            default:
                console = UNKNOWN;
                break;
            }
        }
        else if(entitlement["entitlement_type"].toInt() == 5) // PS4
        {
            QMap<QString,QVariant> game_meta = entitlement["game_meta"].toMap();
            contentId = entitlement["product_id"].toString();
            game_name = game_meta["name"].toString();
            plus = entitlement.contains("inactive_date");
            QVariantList entitlement_attributes = entitlement["entitlement_attributes"].toList();
            QMap<QString,QVariant> entitlement_attribute = entitlement_attributes[0].toMap();
            package_size = entitlement_attribute["package_file_size"].toLongLong();
            package_url = entitlement_attribute["reference_package_url"].toString();
            console = PS4;
        }
        else // 1 or 3
        {
            continue;
        }

        TitleInfo item(contentId, game_name, package_size, package_url, console, plus);
        item_list << item;
    }

    qSort(item_list.begin(), item_list.end(), TitleInfo::lessThan);

    return item_list;
}

QList<Notification> parseNotificationJson(const QVariantList &json)
{
    QList<Notification> item_list;

    for(QVariantList::const_iterator it = json.begin(); it != json.end(); ++it)
    {
        QVariantMap notification = it->toMap();
        Notification notif;
        notif.contentID = notification["contentId"].toString();
        notif.status = notification["status"].toString();
        item_list.append(notif);
    }

    return item_list;
}
