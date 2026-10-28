#ifndef WAYPOINTSTORE_H
#define WAYPOINTSTORE_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class WaypointStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList waypoints READ waypoints NOTIFY waypointsChanged)
public:
    explicit WaypointStore(QObject *parent = nullptr) : QObject(parent) {}
    QVariantList waypoints() const { return m_items; }

    Q_INVOKABLE void add(double lat, double lon) {
        QVariantMap wp;
        wp["name"]  = "WP" + QString::number(m_nextId);
        wp["lat"]   = lat;
        wp["lon"]   = lon;
        wp["shown"] = true;
        m_items.append(wp);
        m_nextId++;
        emit waypointsChanged();
    }

    Q_INVOKABLE void setShown(int row, bool shown) {
        if (row < 0 || row >= m_items.size()) return;
        QVariantMap wp = m_items[row].toMap();   // copy the row out of the list
        wp["shown"] = shown;                     // change the copy
        m_items[row] = wp;                       // put it back
        emit waypointsChanged();
    }

signals:
    void waypointsChanged();

private:
    QVariantList m_items;
    int m_nextId = 1;
};

#endif