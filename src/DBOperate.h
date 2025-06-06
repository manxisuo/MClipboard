#pragma once
#include <QString>
#include <QList>
#include <QPair>
#include <QMouseEvent>

class DBOperate
{
public:
    DBOperate();
    ~DBOperate();

    void init();
    void addHistory(const QString &content, const QString &time);
    void removeHistory(const QString &content);
    void clearHistory();
    QList<QPair<QString, QString>> getHistory(int limit = 200);

    void addFavorite(const QString &content);
    void removeFavorite(const QString &content);
    void clearFavorite();
    QList<QString> getFavorite();
}; 
