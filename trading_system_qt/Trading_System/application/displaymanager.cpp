/* =========================================================================
   DisplayManager.cpp – implementation of DisplayManager.h
   -------------------------------------------------------------------------
   Mediates between GUI widgets (ExecutionWidget, TradeWidget), the local
   in-memory trade map, and the cloud-side TradeServer socket.

     • Streams Binance quote ticks for the currently-selected asset and
       emits liveAssetPrice(bid, ask) so ExecutionWidget can update its
       price labels in real-time.
     • Maintains tradeMap<Trade*, PnL> so the Open-Positions panel can show
       running P & L even before the cloud sends its next snapshot.
     • Performs local risk checks (equity & max-loss) before emitting a
       placeTrade(…) signal to WebSocketClient; rejects the order in-GUI if
       limits are hit.
     • Forwards close-trade presses from TradeWidget to the cloud and
       removes the row immediately on the next onClosedTrade() callback.
   ========================================================================= */

#include "displaymanager.h"
#include "tradewidget.h"
#include "executionwidget.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

/* ----------------------------------------------------------------------
   ctor – wire Binance tick socket + get Account singleton
   -------------------------------------------------------------------- */
DisplayManager::DisplayManager(QObject *parent)
    : QObject{parent},
    webSocket(new QWebSocket),
    url("wss://stream.binance.com:9443/ws/btcusdt@kline_1m"),
    account(Account::getInstance())
{
    connect(webSocket, &QWebSocket::textMessageReceived,
            this,       &DisplayManager::onTextMessageReceived);
    webSocket->open(QUrl(url));
}

/* widget setters ----------------------------------------------------- */
void DisplayManager::setTradeWidget(TradeWidget *tradeWidget){
    this->tradeWidget = tradeWidget;
    connect(tradeWidget, &TradeWidget::closeTradePressed,
            this,        &DisplayManager::closeTradeRequested);
}

void DisplayManager::setExecutionWidget(ExecutionWidget *executionWidget){
    this->executionWidget = executionWidget;
    connect(executionWidget, &ExecutionWidget::newTradePlaced,
            this,            &DisplayManager::inputTrade);
}

void DisplayManager::setWebSocketClient(WebSocketClient *webSocketClient){
    this->webSocketClient = webSocketClient;
    connect(webSocketClient, &WebSocketClient::liveTrade,
            this,            &DisplayManager::onLiveTrade);
    connect(webSocketClient, &WebSocketClient::closeTradeIncomming,
            this,            &DisplayManager::onClosedTrade);
}

DisplayManager::~DisplayManager() = default;

/* ------------------------------------------------------------------
   Switch Binance feed when user picks a new asset
   ---------------------------------------------------------------- */
void DisplayManager::changeWebSocketUrl(){
    QString baseUrl = "wss://stream.binance.com:9443/ws/";
    QString assetSymbol;
    switch (asset) {
    case BTCUSDT: assetSymbol = "btcusdt"; break;
    case ETHUSDT: assetSymbol = "ethusdt"; break;
    case SOLUSDT: assetSymbol = "solusdt"; break;
    case XRPUSDT: assetSymbol = "xrpusdt"; break;
    }

    url = baseUrl + assetSymbol + "@kline_1m";
    webSocket->close();
    webSocket->open(QUrl(url));
    qDebug() << "[DisplayManager] WebSocket URL changed to" << url;
}

/* expose current map to QML ---------------------------------------- */
QMap<Trade*, double> DisplayManager::getTradeMap() const{
    return tradeMap;
}

/* ------------------------------------------------------------------
   Binance k-line tick → emit best bid/ask to ExecutionWidget
   ---------------------------------------------------------------- */
void DisplayManager::onTextMessageReceived(const QString &message){
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qWarning() << "[DisplayManager] JSON parse failure.";
        return;
    }
    QJsonObject kline = doc.object()["k"].toObject();
    double high = kline["h"].toString().toDouble();
    double low  = kline["l"].toString().toDouble();
    emit liveAssetPrice(high, low);
}

/* ------------------------------------------------------------------
   liveTrade event – insert / update running PnL and notify UI
   ---------------------------------------------------------------- */
void DisplayManager::onLiveTrade(Trade* trade, double pnL){
    bool found = false;
    for (auto it = tradeMap.begin(); it != tradeMap.end(); ++it) {
        if (it.key()->getTradeID() == trade->getTradeID()) {
            it.value() = pnL; found = true; break;
        }
    }
    if (!found) tradeMap.insert(trade, pnL);
    emit tradeMapUpdated();
}

/* closedTrade event – drop row and notify UI ------------------------ */
void  DisplayManager::onClosedTrade(QString tradeID){
    for (auto it = tradeMap.begin(); it != tradeMap.end(); ++it) {
        if (it.key()->getTradeID() == tradeID) {
            delete it.key();
            tradeMap.erase(it);
            emit tradeMapUpdated();
            break;
        }
    }
}

/* ------------------------------------------------------------------
   Validate & forward new-trade request from ExecutionWidget
   ---------------------------------------------------------------- */
void DisplayManager::inputTrade(double stopLoss, double takeProfit,
                                double size,     int asset,
                                double openPrice, QString type,
                                QString position)
{
    /* ----- basic validation & risk checks ------------------------ */
    double costToOpen = size * openPrice;
    double potentialLoss = 0.0;

    if (position == "long") {
        if (stopLoss >= openPrice || takeProfit <= openPrice) { emit orderSuccesful(false); return; }
        potentialLoss = (openPrice - stopLoss) * size;
    } else if (position == "short") {
        if (stopLoss <= openPrice || takeProfit >= openPrice) { emit orderSuccesful(false); return; }
        potentialLoss = (stopLoss - openPrice) * size;
    } else { emit orderSuccesful(false); return; }

    double equity        = account->getEquity();
    double maxAllowedRisk= equity - account->getMaxLoss();
    if (costToOpen > equity || potentialLoss > maxAllowedRisk) {
        emit orderSuccesful(false); return;
    }

    /* ----- fire signal upstream ---------------------------------- */
    emit placeTrade(stopLoss, takeProfit, size, asset,
                    openPrice, type, position);
    emit orderSuccesful(true);
}

/* toolbar asset selector ------------------------------------------- */
void DisplayManager::assetChange(int assetIndex){
    asset = static_cast<Asset>(assetIndex);
    changeWebSocketUrl();
}

/* forward close-trade button press --------------------------------- */
void DisplayManager::closeTradeRequested(QString tradeID){
    emit closeTrade(tradeID);
}
