/* =========================================================================
   Account.cpp – implementation of AccountRepository.h
   -------------------------------------------------------------------------
   Client-side singleton that
     • verifies an account by serial (pulls static data + trade history),
     • opens a WebSocket to the cloud AccountServer when active,
     • keeps live fields (balance, equity, alpha) in sync and emits Qt
       signals for QML widgets.
   ========================================================================= */

#include "account.h"
#include "DatabaseManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QSqlError>

/* ------------------------------------------------------------------ */
/*                       static singleton handle                      */
/* ------------------------------------------------------------------ */
Account* Account::instance = nullptr;

/* ctor – grab shared DB handle + prep websocket                      */
Account::Account(QObject *parent)
    : QObject(parent),
    db(DatabaseManager::getInstance().getDatabase()),
    webSocket(new QWebSocket),
    url("ws://trading_cloud:12346/account")        // cloud AccountServer
{
}

Account::~Account()
{
    webSocket->close();
    webSocket->deleteLater();
}

/* ------------------------------------------------------------------ */
/* verifyAccount() – one-time handshake when user scans QR/serial     */
/* ------------------------------------------------------------------ */
bool Account::verifyAccount(QString serial)
{
    const QString sql =
        QStringLiteral("SELECT * FROM \"Account\" WHERE serial_id='%1'")
            .arg(serial);

    QSqlQuery q(db);
    if (!q.exec(sql) || !q.next()) {
        qWarning() << "[verifyAccount] serial not found or SQL error";
        return false;
    }

    /* cache static fields */
    serialID = serial;
    balance  = q.value("balance").toDouble();
    userID   = q.value("user_id").toInt();
    alpha    = q.value("alpha").toDouble();
    maxLoss  = q.value("max_loss").toDouble();
    active   = q.value("status").toBool();

    /* 1️⃣ initial history */
    retrieveTradeHistory();
    emit tradeHistoryUpdated(history);

    /* 2️⃣ live socket only if account still active */
    if (!active) return false;

    connect(webSocket, &QWebSocket::connected,
            this,       &Account::onConnected);
    connect(webSocket, &QWebSocket::textMessageReceived,
            this,       &Account::onTextMessageReceived);

    webSocket->open(QUrl(url));
    return true;
}

/* ------------------------------------------------------------------ */
/* History helper – populate vector from DB (descending date)         */
/* ------------------------------------------------------------------ */
void Account::retrieveTradeHistory()
{
    const QString sql =
        QStringLiteral("SELECT * FROM \"Trade_History\" WHERE user_id=%1 "
                       "ORDER BY date DESC").arg(userID);

    QSqlQuery q(db);
    if (!q.exec(sql)) {
        qWarning() << "[retrieveTradeHistory] SQL:" << q.lastError();
        return;
    }

    while (q.next()) {
        auto *th = new TradeHistory;
        th->setTradeID      (q.value("trade_id").toString());
        th->setUserID       (q.value("user_id").toInt());
        th->setSize         (q.value("size").toDouble());
        th->setAsset        (static_cast<Asset>(q.value("asset").toInt()));
        th->setOpenPrice    (q.value("openPrice").toDouble());
        th->setClosingPrice (q.value("closingPrice").toDouble());
        th->setPnl          (q.value("pnl").toDouble());
        th->setDate         (q.value("date").toDate());

        history.append(th);
    }
}

/* --------------------- simple inline getters ---------------------- */
QList<TradeHistory*> Account::getTradeHistory() const { return history; }
double Account::getBalance() const { return balance; }
int    Account::getUserID()  const { return userID; }
double Account::getMaxLoss() const { return maxLoss; }
double Account::getEquity()  const { return equity; }
double Account::getAlpha()   const { return alpha; }

/* ------------------------------------------------------------------ */
/* WebSocket handshake                                                */
/* ------------------------------------------------------------------ */
void Account::onConnected()
{
    equity = balance;      // reset snapshot

    QJsonObject obj;
    obj["connection"] = "account";
    obj["userID"]     = userID;

    webSocket->sendTextMessage(
        QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

/* ------------------------------------------------------------------ */
/* Incoming JSON router                                               */
/* ------------------------------------------------------------------ */
void Account::onTextMessageReceived(const QString &msg)
{
    const QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    const QJsonObject obj  = doc.object();
    const QString     type = obj["type"].toString();

    if      (type == "accountLocked") handleAccountLocked();
    else if (type == "alphaUpdated")  handleAlphaUpdated();
    else if (type == "tradeClosed")   handleTradeClosed();
    else if (type == "equity") {
        equity = obj["equityUpdate"].toDouble();
        emit equityUpdated();
    }
}

/* ------------------------------------------------------------------ */
/* tiny helper queries used by the slots                              */
/* ------------------------------------------------------------------ */
void Account::handleBalanceUpdated()
{
    const QString sql =
        QStringLiteral("SELECT balance FROM \"Account\" WHERE serial_id='%1'")
            .arg(serialID);

    QSqlQuery q(db);
    if (q.exec(sql) && q.next()) {
        balance = q.value(0).toDouble();
        emit balanceUpdated(balance);
    }
}

void Account::handleAccountLocked()
{
    active = false;
    emit accountLocked();
}

void Account::handleAlphaUpdated()
{
    const QString sql =
        QStringLiteral("SELECT alpha FROM \"Account\" WHERE serial_id='%1'")
            .arg(serialID);

    QSqlQuery q(db);
    if (q.exec(sql) && q.next()) {
        alpha = q.value(0).toDouble();
        emit alphaUpdated(alpha);
    }
}

/* ------------------------------------------------------------------ */
/* 🔑  FIX – when server says “tradeClosed” we reload the whole list  */
/* ------------------------------------------------------------------ */
void Account::handleTradeClosed()
{
    handleBalanceUpdated();           // balance label refresh

    /* clear old cache to avoid duplicates */
    qDeleteAll(history);
    history.clear();

    /* repopulate & emit */
    retrieveTradeHistory();
    emit tradeHistoryUpdated(history);
}

/* ------------------------------------------------------------------ */
/* QVariantList helper for QML                                        */
/* ------------------------------------------------------------------ */
QVariantList Account::getTradeHistoryVariant() const
{
    QVariantList arr;
    for (const TradeHistory *th : history) {
        QVariantMap m;
        m["tradeID"]    = th->getTradeID();
        m["asset"]      = static_cast<int>(th->getAsset());
        m["size"]       = th->getSize();
        m["openPrice"]  = th->getOpenPrice();
        m["closePrice"] = th->getClosingPrice();
        m["pnl"]        = th->getPnl();
        m["date"]       = th->getDate().toString(Qt::ISODate);
        arr.push_back(m);
    }
    return arr;
}
