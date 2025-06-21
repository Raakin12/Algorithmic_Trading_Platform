/* =========================================================================
   AccountServer.cpp – implementation of AccountServer.h
   -------------------------------------------------------------------------
   Bridges trader dashboards with the cloud‑side risk engine. Handles:
     • WebSocket session management per account (multi‑tenant).
     • Real‑time equity / P&L / alpha pushes.
     • First‑line risk lock when draw‑down breaches.
     • Delegates statistical alpha calculation to AlphaCalculator.
   ========================================================================= */

#include "accountserver.h"
#include "DatabaseManager.h"
#include "tradeserver.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QtMath>
#include <QDebug>

/* -------------------------------------------------------------------------
   ctor – spin up the WebSocket listener and wire cross‑module signals.
   ------------------------------------------------------------------------- */
AccountServer::AccountServer(quint16 port,
                             TradeServer *ts,
                             QObject *parent)
    : QObject(parent),
    server(new QWebSocketServer(QStringLiteral("Account WS"),
                                QWebSocketServer::NonSecureMode, this)),
    tradeServer(ts),
    alphaCalc(this),
    db(DatabaseManager::getInstance().getDatabase())
{
    // 1. Listen on the requested port (0.0.0.0). Any failure is fatal to UX.
    if (server->listen(QHostAddress::Any, port)) {
        qDebug() << "Account WS listening on" << port;
        connect(server, &QWebSocketServer::newConnection,
                this,   &AccountServer::onNewConnection);
    } else {
        qCritical() << "Account WS failed to listen!";
    }

    // 2. Cross‑module hooks (signals come from TradeServer + AlphaCalculator)
    connect(tradeServer, &TradeServer::tradeClosed,
            this,        &AccountServer::onCloseTrade);
    connect(tradeServer, &TradeServer::equityUpdate,
            this,        &AccountServer::onEquityUpdate);
    connect(&alphaCalc, &AlphaCalculator::alphaUpdated,
            this,       &AccountServer::onAlphaReady);
}

/* -------------------------------------------------------------------------
   Handle a raw TCP connection → upgrade to WebSocket and wait for the
   client to identify itself via a JSON handshake.
   ------------------------------------------------------------------------- */
void AccountServer::onNewConnection(){
    auto *sock = server->nextPendingConnection();
    if (!sock) return;   // defensive: should never happen

    // Wire per‑socket handlers.
    connect(sock, &QWebSocket::textMessageReceived,
            this, &AccountServer::onTextMessageReceived);
    connect(sock, &QWebSocket::disconnected,
            this, &AccountServer::onSocketDisconnected);

    qDebug() << "AccountServer: raw client connected";
}

/* -------------------------------------------------------------------------
   First message from the dashboard must include { connection: "account",
   userID: <int> }. Register the socket under that UID for future fan‑out.
   ------------------------------------------------------------------------- */
void AccountServer::onTextMessageReceived(const QString &msg){
    auto *sock = qobject_cast<QWebSocket*>(sender());
    if (!sock) return;   // should never happen

    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;   // ignore garbage

    const QJsonObject obj = doc.object();
    const QString      connType = obj.value("connection").toString();

    if (connType == "account") {
        int uid = obj.value("userID").toInt();
        userSessions[uid].append(sock);
        socketUserMap[sock] = uid;
        qDebug() << "AccountServer: registered socket for user" << uid;
    }
}

/* ------------------------------------------------------------------------- */
void AccountServer::onSocketDisconnected(){
    auto *sock = qobject_cast<QWebSocket*>(sender());
    if (!sock) return;

    int uid = socketUserMap.take(sock);   // 0 if not found
    if (uid != 0) {
        userSessions[uid].removeOne(sock);
        if (userSessions[uid].isEmpty())
            userSessions.remove(uid);
    }
    sock->deleteLater();
    qDebug() << "AccountServer: socket closed for user" << uid;
}

/* -------------------------------------------------------------------------
   Simple getter – exposed mainly for unit tests & monitoring widgets.
   ------------------------------------------------------------------------- */
QMap<int, QList<QWebSocket*>> AccountServer::getUserSessions() const{
    return userSessions;
}

/* -------------------------------------------------------------------------
   Equity update from TradeServer. Perform draw‑down check, update DB, and
   broadcast fresh equity to the dashboard.
   ------------------------------------------------------------------------- */
void AccountServer::onEquityUpdate(int userID, double totalPnL){
    QSqlQuery q(db);
    q.prepare("SELECT balance, max_loss FROM \"Account\" WHERE user_id=:u");
    q.bindValue(":u", userID);
    if (!q.exec() || !q.next()) {
        qWarning() << "[EquityUpdate] SQL fail:" << q.lastError();
        return;
    }
    const double bal   = q.value(0).toDouble();
    const double mLoss = q.value(1).toDouble();

    const double equity = bal + totalPnL;

    if (equity <= mLoss) {
        // Hard breach: lock account and exit early.
        q.prepare("UPDATE \"Account\" SET balance=:b WHERE user_id=:u");
        q.bindValue(":b", equity);
        q.bindValue(":u", userID);
        q.exec();
        accountLocked(userID);
        return;
    }

    // Broadcast real‑time equity.
    QJsonObject o;  o["type"] = "equity";  o["equityUpdate"] = equity;
    broadcastJson(userID, o);
}

/* -------------------------------------------------------------------------
   Trade has closed – update balance, check risk lock, push UI event, and
   feed the trade result into AlphaCalculator.
   ------------------------------------------------------------------------- */
void AccountServer::onCloseTrade(int userID, double pnl){
    // 1. DB update (balance bump + read back new balance & max_loss)
    QSqlQuery q(db);
    q.prepare("UPDATE \"Account\" SET balance = balance + :p "
              "WHERE user_id = :u RETURNING balance, max_loss");
    q.bindValue(":p", pnl);
    q.bindValue(":u", userID);
    if (!q.exec() || !q.next()) {
        qWarning() << "[CloseTrade] SQL fail:" << q.lastError();
        return;
    }
    const double newBal = q.value(0).toDouble();
    const double maxL   = q.value(1).toDouble();

    if (newBal <= maxL) {
        accountLocked(userID);
    } else {
        QJsonObject o;  o["type"] = "tradeClosed";
        broadcastJson(userID, o);
    }

    // 2. Feed realised trade into alpha calculator.
    const double w = qAbs(pnl);
    if (w > 0) {
        const double rp = pnl / w;
        const QDate   d = QDate::currentDate();
        alphaCalc.addTrade(userID, d, rp, w);
    }
}

/* -------------------------------------------------------------------------
   AlphaCalculator produced a new alpha value → persist + broadcast.
   ------------------------------------------------------------------------- */
void AccountServer::onAlphaReady(int userID, double alpha){
    QSqlQuery q(db);
    q.prepare("UPDATE \"Account\" SET alpha=:a WHERE user_id=:u");
    q.bindValue(":a", alpha);
    q.bindValue(":u", userID);
    if (!q.exec())
        qWarning() << "[AlphaWrite] SQL:" << q.lastError();

    QJsonObject o;  o["type"] = "alphaUpdated";  o["alpha"] = alpha;
    broadcastJson(userID, o);
}

/* -------------------------------------------------------------------------
   Lock account in DB, force‑close trades via TradeServer, push dashboard
   notification.
   ------------------------------------------------------------------------- */
void AccountServer::accountLocked(int userID){
    // Flag account as disabled.
    QSqlQuery q(db);
    q.prepare("UPDATE \"Account\" SET status=false WHERE user_id=:u");
    q.bindValue(":u", userID);
    q.exec();

    // Cascade close trades (TradeServer listens to this signal).
    emit closeAllTrades(userID);

    // Notify dashboard.
    QJsonObject o;   o["type"] = "accountLocked";
    broadcastJson(userID, o);
}

/* -------------------------------------------------------------------------
   Helper – send compact JSON string to every socket for the given uid.
   One serialisation per event keeps CPU + latency under control.
   ------------------------------------------------------------------------- */
void AccountServer::broadcastJson(int uid,
                                  const QJsonObject &obj) const{
    if (!userSessions.contains(uid)) return;
    const QString msg = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    for (auto *ws : userSessions[uid])
        ws->sendTextMessage(msg);
}
