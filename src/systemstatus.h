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
    Q_PROPERTY(bool online READ online NOTIFY onlineChanged)
    public:
        explicit SystemStatus(QObject *parent = nullptr);
        int battery() const { return m_battery; }
        QString time() const { return QTime::currentTime().toString("HH:mm"); }
        bool online() const { return m_online; }
    signals:
        void changed();
        void onlineChanged();
    private:
        int m_battery = 87;
        bool m_online = false;
};

#endif // SYSTEMSTATUS_H