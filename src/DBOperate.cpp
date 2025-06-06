#include "DBOperate.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QCoreApplication>

DBOperate::DBOperate() {}
DBOperate::~DBOperate() {}

void DBOperate::init()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QCoreApplication::applicationDirPath() + "/MClipboard.db";
    db.setDatabaseName(dbPath);
    
    if (!db.open())
    {
        qDebug() << "数据库打开失败:" << db.lastError().text();
    }
    
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS history ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "time TEXT,"
               "content TEXT UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS favorite ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "content TEXT UNIQUE)");
}

void DBOperate::addHistory(const QString &content, const QString &time)
{
    // 先查找是否已存在，存在则删除
    QSqlQuery checkQ;
    checkQ.prepare("SELECT COUNT(*) FROM history WHERE content=?");
    checkQ.addBindValue(content);
    checkQ.exec();
    if (checkQ.next() && checkQ.value(0).toInt() > 0) {
        QSqlQuery delQ;
        delQ.prepare("DELETE FROM history WHERE content=?");
        delQ.addBindValue(content);
        delQ.exec();
    }

    // 再插入
    QSqlQuery q;
    q.prepare("INSERT INTO history (time, content) VALUES (?, ?)");
    q.addBindValue(time);
    q.addBindValue(content);
    q.exec();
}

void DBOperate::removeHistory(const QString &content)
{
    QSqlQuery q;
    q.prepare("DELETE FROM history WHERE content=?");
    q.addBindValue(content);
    q.exec();
}

void DBOperate::clearHistory()
{
    QSqlQuery("DELETE FROM history").exec();
}

QList<QPair<QString, QString>> DBOperate::getHistory(int limit)
{
    QList<QPair<QString, QString>> list;
    QSqlQuery q;
    q.prepare("SELECT time, content FROM history ORDER BY id LIMIT ?");
    q.addBindValue(limit);
    q.exec();
    while (q.next())
    {
        list.append(qMakePair(q.value(0).toString(), q.value(1).toString()));
    }
    return list;
}

void DBOperate::addFavorite(const QString &content)
{
    // 先查找是否已存在，存在则删除
    QSqlQuery checkQ;
    checkQ.prepare("SELECT COUNT(*) FROM favorite WHERE content=?");
    checkQ.addBindValue(content);
    checkQ.exec();
    if (checkQ.next() && checkQ.value(0).toInt() > 0) {
        QSqlQuery delQ;
        delQ.prepare("DELETE FROM favorite WHERE content=?");
        delQ.addBindValue(content);
        delQ.exec();
    }

    // 再插入
    QSqlQuery q;
    q.prepare("INSERT INTO favorite (content) VALUES (?)");
    q.addBindValue(content);
    q.exec();
}

void DBOperate::removeFavorite(const QString &content)
{
    QSqlQuery q;
    q.prepare("DELETE FROM favorite WHERE content=?");
    q.addBindValue(content);
    q.exec();
}

void DBOperate::clearFavorite()
{
    QSqlQuery("DELETE FROM favorite").exec();
}

QList<QString> DBOperate::getFavorite()
{
    QList<QString> list;
    QSqlQuery q("SELECT content FROM favorite ORDER BY id");
    while (q.next())
    {
        list.append(q.value(0).toString());
    }
    return list;
} 
