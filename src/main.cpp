#include <QApplication>
#include <QFile>
#include <QTextStream>

#include "UI/MainWindow.h"

int main(int argc, char *argv[])
{
    // Fusion 是跟自訂 QSS 搭配最不容易出怪問題的內建樣式基底，
    // 用系統原生樣式（Windows11Style 等）疊加大量自訂 QSS 常常會有些控制項套不到樣式。
    QApplication::setStyle(QStringLiteral("Fusion"));

    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Neil"));
    app.setApplicationName(QStringLiteral("MinecraftResourceCalculator"));

    QFile qssFile(QStringLiteral(":/theme.qss"));
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&qssFile);
        app.setStyleSheet(stream.readAll());
    }

    MainWindow window;
    window.resize(1240, 820);
    window.show();

    return app.exec();
}
