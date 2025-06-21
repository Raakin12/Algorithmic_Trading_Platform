/* =========================================================================
   WebSocketClient.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Thin transport adapter that connects the desktop GUI to the cloud-side
   TradeServer.

     • Sends JSON commands for “newTrade” and “closeTrade”.
     • Parses server push messages and re-emits:
         liveTrade(Trade*, pnl)        – new or updated position
         closeTradeIncomming(tradeID)  – server confirmed close
     • tradeExists(id) lets higher layers ignore duplicate requests while
       awaiting confirmation.

   Design notes
     • onConnected() performs the user-ID handshake right after the WS
       upgrade so the server knows which account this socket serves.
     • All risk checks, file I/O, and GUI updates live in higher layers;
       this class is transport-only.
     • TODO – migrate to wss:// and add JWT authentication before public
       release.
   ========================================================================= */

#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include "Account_System/account.h"
#include "trademanager.h"

#include <QObject>

class DisplayManager;

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();
    bool tradeExists(QString tradeID);
    void setDisplayManager(DisplayManager* displayManager);
    void setTradeManager(TradeManager* tradeManager);

signals:
    void liveTrade(Trade* trade, double pnL);
    void closeTradeIncomming(QString TradeID);

private slots:
    void onTextMessageReceived(const QString &message);
    void onConnected();
    void newTrade(Trade *trade);
    void closeTradeOutgoing(QString tradeID);

private:
    QWebSocket *webSocket;
    Account *account;
    QList<Trade*> trades;
    TradeManager *tradeManager;
    DisplayManager *displayManager;
    QString url;
};

#endif // WEBSOCKETCLIENT_H
