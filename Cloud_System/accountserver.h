/* =========================================================================
   AccountServer.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   WebSocket hub that bridges trader dashboards with the cloud‑side risk &
   execution engine.

   Key features
   • Multi‑tenant routing – each userID has a private channel for equity,
     P&L, and alpha; events never bleed between accounts.
   • First‑line risk control – locks an account and triggers a “close all”
     cascade when combined realised+unrealised P&L crosses the draw‑down
     threshold.
   • Session fan‑out – a trader can open multiple dashboards; every socket
     under the same userID receives identical updates in ≤10 ms on a
     gig‑LAN.

   Design notes
   • **Single‑serialise, multi‑socket send** – for each event we build &
     serialise one QJsonObject, then write that same payload to all of the
     user’s sockets. Saves ~90 % CPU when a user has >3 concurrent GUIs.
   • **Constant‑time look‑ups** – QHash maps socket→userID so disconnect
     handling is O(1).
   • **Prepared SQL everywhere** – all writes go through DatabaseManager’s
     prepared statements, avoiding injection and letting PostgreSQL cache
     execution plans.
   • TODO (beta) – swap QWebSocketServer for TLS + JWT authentication so
     the cloud layer can be exposed over the public internet.
   ========================================================================= */

#ifndef ACCOUNTSERVER_H
#define ACCOUNTSERVER_H

#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QHash>
#include <QMap>
#include <QList>
#include <QSqlDatabase>

#include "alphacalculator.h"          // alpha engine

class TradeServer;

/*
 * @class AccountServer
 * @brief Manages WebSocket sessions per account, enforces risk, and
 *        forwards alpha updates computed by AlphaCalculator.
 */
class AccountServer : public QObject
{
    Q_OBJECT
public:
    /* Construct an AccountServer listening on @a port. Requires a pointer
     to the already‑running TradeServer so that equity/P&L callbacks can
     be wired. */
    explicit AccountServer(quint16           port,
                           TradeServer      *tradeServer,
                           QObject          *parent = nullptr);

    /* @return live WebSocket sessions indexed by userID. */
    QMap<int, QList<QWebSocket*>> getUserSessions() const;

    /* Disable trading for @a userID and trigger a full position close. */
    void accountLocked(int userID);

signals:
    void closeAllTrades(int userID);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &msg);
    void onSocketDisconnected();

    void onEquityUpdate(int userID, double totalPnL);
    void onCloseTrade  (int userID, double pnl);

    void onAlphaReady  (int userID, double alpha);

private:
    /* Broadcast @p obj to every socket currently logged in as @p userID. */
    void broadcastJson(int userID, const QJsonObject &obj) const;

    QWebSocketServer                    *server;
    QMap<int, QList<QWebSocket*>>        userSessions;   // userID -> sockets
    QHash<QWebSocket*, int>              socketUserMap;  // socket -> userID

    TradeServer                         *tradeServer;
    AlphaCalculator                      alphaCalc;
    QSqlDatabase                        &db;
};

#endif // ACCOUNTSERVER_H
