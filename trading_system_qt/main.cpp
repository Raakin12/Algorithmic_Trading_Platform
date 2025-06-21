#include <QApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

#include "account.h"
#include "mainwindow.h"
#include "DatabaseManager.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();
    if (!DatabaseManager::getInstance().initialize()) {
        qWarning() << "Failed to open the database. Exiting...";
        return -1;
    }

    Account *account = Account::getInstance();
    account->verifyAccount("SERIAL-ABC");

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

