// src/systemstatus.h
#ifndef SYSTEMSTATUS_H
#define SYSTEMSTATUS_H

#include <QObject>
#include <QString>
#include <QTime>
#include <QtQml/qqmlregistration.h>

class SystemStatus : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int battery READ battery NOTIFY changed)
    Q_PROPERTY(QString time READ time NOTIFY changed)
    public:
        explicit SystemStatus(QObject *parent = nullptr);
        int battery() const { return m_battery; }
        QString time() const { return QTime::currentTime().toString("HH:mm"); }
    signals:
        void changed();
    private:
        int m_battery = 87;
};

#endif // SYSTEMSTATUS_H