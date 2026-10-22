#include "systemstatus.h"
#include <QTimer>

SystemStatus::SystemStatus(QObject *parent) : QObject(parent) {
    auto *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &SystemStatus::changed);
    t->start(1000);
}