/* =========================================================================
   TradeServer.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Core engine that owns order lifecycle, live pricing feeds, and first‑line
   risk checks.

   Key features
   • WebSocket edge – one listening socket for trader GUIs plus four outbound
     sockets to market‑data streams (Asset0‑3).
   • Per‑user trade map lets us mark‑to‑market positions in O(#positions) on
     each tick.
   • Emits equityUpdate(user, totalPnL) so AccountServer can enforce
     draw‑down limits.

   Design notes
   • Tick fan‑in: each asset tick handler updates livePrices and then walks
     only the affected users’ trades, avoiding global scans.
   • BenchOpen captured at session start – used to derive benchmark return
     for AlphaCalculator.
   • All DB writes funnel through prepared statements (see TradeServer.cpp).
   • TODO: back‑pressure guard if trader floods orders >100/s.
   ========================================================================= */

#ifndef TRADESERVER_H
#define TRADESERVER_H

#include "qsqldatabase.h"
#include "qwebsocketserver.h"
#include "trade.h"
#include "alphacalculator.h"

#include <QObject>
#include <QHash>
#include <QMap>
#include <QList>
#include <QWebSocket>



class AccountServer;

class TradeServer : public QObject
{
    Q_OBJECT
public:
    explicit TradeServer(quint16 port, QObject *parent = nullptr);

    void setAccountServer(AccountServer *accountserver);

    double getTotalPnL(int userID);

signals:
    void tradeClosed (int userID, double pnl);
    void equityUpdate(int userID, double totalPnL);

private slots:
    void onCloseAllTrades(int userID);

    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    void onSocketDisconnected();

    void onAsset0Tick(const QString &message);
    void onAsset1Tick(const QString &message);
    void onAsset2Tick(const QString &message);
    void onAsset3Tick(const QString &message);

private:
    void initializeAssetWebSockets();
    void updateAssetPnL(int userID, Asset asset);
    void checkLimits   (int userID, Asset asset);
    void tradeDashboardUpdate(int userID, Asset asset);
    void closeTrade(int userID, const QString &tradeID);

    QWebSocketServer                 *server;
    AccountServer                    *accountServer {nullptr};
    QHash<QWebSocket*, int>           socketUserMap;
    QMap<int, QMap<Trade*, double>>   usersTradeMap;
    QMap<int, QList<QWebSocket*>>     userSessions;
    QList<QWebSocket*>                assetWebSockets;
    QMap<Asset, double>               livePrices;
    QSqlDatabase                     &db;
    AlphaCalculator                   alphaCalc;

    QDate                             currentDay { QDate::currentDate() };
    double                            benchOpen  { 0.0 };
};

#endif // TRADESERVER_H
