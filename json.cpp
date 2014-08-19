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

#include "json.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
#include <QJsonDocument>

QVariantMap json_decode(const QString &jsonStr)
{
    QJsonDocument d = QJsonDocument::fromJson(qPrintable(jsonStr));
    return d.toVariant().toMap();
}

QByteArray json_encode(const QVariantList &json)
{
    return QJsonDocument::fromVariant(json).toJson();
}

QByteArray json_encode(const QVariantMap &json)
{
    return QJsonDocument::fromVariant(json).toJson();
}

#else
#include <QScriptEngine>
#include <QScriptValueIterator>

static QVariantMap decodeInner(QScriptValue object);

static QVariantList decodeInnerToList(QScriptValue arrayValue)
{
    QVariantList list;
    QScriptValueIterator it(arrayValue);
    while (it.hasNext()) {
        it.next();
        if (it.name() == "length")
            continue;

        if (it.value().isArray())
            list.append(QVariant(decodeInnerToList(it.value())));
        else if (it.value().isNumber())
            list.append(QVariant(it.value().toNumber()));
        else if (it.value().isString())
            list.append(QVariant(it.value().toString()));
        else if (it.value().isNull())
            list.append(QVariant());
        else if(it.value().isObject())
            list.append(QVariant(decodeInner(it.value())));
    }
    return list;
}

static QVariantMap decodeInner(QScriptValue object)
{
    QVariantMap map;
    QScriptValueIterator it(object);
    while (it.hasNext()) {
        it.next();
        if (it.value().isArray())
            map.insert(it.name(),QVariant(decodeInnerToList(it.value())));
        else if (it.value().isNumber())
            map.insert(it.name(),QVariant(it.value().toNumber()));
        else if (it.value().isString())
            map.insert(it.name(),QVariant(it.value().toString()));
        else if (it.value().isNull())
            map.insert(it.name(),QVariant());
        else if(it.value().isObject())
            map.insert(it.name(),QVariant(decodeInner(it.value())));
    }
    return map;
}

static QScriptValue encodeInner(const QVariantMap &map, QScriptEngine* engine)
{
    QScriptValue obj = engine->newObject();
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        if (i.value().type() == QVariant::String)
            obj.setProperty(i.key(), i.value().toString());
        else if (i.value().type() == QVariant::Int)
            obj.setProperty(i.key(), i.value().toInt());
        else if (i.value().type() == QVariant::Double)
            obj.setProperty(i.key(), i.value().toDouble());
        else if (i.value().type() == QVariant::List)
            obj.setProperty(i.key(), qScriptValueFromSequence(engine, i.value().toList()));
        else if (i.value().type() == QVariant::Map)
            obj.setProperty(i.key(), encodeInner(i.value().toMap(), engine));
    }
    return obj;
}

QVariantMap json_decode(const QString &jsonStr)
{
    QScriptValue object;
    QScriptEngine engine;
    object = engine.evaluate("(" + jsonStr + ")");
    return decodeInner(object);
}

QByteArray json_encode(const QVariantMap &json)
{
    QScriptEngine engine;
    engine.evaluate("function toString() { return JSON.stringify(this) }");

    QScriptValue toString = engine.globalObject().property("toString");
    QScriptValue obj = encodeInner(json, &engine);
    return qPrintable(toString.call(obj).toString());
}

QByteArray json_encode(const QVariantList &json)
{
    QScriptEngine engine;
    engine.evaluate("function toString() { return JSON.stringify(this) }");

    QScriptValue toString = engine.globalObject().property("toString");
    QScriptValue obj = qScriptValueFromSequence(&engine, json);
    return qPrintable(toString.call(obj).toString());
}

#endif
