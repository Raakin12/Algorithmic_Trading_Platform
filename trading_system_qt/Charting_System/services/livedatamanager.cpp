/* =========================================================================
   LiveDataManager.cpp – implementation of LiveDataManager.h
   -------------------------------------------------------------------------
   Streams real-time k-line data from Binance’s WebSocket API and forwards
   each tick to ChartManager.

     • connectToWebSocket() opens the socket for the current asset/timeframe.
     • changeWebSocketUrl() rebuilds the endpoint when the user switches
       symbol or duration, then reconnects.
     • onTextMessageReceived() parses the JSON tick, extracts timestamp,
       open/high/low/close plus the “x” (candle-closed) flag, and emits
       sendTick(…).
   ========================================================================= */

#include "livedatamanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

LiveDataManager::LiveDataManager(QObject *parent)
    : QObject{parent},
    url("wss://stream.binance.com:9443/ws/btcusdt@kline_1m"),
    websocket(new QWebSocket)
{
    qDebug() << "[LiveDataManager] Constructor called. URL:" << url;
}

LiveDataManager::~LiveDataManager(){
    qDebug() << "[LiveDataManager] Destructor called.";
    websocket->close();
    websocket->deleteLater();
}

void LiveDataManager::connectToWebSocket(){
    qDebug() << "[LiveDataManager] connectToWebSocket() called.";
    connect(websocket, &QWebSocket::connected, this, [this]() {
        qDebug() << "[LiveDataManager] Connected to Binance WebSocket!";
    });
    connect(websocket, &QWebSocket::textMessageReceived,
            this,       &LiveDataManager::onTextMessageReceived);
    connect(websocket, &QWebSocket::disconnected,
            this,       &LiveDataManager::onDisconnected);

    websocket->open(QUrl(url));
    qDebug() << "[LiveDataManager] WebSocket opened with URL:" << url;
}

void LiveDataManager::changeWebSocketUrl(){
    qDebug() << "[LiveDataManager] changeWebSocketUrl() called.";
    QString baseUrl = "wss://stream.binance.com:9443/ws/";
    QString assetSymbol;
    switch (asset) {
    case BTCUSDT: assetSymbol = "btcusdt"; break;
    case ETHUSDT: assetSymbol = "ethusdt"; break;
    case SOLUSDT: assetSymbol = "solusdt"; break;
    case XRPUSDT: assetSymbol = "xrpusdt"; break;
    }
    QString timeFrameStr;
    switch (timeframe) {
    case OneMinute:     timeFrameStr = "kline_1m";  break;
    case FiveMinute:    timeFrameStr = "kline_5m";  break;
    case FifteenMinute: timeFrameStr = "kline_15m"; break;
    case OneHour:       timeFrameStr = "kline_1h";  break;
    case FourHour:      timeFrameStr = "kline_4h";  break;
    case OneDay:        timeFrameStr = "kline_1d";  break;
    }
    url = baseUrl + assetSymbol + "@" + timeFrameStr;
    qDebug() << "[LiveDataManager] New WebSocket URL:" << url;
    websocket->close();
    websocket->open(QUrl(url));
}

void LiveDataManager::onTextMessageReceived(const QString &message){
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isObject()) {
        QJsonObject obj   = doc.object();
        QJsonObject kline = obj["k"].toObject();

        qint64 timestamp = kline["t"].toVariant().toLongLong();
        double open      = kline["o"].toString().toDouble();
        double high      = kline["h"].toString().toDouble();
        double low       = kline["l"].toString().toDouble();
        double close     = kline["c"].toString().toDouble();
        bool   x         = kline["x"].toBool();      // candle closed flag

        emit sendTick(timestamp, open, high, low, close, x);
    } else {
        qWarning() << "[LiveDataManager] onTextMessageReceived() failed to parse JSON.";
    }
}

void LiveDataManager::onDisconnected(){
    qDebug() << "[LiveDataManager] Disconnected from Binance WebSocket!";
}

void LiveDataManager::timeFrameChange(TimeFrame t){
    qDebug() << "[LiveDataManager] timeFrameChange() called with t =" << t;
    timeframe = t;
    changeWebSocketUrl();
}

void LiveDataManager::assetChange(Asset a){
    qDebug() << "[LiveDataManager] assetChange() called with asset =" << a;
    asset = a;
    changeWebSocketUrl();
}
