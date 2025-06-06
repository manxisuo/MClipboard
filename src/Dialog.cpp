#include "Dialog.h"
#include "ui_Dialog.h"

#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QDebug>
#include <QFile>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QPixmap>
#include <QApplication>
#include <QStyleFactory>


namespace {
const char *STYLE_CPY = "QPushButton { border: none; color: #2980b9; text-decoration: underline; background: transparent; }"
                        "QPushButton:hover { color: #e67e22; }";
const char *STYLE_DEL = "QPushButton { border: none; color: #c0392b; text-decoration: underline; background: transparent; }"
                        "QPushButton:hover { color: #e74c3c; }";
const char *STYLE_FAV = "QPushButton { border: none; color: #16a085; text-decoration: underline; background: transparent; }"
                        "QPushButton:hover { color: #27ae60; }";

// 查找某个按钮所在的行
static int findButtonRow(QTableWidget *table, int col, QPushButton *btn)
{
    for (int i = 0; i < table->rowCount(); ++i)
    {
        QWidget *cellWidget = table->cellWidget(i, col);
        if (cellWidget && cellWidget->findChildren<QPushButton *>().contains(btn))
        {
            return i;
        }
    }
    return -1;
}
}

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    resize(1800, 1000);

    // 初始化剪切板
    m_Clipboard = QApplication::clipboard();
    connect(m_Clipboard, &QClipboard::dataChanged, this, &Dialog::onClipboardDataChanged);

    setWindowFlags(windowFlags() | Qt::Tool | Qt::FramelessWindowHint);

    // 设置窗口为无边框和无标题栏
    //setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    ui->buttonGroup->setId(ui->btnClipboard, 0);
    ui->buttonGroup->setId(ui->btnFavorite, 1);
    ui->buttonGroup->setId(ui->btnSetting, 2);
    connect(ui->buttonGroup, &QButtonGroup::idPressed, this, &Dialog::slotIdPressed);

    // 设置表格
    setupClipboardTable();
    setupFavoriteTable();

    // 初始化托盘图标
    m_TrayIcon = new QSystemTrayIcon(this);
    m_TrayIcon->setIcon(QIcon(":/icons/image/MClipboard.png"));

    // 创建托盘菜单
    QMenu *trayMenu = new QMenu;
    QAction *showAction = trayMenu->addAction("显示主窗口");
    QAction *quitAction = trayMenu->addAction("退出");
    m_TrayIcon->setContextMenu(trayMenu);

    // 连接信号槽
    connect(m_TrayIcon, &QSystemTrayIcon::activated, this, &Dialog::onTrayIconActivated);
    connect(showAction, &QAction::triggered, this, &Dialog::showMe);
    connect(quitAction, &QAction::triggered, &QApplication::quit);

    m_TrayIcon->show();

    m_DB.init();

    QList<QPair<QString, QString>> historyList = m_DB.getHistory(200);
    for (const auto &item : historyList) {
        addToClipboardHistory(item.second, item.first);
    }
    QList<QString> favList = m_DB.getFavorite();
    for (const auto &item : favList) {
        addToFavorite(item);
    }

    QIcon settingsIcon(":/icons/image/Settings.png");
    ui->btnSetting->setIcon(settingsIcon);

    connect(ui->btnAddFavorite, &QPushButton::clicked, this, &Dialog::addFavorite);
    connect(ui->btnClearHistory, &QPushButton::clicked, this, &Dialog::clearHistory);
    connect(ui->btnClearFavorite, &QPushButton::clicked, this, &Dialog::clearFavorite);

    //QPalette palette;
    //palette.setColor(QPalette::Window, QColor(53,53,53));
    //palette.setColor(QPalette::WindowText, Qt::white);
    // ...更多颜色设置...
    //QApplication::setPalette(palette);
}

Dialog::~Dialog()
{
    delete ui;
}

bool Dialog::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate)
    {
        hide();
    }
    return QDialog::event(event);
}

void Dialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
}

void Dialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void Dialog::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void Dialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = false;
        event->accept();
    }
}

void Dialog::setupClipboardTable()
{
    // 设置表格属性
    QTableWidget *table = ui->tableClipboard;

    // 设置表格样式
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置列宽
    table->setColumnWidth(0, 300); // 时间
    table->setColumnWidth(2, 300); // 操作
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // 内容
}

void Dialog::setupFavoriteTable()
{
    // 设置表格属性
    QTableWidget *table = ui->tableFavorite;

    // 设置表格样式
    table->setEditTriggers(QAbstractItemView::DoubleClicked);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置列宽
    table->setColumnWidth(1, 300); // 操作
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); // 内容
}

void Dialog::addToClipboardHistory(const QString &text, const QString &dt)
{
    if (text.trimmed().isEmpty())
    {
        return; // 忽略空内容
    }

    QTableWidget *table = ui->tableClipboard;

    // 检查是否已存在相同内容，存在则删除该行
    for (int i = 0; i < table->rowCount(); ++i)
    {
        if (table->item(i, 1) && table->item(i, 1)->toolTip() == text)
        {
            table->removeRow(i);
            break;
        }
    }

    // 插入新行到最上面
    table->insertRow(0);

    // 添加时间
    table->setItem(0, 0, new QTableWidgetItem(dt));
    table->item(0, 0)->setTextAlignment(Qt::AlignCenter);

    // 添加内容
    table->setItem(0, 1, new QTableWidgetItem(text));
    table->item(0, 1)->setToolTip(text);

    // 添加"复制"按钮
    QPushButton *copyBtn = new QPushButton("复制");
    copyBtn->setStyleSheet(STYLE_CPY);
    copyBtn->setCursor(Qt::PointingHandCursor);

    // 添加"删除"按钮
    QPushButton *deleteBtn = new QPushButton("删除");
    deleteBtn->setStyleSheet(STYLE_DEL);
    deleteBtn->setCursor(Qt::PointingHandCursor);

    // 添加"收藏"按钮
    QPushButton *favBtn = new QPushButton("收藏");
    favBtn->setStyleSheet(STYLE_FAV);
    favBtn->setCursor(Qt::PointingHandCursor);

    // 用一个widget做容器
    QWidget *opWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(opWidget);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(copyBtn);
    layout->addWidget(deleteBtn);
    layout->addWidget(favBtn);
    layout->addStretch();
    opWidget->setLayout(layout);

    table->setCellWidget(0, 2, opWidget);

    // 复制按钮信号
    connect(copyBtn, &QPushButton::clicked, this, [this, table, copyBtn]() {
        int row = findButtonRow(table, 2, copyBtn);
        if (row != -1)
        {
            QTableWidgetItem *item = table->item(row, 1);
            if (item)
            {
                QString text = item->toolTip();
                QClipboard *clipboard = QApplication::clipboard();
                clipboard->setText(text);
            }
        }
    });

    // 删除按钮信号
    connect(deleteBtn, &QPushButton::clicked, this, [this, table, deleteBtn]() {
        int row = findButtonRow(table, 2, deleteBtn);
        if (row != -1)
        {
            QTableWidgetItem *item = table->item(row, 1);
            if (item)
            {
                QString text = item->toolTip();
                removeFromClipboardHistory(text);
            }
        }
    });

    // 收藏按钮信号
    connect(favBtn, &QPushButton::clicked, this, [this, table, favBtn]() {
        int row = findButtonRow(table, 2, favBtn);
        if (row != -1)
        {
            QTableWidgetItem *item = table->item(row, 1);
            if (item)
            {
                QString text = item->toolTip();
                addToFavorite(text);
                QTableWidget *favTable = ui->tableFavorite;
                int favRow = favTable->rowCount() - 1;
                favTable->scrollToItem(favTable->item(favRow, 0));
            }
        }
    });

    // 滚动到最上面（可选）
    table->scrollToItem(table->item(0, 0));

    m_DB.addHistory(text, dt);
}

void Dialog::addToFavorite(const QString &text)
{
    QTableWidget *table = ui->tableFavorite;

    // 检查是否已存在相同内容，存在则删除该行
    for (int i = 0; i < table->rowCount(); ++i)
    {
        if (table->item(i, 0) && table->item(i, 0)->toolTip() == text)
        {
            table->removeRow(i);
            break;
        }
    }

    table->insertRow(0);

    // 添加内容
    table->setItem(0, 0, new QTableWidgetItem(text));
    table->item(0, 0)->setToolTip(text);

    // 添加"复制"按钮
    QPushButton *copyBtn = new QPushButton("复制");
    copyBtn->setStyleSheet(STYLE_CPY);
    copyBtn->setCursor(Qt::PointingHandCursor);

    // 添加"删除"按钮
    QPushButton *deleteBtn = new QPushButton("删除");
    deleteBtn->setStyleSheet(STYLE_DEL);
    deleteBtn->setCursor(Qt::PointingHandCursor);

    // 用一个widget做容器
    QWidget *opWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(opWidget);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(copyBtn);
    layout->addWidget(deleteBtn);
    layout->addStretch();
    opWidget->setLayout(layout);

    table->setCellWidget(0, 1, opWidget);

    // 复制按钮信号
    connect(copyBtn, &QPushButton::clicked, this, [table, copyBtn]() {
        int row = findButtonRow(table, 1, copyBtn);
        if (row != -1)
        {
            QTableWidgetItem *item = table->item(row, 0);
            if (item)
            {
                QString text = item->text();
                QClipboard *clipboard = QApplication::clipboard();
                clipboard->setText(text);
            }
        }
    });

    // 删除按钮信号
    connect(deleteBtn, &QPushButton::clicked, this, [this, table, deleteBtn]() {
        int row = findButtonRow(table, 1, deleteBtn);
        if (row != -1)
        {
            QTableWidgetItem *item = table->item(row, 0);
            if (item)
            {
                QString text = item->text();
                removeFromFavorite(text);
            }
        }
    });

    m_DB.addFavorite(text);
}

void Dialog::showMe()
{
    show();
    raise();
    activateWindow();
}

void Dialog::slotIdPressed(int id)
{
    ui->stackedWidget->setCurrentIndex(id);
}

void Dialog::onClipboardDataChanged()
{
    // 获取剪贴板文本
    QString text = m_Clipboard->text();
    if (!text.isEmpty())
    {
        addToClipboardHistory(text, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
}

void Dialog::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        isVisible() ? hide() : showMe();
    }
}

void Dialog::removeFromClipboardHistory(const QString &content)
{
    QTableWidget *table = ui->tableClipboard;

    // 删除历史
    m_DB.removeHistory(content);

    // 删除收藏
    m_DB.removeFavorite(content);

    // 检查是否已存在相同内容，存在则删除该行
    for (int i = 0; i < table->rowCount(); ++i)
    {
        if (table->item(i, 1) && table->item(i, 1)->toolTip() == content)
        {
            table->removeRow(i);
            break;
        }
    }

    // 滚动到最上面（可选）
    table->scrollToItem(table->item(0, 0));
}

void Dialog::removeFromFavorite(const QString &content)
{
    QTableWidget *table = ui->tableFavorite;

    // 删除历史
    m_DB.removeHistory(content);

    // 删除收藏
    m_DB.removeFavorite(content);

    // 检查是否已存在相同内容，存在则删除该行
    for (int i = 0; i < table->rowCount(); ++i)
    {
        if (table->item(i, 0) && table->item(i, 0)->toolTip() == content)
        {
            table->removeRow(i);
            break;
        }
    }

    // 滚动到最上面（可选）
    table->scrollToItem(table->item(0, 0));
}

void Dialog::addFavorite()
{
    QString content = ui->lineEdit->text().trimmed();
    if (!content.isEmpty())
    {
        addToFavorite(content);
        ui->lineEdit->clear();
    }
}

// 1. 添加清除历史记录槽函数
void Dialog::clearHistory()
{
    // 清空数据库
    m_DB.clearHistory();

    // 清空表格
    ui->tableClipboard->clearContents();
}

void Dialog::clearFavorite()
{
    // 清空数据库
    m_DB.clearFavorite();

    // 清空表格
    ui->tableFavorite->clearContents();
}
