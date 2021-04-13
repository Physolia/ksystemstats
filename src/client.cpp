/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "client.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QTimer>

#include <algorithm>

#include <systemstats/SensorPlugin.h>
#include <systemstats/SensorObject.h>
#include <systemstats/SensorProperty.h>

#include "ksysguarddaemon.h"

Client::Client(KSysGuardDaemon *parent, const QString &serviceName)
    : QObject(parent)
    , m_serviceName(serviceName)
    , m_daemon(parent)
{
    connect(m_daemon, &KSysGuardDaemon::sensorRemoved, this, [this](const QString &sensor) {
        m_subscribedSensors.remove(sensor);
    });
}

Client::~Client()
{
    for (auto sensor : qAsConst(m_subscribedSensors)) {
        sensor->unsubscribe();
    }
}

void Client::subscribeSensors(const QStringList &sensorPaths)
{
    KSysGuard::SensorDataList entries;

    for (const QString &sensorPath : sensorPaths) {
        if (auto sensor = m_daemon->findSensor(sensorPath)) {
            m_connections.insert(sensor, connect(sensor, &KSysGuard::SensorProperty::valueChanged, this, [this, sensor]() {
                const QVariant value = sensor->value();
                if (!value.isValid()) {
                    return;
                }
                m_pendingUpdates << KSysGuard::SensorData(sensor->path(), value);
            }));
            m_connections.insert(sensor, connect(sensor, &KSysGuard::SensorProperty::sensorInfoChanged, this, [this, sensor]() {
                m_pendingMetaDataChanges[sensor->path()] = sensor->info();
            }));

            sensor->subscribe();

            m_subscribedSensors.insert(sensorPath, sensor);
        }
    }
}

void Client::unsubscribeSensors(const QStringList &sensorPaths)
{
    for (const QString &sensorPath : sensorPaths) {
        if (auto sensor = m_subscribedSensors.take(sensorPath)) {
            disconnect(m_connections.take(sensor));
            disconnect(m_connections.take(sensor));
            sensor->unsubscribe();
        }
    }
}

void Client::sendFrame()
{
    sendMetaDataChanged(m_pendingMetaDataChanges);
    sendValues(m_pendingUpdates);
    m_pendingUpdates.clear();
    m_pendingMetaDataChanges.clear();
}

void Client::sendValues(const KSysGuard::SensorDataList &entries)
{
    if (entries.isEmpty()) {
        return;
    }
    auto msg = QDBusMessage::createTargetedSignal(m_serviceName, "/", "org.kde.KSysGuardDaemon", "newSensorData");
    msg.setArguments({QVariant::fromValue(entries)});
    QDBusConnection::sessionBus().send(msg);
}

void Client::sendMetaDataChanged(const KSysGuard::SensorInfoMap &sensors)
{
    if (sensors.isEmpty()) {
        return;
    }
    auto msg = QDBusMessage::createTargetedSignal(m_serviceName, "/", "org.kde.KSysGuardDaemon", "sensorMetaDataChanged");
    msg.setArguments({QVariant::fromValue(sensors)});
    QDBusConnection::sessionBus().send(msg);
}
