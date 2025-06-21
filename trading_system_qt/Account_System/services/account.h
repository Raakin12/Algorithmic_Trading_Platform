/* =========================================================================
   AccountRepository.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Represents a single trading account inside the Qt desktop client.

     • Holds live state (balance, equity, alpha, max-loss) and emits Qt
       signals so QML widgets refresh automatically.
     • Maintains a single WebSocket connection to the cloud AccountServer
       and routes inbound JSON messages to slots (balance, alpha, etc.).
     • Stores TradeHistory* objects and exposes them to QML via the
       Q_INVOKABLE getTradeHistoryVariant() helper.
     • verifyAccount(serial) binds a freshly launched GUI to its cloud
       account after scanning the serial QR code.

   Design notes
     • Singleton pattern (getInstance) → exactly one WebSocket per GUI.
       the same state object and you never open two WebSocket connections.
     • Local QSqlDatabase is used only for cached trade history; all live
       data comes from the cloud.
     • TODO: add TLS + exponential back-off in onConnected() before a public
       release.
   ========================================================================= */

#ifndef ACCOUNTREPOSITORY_H
#define ACCOUNTREPOSITORY_H

#include "TradeHistory.h"
#include <QString>
#include <QSqlDatabase>
#include <QObject>
#include <QList>
#include <QWebSocket>
#include <QVariantMap>
#include <QVariantList>

class Account : public QObject
{
    Q_OBJECT

public:
    static Account* getInstance(){
        if(instance == nullptr){
            instance = new Account();
            return instance;
        } else {
            return instance;
        }
    }

    Account(const Account&) = delete;
    Account& operator=(const Account&) = delete;

    bool verifyAccount(QString serial);

    QList<TradeHistory*> getTradeHistory() const;
    double getBalance() const;
    int getUserID() const;
    double getMaxLoss() const;
    double getEquity() const;
    double getAlpha() const;

    void handleBalanceUpdated();
    void handleAccountLocked();
    void handleAlphaUpdated();
    void handleTradeClosed();

    Q_INVOKABLE QVariantList getTradeHistoryVariant() const;

signals:
    void balanceUpdated(double balance);
    void accountLocked();
    void alphaUpdated(double alpha);
    void tradeHistoryUpdated(QList<TradeHistory*> history);
    void equityUpdated();

private slots:
    void onTextMessageReceived(const QString &message);
    void onConnected();

private:
    static Account* instance;
    QString serialID;
    double balance;
    double equity;
    int userID;
    QList<TradeHistory*> history;
    double alpha;
    double maxLoss;
    bool active;
    QSqlDatabase &db;
    QWebSocket *webSocket;
    QString url;

    void retrieveTradeHistory();
    explicit Account(QObject *parent = nullptr);
    ~Account();
};

#endif // ACCOUNT_H
