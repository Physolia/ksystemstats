/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include <systemstats/SensorInfo.h>

namespace KSysGuard
{
    class SensorProperty;
}
class KSysGuardDaemon;

/**
 * This class represents an individual connection to the daemon
 */
class Client : public QObject
{
    Q_OBJECT
public:
    Client(KSysGuardDaemon *parent, const QString &serviceName);
    ~Client() override;
    void subscribeSensors(const QStringList &sensorIds);
    void unsubscribeSensors(const QStringList &sensorIds);
    void sendFrame();

private:
    void sendValues(const KSysGuard::SensorDataList &updates);
    void sendMetaDataChanged(const KSysGuard::SensorInfoMap &sensors);

    const QString m_serviceName;
    KSysGuardDaemon *m_daemon;
    QHash<QString, KSysGuard::SensorProperty *> m_subscribedSensors;
    QMultiHash<KSysGuard::SensorProperty *, QMetaObject::Connection> m_connections;
    KSysGuard::SensorDataList m_pendingUpdates;
    KSysGuard::SensorInfoMap m_pendingMetaDataChanges;
};
