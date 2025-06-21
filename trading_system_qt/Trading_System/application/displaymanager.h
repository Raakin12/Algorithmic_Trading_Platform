/* =========================================================================
   DisplayManager.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Glue layer that wires three GUI panels (ExecutionWidget, TradeWidget,
   live quote banner) to the WebSocketClient talking to the cloud-side
   TradeServer.

     • Maintains a local QMap<Trade*, double> so open positions and their
       running P & L are visible even if the cloud feed lags.
     • Forwards UI actions upstream:
         – placeTrade(…)  → “newTrade” JSON on the socket
         – closeTrade(id) → “closeTrade” JSON on the socket
     • Forwards cloud events downstream:
         – onLiveTrade()  → updates the map and TradeWidget
         – onClosedTrade()→ removes the position and updates equity label

   Design notes
     • changeWebSocketUrl() rebuilds the endpoint string when the user
       changes the asset from the toolbar, then reconnects the same
       QWebSocket to avoid reallocations.
     • Signals closeTradeLocal() and tradeMapUpdated() let QML list-views
       repaint without pulling.
     • TODO – Persist tradeMap to disk on graceful exit so a reconnect can
       restore the last known state instantly.
   ========================================================================= */

#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include "trade.h"
#include "websocketclient.h"
#include <QObject>
#include <QWebSocket>

class ExecutionWidget;
class TradeWidget;

class DisplayManager : public QObject
{
    Q_OBJECT
public:
    explicit DisplayManager(QObject *parent = nullptr);
    ~DisplayManager();

    void setTradeWidget(TradeWidget *tradeWidget);
    void setExecutionWidget(ExecutionWidget *executionWidget);
    void setWebSocketClient(WebSocketClient *webSocketClient);

    void changeWebSocketUrl();          // rebuild URL + reconnect
    void assetChange(int assetIndex);   // toolbar hook
    QMap<Trade*, double> getTradeMap() const;

signals:
    /* --- outbound to cloud --------------------------------------- */
    void closeTrade(QString tradeID);
    void placeTrade(double stopLoss, double takeProfit, double size,
                    int asset, double openPrice, QString type,
                    QString position);

    /* --- inbound to GUI ------------------------------------------ */
    void closeTradeLocal(QString tradeID);
    void tradeMapUpdated();
    void liveAssetPrice(double bid, double sell);
    void orderSuccesful(bool sucess);

private slots:
    /* raw socket messages from WebSocketClient */
    void onTextMessageReceived(const QString &message);

    /* decoded events */
    void onLiveTrade(Trade* trade, double pnL);
    void onClosedTrade(QString tradeID);

    /* UI inputs */
    void inputTrade(double stopLoss, double takeProfit, double size,
                    int asset, double openPrice, QString type,
                    QString position);
    void closeTradeRequested(QString tradeID);

private:
    WebSocketClient     *webSocketClient;
    QWebSocket          *webSocket;
    QMap<Trade*, double> tradeMap;          // open positions → running PnL
    QString              url;
    Asset                asset {BTCUSDT};

    ExecutionWidget     *executionWidget;
    TradeWidget         *tradeWidget;
    Account             *account;
};

#endif // DISPLAYMANAGER_H
