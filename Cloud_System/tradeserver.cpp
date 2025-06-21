/* =========================================================================
   TradeServer.cpp – implementation of TradeServer.h
   -------------------------------------------------------------------------
   Bridges trader dashboards, live Binance price feeds, and the cloud-side
   risk engine.

     • Manages GUI WebSocket sessions and maps each socket to a user-ID.
     • Opens four outbound Binance 1-minute k-line feeds (BTC, ETH, SOL, XRP).
     • Keeps a per-user Trade* → PnL map and emits equityUpdate so
       AccountServer can enforce draw-down limits.
     • Handles stop-loss / take-profit hits automatically or via “closeTrade”.
     • Streams realised trades + benchmark returns into AlphaCalculator.
   ========================================================================= */

#include "tradeserver.h"
#include "DatabaseManager.h"
#include "accountserver.h"

#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QDateTime>
#include <QtDebug>

/* -------------------------------------------------------------------------
   ctor – start listening for dashboard sockets & wire alpha callback
   ------------------------------------------------------------------------- */
TradeServer::TradeServer(quint16 port, QObject *parent)
    : QObject(parent),
    server(new QWebSocketServer(QStringLiteral("Trade WebSocket Server"),
                                QWebSocketServer::NonSecureMode, this)),
    db(DatabaseManager::getInstance().getDatabase())
{
    if (server->listen(QHostAddress::Any, port)) {
        qDebug() << "[TradeServer] listening on port" << port;
        connect(server, &QWebSocketServer::newConnection,
                this,   &TradeServer::onNewConnection);
    } else {
        qFatal("[TradeServer] cannot listen – port busy?");
    }

    /* Pass through alpha updates to dashboards (if user online) */
    connect(&alphaCalc, &AlphaCalculator::alphaUpdated,
            this, [this](int uid, double alpha)
            {
                QSqlQuery q(db);
                q.prepare("UPDATE \"Account\" SET alpha=:a WHERE user_id=:u");
                q.bindValue(":a", alpha);
                q.bindValue(":u", uid);
                if (!q.exec())
                    qWarning() << q.lastError();

                if (accountServer &&
                    accountServer->getUserSessions().contains(uid)) {
                    QJsonObject obj;  obj["type"] = "alphaUpdated";
                    const QString msg =
                        QJsonDocument(obj).toJson(QJsonDocument::Compact);
                    for (QWebSocket *s : accountServer->getUserSessions()[uid])
                        if (s) s->sendTextMessage(msg);
                }
            });

    initializeAssetWebSockets();
}

void TradeServer::setAccountServer(AccountServer *acc){
    accountServer = acc;
    connect(accountServer, &AccountServer::closeAllTrades,
            this,           &TradeServer::onCloseAllTrades);
}

/* -------------------------------------------------------------------------
   Open four Binance price streams (1-min klines) and hook their tick slots
   ------------------------------------------------------------------------- */
void TradeServer::initializeAssetWebSockets(){
    const QString base = "wss://stream.binance.com:9443/ws/";
    const QStringList ep = {
        "btcusdt@kline_1m", "ethusdt@kline_1m",
        "solusdt@kline_1m", "xrpusdt@kline_1m"
    };

    for (const QString &e : ep) {
        QWebSocket *ws = new QWebSocket;
        ws->open(QUrl(base + e));
        assetWebSockets << ws;
    }
    connect(assetWebSockets[0], &QWebSocket::textMessageReceived,
            this, &TradeServer::onAsset0Tick);
    connect(assetWebSockets[1], &QWebSocket::textMessageReceived,
            this, &TradeServer::onAsset1Tick);
    connect(assetWebSockets[2], &QWebSocket::textMessageReceived,
            this, &TradeServer::onAsset2Tick);
    connect(assetWebSockets[3], &QWebSocket::textMessageReceived,
            this, &TradeServer::onAsset3Tick);
}

/* --------------------------- socket lifecycle --------------------------- */
void TradeServer::onNewConnection(){
    QWebSocket *sock = server->nextPendingConnection();
    connect(sock, &QWebSocket::textMessageReceived,
            this, &TradeServer::onTextMessageReceived);
    connect(sock, &QWebSocket::disconnected,
            this, &TradeServer::onSocketDisconnected);
    qDebug() << "[TradeServer] dashboard socket connected";
}

void TradeServer::onSocketDisconnected(){
    QWebSocket *s = qobject_cast<QWebSocket*>(sender());
    int uid = socketUserMap.take(s);
    if (uid != -1) userSessions[uid].removeAll(s);
    s->deleteLater();
}

/* --------------------------- dashboard API ----------------------------- */
void TradeServer::onTextMessageReceived(const QString &msg){
    QWebSocket *sock = qobject_cast<QWebSocket*>(sender());
    QJsonDocument d  = QJsonDocument::fromJson(msg.toUtf8());
    if (!d.isObject()) return;
    QJsonObject o = d.object();

    const QString conn = o.value("connection").toString();
    if (conn == "tradeDashboard") {
        int uid = o["userID"].toInt();
        socketUserMap[sock] = uid;
        userSessions[uid]  << sock;
        return;
    }

    /* ----- new trade request ------------------------------------------ */
    if (o.contains("newTrade")) {
        int     uid   = o["userID"].toInt();
        QString tid   = o["tradeID"].toString();
        Asset   asset = static_cast<Asset>(o["asset"].toInt());
        QString pos   = o["position"].toString();
        double  size  = o["size"].toDouble();
        double  sl    = o["stopLoss"].toDouble();
        double  tp    = o["takeProfit"].toDouble();
        double  open  = livePrices.value(asset, 0.0);

        Trade *t = new Trade(tid, sl, tp, size, asset,
                             open, o["type"].toString(), pos);

        double diff = (pos == "long") ? (livePrices[asset] - open)
                                      : (open - livePrices[asset]);
        usersTradeMap[uid][t] = diff * size;

        /* persist skeleton row */
        QSqlQuery q(db);
        q.prepare("INSERT INTO \"Trade_History\" "
                  "(trade_id,user_id,size,asset,openPrice,closingPrice,pnl,date) "
                  "VALUES(:id,:u,:s,:a,:op,0,0,:d)");
        q.bindValue(":id", tid);
        q.bindValue(":u",  uid);
        q.bindValue(":s",  size);
        q.bindValue(":a",  static_cast<int>(asset));
        q.bindValue(":op", open);
        q.bindValue(":d",  QDateTime::currentDateTime().toString(Qt::ISODate));
        q.exec();

        return;
    }

    /* ----- close trade request ---------------------------------------- */
    if (o.contains("closeTrade")) {
        closeTrade(o["userID"].toInt(), o["tradeID"].toString());
    }
}

/* ------------------------- PnL helpers --------------------------------- */
double TradeServer::getTotalPnL(int uid){
    double tot = 0.0;
    for (double p : usersTradeMap[uid].values()) tot += p;
    return tot;
}

void TradeServer::updateAssetPnL(int uid, Asset asset){
    for (auto it = usersTradeMap[uid].begin();
         it != usersTradeMap[uid].end(); ++it)
    {
        Trade *t = it.key();
        if (t->getAsset() != asset) continue;

        double diff = (t->getPosition() == "long")
                          ? (livePrices[asset] - t->getOpenPrice())
                          : (t->getOpenPrice() - livePrices[asset]);
        it.value() = diff * t->getSize();
    }
}

void TradeServer::checkLimits(int uid, Asset asset){
    for (Trade *t : usersTradeMap[uid].keys()) {
        if (t->getAsset() != asset) continue;

        double px = livePrices[asset];
        bool hit  = (t->getPosition() == "long")
                       ? (px >= t->getTakeProfit() || px <= t->getStopLoss())
                       : (px <= t->getTakeProfit() || px >= t->getStopLoss());

        if (hit) closeTrade(uid, t->getTradeID());
    }
}

/* Broadcast a snapshot of one user’s open trade & PnL ------------------- */
void TradeServer::tradeDashboardUpdate(int uid, Asset asset){
    for (Trade *t : usersTradeMap[uid].keys()) {
        if (t->getAsset() != asset) continue;

        QJsonObject obj;
        obj["type"]       = "open";
        obj["userID"]     = uid;
        obj["tradeID"]    = t->getTradeID();
        obj["stopLoss"]   = t->getStopLoss();
        obj["takeProfit"] = t->getTakeProfit();
        obj["size"]       = t->getSize();
        obj["asset"]      = static_cast<int>(asset);
        obj["openPrice"]  = t->getOpenPrice();
        obj["position"]   = t->getPosition();
        obj["pnl"]        = usersTradeMap[uid][t];

        const QString msg =
            QJsonDocument(obj).toJson(QJsonDocument::Compact);
        for (QWebSocket *s : userSessions[uid])
            if (s) s->sendTextMessage(msg);
    }
}

/* ------------------------------------------------------------------ */
void TradeServer::closeTrade(int uid, const QString &tid){
    for (Trade *t : usersTradeMap[uid].keys()) {
        if (t->getTradeID() != tid) continue;

        double live = livePrices[t->getAsset()];
        double diff = (t->getPosition() == "long")
                          ? (live - t->getOpenPrice())
                          : (t->getOpenPrice() - live);
        double pnl  = diff * t->getSize();

        /* persist closing price & PnL */
        QSqlQuery q(db);
        q.prepare("UPDATE \"Trade_History\" SET "
                  "closingPrice=:cp, pnl=:p, date=:d "
                  "WHERE trade_id=:id");
        q.bindValue(":cp", live);
        q.bindValue(":p",  pnl);
        q.bindValue(":d",  QDateTime::currentDateTime().toString(Qt::ISODate));
        q.bindValue(":id", tid);
        q.exec();

        /* feed realised trade into alpha model */
        const QDate today = QDate::currentDate();
        double rp = (live - t->getOpenPrice()) / t->getOpenPrice();
        double w  = std::abs(t->getSize() * t->getOpenPrice());
        alphaCalc.addTrade(uid, today, rp, w);

        usersTradeMap[uid].remove(t);
        emit tradeClosed(uid, pnl);

        /* >>> NEW: notify live dashboards that the trade is gone <<< */
        if (userSessions.contains(uid)) {
            QJsonObject obj;
            obj["type"]    = "closed";
            obj["userID"]  = uid;
            obj["tradeID"] = tid;
            obj["pnl"]     = pnl;
            const QString msg =
                QJsonDocument(obj).toJson(QJsonDocument::Compact);
            for (QWebSocket *s : userSessions[uid])
                if (s) s->sendTextMessage(msg);
        }
        /* ----------------------------------------------------------- */

        delete t;
        break;
    }
}

void TradeServer::onCloseAllTrades(int uid){
    for (Trade *t : usersTradeMap[uid].keys())
        closeTrade(uid, t->getTradeID());
}

/* Helper – extract candle close price from Binance kline JSON */
static inline double extractClose(const QString &msg){
    QJsonDocument d = QJsonDocument::fromJson(msg.toUtf8());
    return d.object()["k"].toObject()["c"].toString().toDouble();
}

/* ----------------------------------------------------------------------
   Macro generates four near-identical tick handlers (BTC, ETH, SOL, XRP)
   ---------------------------------------------------------------------- */
#define HANDLE_TICK(N, ENUM)                                             \
void TradeServer::onAsset ## N ## Tick(const QString &msg){              \
        const double px = extractClose(msg);                             \
        livePrices[ENUM] = px;                                           \
        const QDate today = QDate::currentDate();                        \
        /* BTC stream also drives benchmark return */                    \
        if (ENUM == Asset::BTCUSDT) {                                    \
            if (today != currentDay) {                                   \
                currentDay = today;                                      \
                benchOpen  = px;                                         \
        }                                                                \
            if (QTime::currentTime().hour() == 23 &&                     \
                                                                             QTime::currentTime().minute() >= 59) {                   \
                double rb = (px - benchOpen) / benchOpen;                \
                alphaCalc.addBenchmark(0, currentDay, rb, 1.0);          \
        }                                                                \
    }                                                                    \
        /* risk + PnL updates */                                         \
        for (int uid : usersTradeMap.keys()) {                           \
            checkLimits(uid, ENUM);                                      \
            updateAssetPnL(uid, ENUM);                                   \
    }                                                                    \
        for (int uid : accountServer->getUserSessions().keys()) {        \
            emit equityUpdate(uid, getTotalPnL(uid));                    \
            tradeDashboardUpdate(uid, ENUM);                             \
    }                                                                    \
}

HANDLE_TICK(0, Asset::BTCUSDT)
HANDLE_TICK(1, Asset::ETHUSDT)
HANDLE_TICK(2, Asset::SOLUSDT)
HANDLE_TICK(3, Asset::XRPUSDT)

#undef HANDLE_TICK
