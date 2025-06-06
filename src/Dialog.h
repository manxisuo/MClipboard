#pragma once

#include <QDialog>
#include <QSystemTrayIcon>
#include <QClipboard>
#include <QDateTime>

#include "DBOperate.h"

namespace Ui {
class Dialog;
}

class DBOperate;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupClipboardTable();
    void setupFavoriteTable();
    void addToClipboardHistory(const QString &text, const QString &dt);
    void addToFavorite(const QString &text);
    void removeFromClipboardHistory(const QString &content);
    void removeFromFavorite(const QString &content);

private slots:
    void showMe();
    void slotIdPressed(int id);
    void onClipboardDataChanged();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void addFavorite();
    void clearHistory();
    void clearFavorite();

private:
    Ui::Dialog *ui;
    QSystemTrayIcon *m_TrayIcon;
    QClipboard *m_Clipboard;
    DBOperate m_DB;

    QPoint m_dragPosition;
    bool m_dragging = false;
};
