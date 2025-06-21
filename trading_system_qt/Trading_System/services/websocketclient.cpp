/* =========================================================================
   WebSocketClient.cpp – implementation of WebSocketClient.h
   -------------------------------------------------------------------------
   Socket bridge between the trader’s desktop GUI and the cloud-side
   TradeServer.

     • Sends JSON for “newTrade” and “closeTrade” when TradeManager /
       DisplayManager raise an event.
     • Parses server push messages, creating/patching local Trade objects,
       and re-emits:
         liveTrade(Trade*, pnl)        – mark-to-market or newly opened
         closeTradeIncomming(tradeID)  – server confirmed close.
     • Keeps an in-memory QList<Trade*> so tradeExists(id) can de-dupe
       duplicates that may arrive during latency spikes.
   ========================================================================= */

#include "websocketclient.h"
#include "Trading_System/displaymanager.h"

#include<QJsonDocument>
#include<QJsonObject>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent),
    webSocket(new QWebSocket),
    account(Account::getInstance()),
    url("ws://trading_cloud:12345/trade")
{
    connect(webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    webSocket -> open(QUrl(url));
}
WebSocketClient::~WebSocketClient(){
    webSocket->close();
    webSocket->deleteLater();
}

void WebSocketClient::setDisplayManager(DisplayManager* displayManager){
    this->displayManager=displayManager;
    connect(displayManager, &DisplayManager::closeTrade,this, &WebSocketClient::closeTradeOutgoing);
}

void WebSocketClient::setTradeManager(TradeManager* tradeManager){
    this->tradeManager=tradeManager;
    connect(tradeManager, &TradeManager::openTrade, this, &WebSocketClient::newTrade);
}

void WebSocketClient::onConnected(){
    QJsonObject obj;
    obj["connection"] = "tradeDashboard";
    obj["userID"] = account -> getUserID();
    QJsonDocument doc(obj);
    webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void WebSocketClient::onTextMessageReceived(const QString &message){
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    QString tradeID = obj["tradeID"].toString();

    if(!tradeExists(tradeID)){
        double stopLoss = obj["stopLoss"].toDouble();
        double takeProfit = obj["takeProfit"].toDouble();
        double size = obj["size"].toDouble();
        Asset asset = static_cast<Asset>(obj["asset"].toInt());
        double openPrice = obj["openPrice"].toDouble();
        QString position = obj["position"].toString();

        trades.append(new Trade(tradeID, stopLoss, takeProfit, size, asset,openPrice, type, position));
    }
    if(type == "open"){
        double pnL = obj["pnl"].toDouble();
        for (Trade *t : trades){
            if(t->getTradeID()==tradeID){
                emit liveTrade(t, pnL);
                break;
            }
        }
    } else if(type == "closed"){
        for (Trade *t : trades){
            if(t->getTradeID()==tradeID){
                trades.removeOne(t);
                emit closeTradeIncomming(tradeID);
                break;
            }
        }
    }
}

bool WebSocketClient::tradeExists(QString tradeID){
    for (Trade *t: trades){
        if(t->getTradeID()==tradeID)
            return true;
    }
    return false;
}

void WebSocketClient::newTrade(Trade *trade){
    QJsonObject obj;

    obj["newTrade"] = "newTrade";
    obj["userID"] = account->getUserID();
    obj["tradeID"] = trade->getTradeID();
    obj["stopLoss"] = trade->getStopLoss();
    obj["takeProfit"] = trade->getTakeProfit();
    obj["size"] = trade->getSize();
    obj["asset"] = static_cast<int>(trade->getAsset());
    obj["openPrice"] = trade->getOpenPrice();
    obj["type"] = trade->getType();
    obj["position"] = trade->getPosition();

    QJsonDocument doc(obj);
    QString message = doc.toJson(QJsonDocument::Compact);

    webSocket->sendTextMessage(message);
}

void WebSocketClient::closeTradeOutgoing(QString tradeID){
    QJsonObject obj;
    obj["closeTrade"] = "closeTrade";
    obj["userID"] = account->getUserID();
    obj["tradeID"] = tradeID;


    QJsonDocument doc(obj);
    QString message = doc.toJson(QJsonDocument::Compact);

    webSocket->sendTextMessage(message);
}
