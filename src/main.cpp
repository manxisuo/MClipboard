#include "Dialog.h"
#include <QApplication>
#include <QFile>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QFile file("MClipboard.qss");
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        QString styleSheet = file.readAll();
        app.setStyleSheet(styleSheet);
        file.close();
    }

    Dialog dlg;
    Q_UNUSED(dlg)

    return app.exec();
}
