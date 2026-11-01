#include "mainwindow.h"
#include "utils/singleinstance.h"

#include <QApplication>
#include <QStandardPaths>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Vent";

    SingleInstance instance(directory);
    const SingleInstance::Result result = instance.start();
    if (result == SingleInstance::Result::Activated)
        return 0;
    if (result == SingleInstance::Result::Error) {
        QMessageBox::critical(nullptr, "Ошибка запуска", instance.errorString());
        return 1;
    }

    MainWindow w;
    QObject::connect(&instance, &SingleInstance::activationRequested, &w, &MainWindow::showAndActivate);
    w.setWindowTitle("Vent");
    w.show();
    return a.exec();
}
