#include "mainwindow.h"
#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("LVGL Font Generator");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("LvglTools");

    MainWindow window;
    window.show();

    return app.exec();
}
