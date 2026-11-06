#ifndef ROUTESTORE_H
#define ROUTESTORE_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class RouteStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList routes READ routes NOTIFY routesChanged)
public:
    explicit RouteStore(QObject *parent = nullptr) : QObject(parent) {}
    QVariantList routes() const { return m_items; }

    Q_INVOKABLE int add() {
        int id = m_nextId++;
        QVariantMap r;
        r["id"] = id;
        r["name"] = "ROUTE " + QString::number(id);
        r["waypointIds"] = QVariantList();
        m_items.append(r);
        emit routesChanged();
        return id;
    }

    Q_INVOKABLE void rename(int id, const QString &name) {
        int i = indexOf(id);
        if (i < 0) return;
        QVariantMap r = m_items[i].toMap();
        r["name"] = name;
        m_items[i] = r;
        emit routesChanged();
    }

    Q_INVOKABLE void remove(int id) {
        int i = indexOf(id);
        if (i < 0) return;
        m_items.removeAt(i);
        emit routesChanged();
    }

    Q_INVOKABLE void setCheckpoints(int id, QVariantList waypointIds) {
        int i = indexOf(id);
        if (i < 0) return;
        QVariantMap r = m_items[i].toMap();
        r["waypointIds"] = waypointIds;
        m_items[i] = r;
        emit routesChanged();
    }

    Q_INVOKABLE void removeWaypoint(int waypointId) {
        bool changed = false;
        for (int i = 0; i < m_items.size(); i++) {
            QVariantMap r = m_items[i].toMap();
            QVariantList ids = r.value("waypointIds").toList();
            const qsizetype before = ids.size();
            ids.removeIf([&](const QVariant &v) { return v.toInt() == waypointId; });
            if (ids.size() == before) continue;
            r["waypointIds"] = ids;
            m_items[i] = r;
            changed = true;
        }
        if (changed) emit routesChanged();
    }

    Q_INVOKABLE QVariantList routesUsing(int waypointId) const {
        QVariantList result;

        for (const QVariant &item : m_items) {
            QVariantMap r = item.toMap();
            const QVariantList ids = r.value("waypointIds").toList();

            for (const QVariant &v : ids) {
                if (v.toInt() == waypointId) {
                    QVariantMap entry;
                    entry["id"] = r.value("id");
                    entry["name"] = r.value("name");
                    result.append(entry);
                    break;
                }
            }
        }

        return result;
    }

signals:
    void routesChanged();

private:
    QVariantList m_items;
    int m_nextId = 1;
    int indexOf(int id) const {
        for (int i = 0; i < m_items.size(); i++)
            if (m_items[i].toMap().value("id").toInt() == id)
                return i;
        return -1;
    }
};

#endif