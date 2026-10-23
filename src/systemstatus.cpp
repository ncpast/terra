#include "systemstatus.h"
#include <QNetworkInformation>
#include <QTimer>

SystemStatus::SystemStatus(QObject *parent) : QObject(parent) {
    auto *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &SystemStatus::changed);
    t->start(1000);

    if (QNetworkInformation::loadBackendByFeatures(
            QNetworkInformation::Feature::Reachability)) {
        auto *ni = QNetworkInformation::instance();
        auto update = [this, ni] {
            const bool on = ni->reachability()
            == QNetworkInformation::Reachability::Online;
            if (on != m_online) { m_online = on; emit onlineChanged(); }
        };
        connect(ni, &QNetworkInformation::reachabilityChanged, this, update);
        update();
    }
}