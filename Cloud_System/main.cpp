#include <QCoreApplication>
#include <QDebug>
#include "accountserver.h"
#include "tradeserver.h"
#include "DatabaseManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();
    if (!DatabaseManager::getInstance().initialize()) {
        qWarning() << "Failed to open the database. Exiting...";
        return -1;
    }

    quint16 tradePort = 12345;
    TradeServer *tradeServer = new TradeServer(tradePort);
    qDebug() << "TradeServer started on port" << tradePort;

    quint16 accountPort = 12346;
    AccountServer *accountServer = new AccountServer(accountPort, tradeServer, nullptr);
    qDebug() << "AccountServer started on port" << accountPort;

    tradeServer->setAccountServer(accountServer);

    return app.exec();
}
