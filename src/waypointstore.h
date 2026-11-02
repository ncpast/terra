#ifndef WAYPOINTSTORE_H
#define WAYPOINTSTORE_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class WaypointStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList waypoints READ waypoints NOTIFY waypointsChanged)
public:
    explicit WaypointStore(QObject *parent = nullptr) : QObject(parent) {}
    QVariantList waypoints() const { return m_items; }

    Q_INVOKABLE int add(double lat, double lon) {
        QVariantMap wp;
        int id      = m_nextId;
        wp["id"]    = id;
        wp["name"]  = "WP" + QString::number(id);
        wp["lat"]   = lat;
        wp["lon"]   = lon;
        wp["shown"] = true;
        m_items.append(wp);
        m_nextId++;
        emit waypointsChanged();
        return id;
    }

    Q_INVOKABLE void setShown(int row, bool shown) {
        if (row < 0 || row >= m_items.size()) return;
        QVariantMap wp = m_items[row].toMap();   // copy the row out of the list
        wp["shown"] = shown;                     // change the copy
        m_items[row] = wp;                       // put it back
        emit waypointsChanged();
    }

    Q_INVOKABLE void update(int id, const QString &name, double lat, double lon, bool shown) {
        for (int i = 0; i < m_items.size(); i++) {
            QVariantMap wp = m_items[i].toMap();
            if (wp["id"].toInt() != id) continue;
            wp["name"] = name;
            wp["lat"] = lat;
            wp["lon"] = lon;
            wp["shown"] = shown;
            m_items[i] = wp;
            emit waypointsChanged();
            return;
        }
    }

    Q_INVOKABLE void remove(int id) {
        for (int i = 0; i < m_items.size(); i++) {
            if (m_items[i].toMap()["id"].toInt() == id) {
                m_items.removeAt(i);
                emit waypointsChanged();
                return;
            }
        }
    }

signals:
    void waypointsChanged();

private:
    QVariantList m_items;
    int m_nextId = 1;
};

#endif