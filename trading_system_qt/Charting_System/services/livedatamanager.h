/* =========================================================================
   LiveDataManager.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Streams real-time tick data from Binance’s WebSocket API and converts it
   into per-tick “sendTick” signals for ChartManager.

     • connectToWebSocket() opens the current URL and wires message and
       disconnect handlers.
     • timeFrameChange() / assetChange() rebuild the endpoint string via
       changeWebSocketUrl(), then reconnect so the live feed matches the
       user’s selection.
     • onTextMessageReceived() parses the k-line JSON, extracts open, high,
       low, close, and the “x” flag (k-line closed), and emits sendTick(...).

   Design notes
     • URL schema: wss://stream.binance.com:9443/ws/<symbol>@kline_<interval>
       where interval maps directly from the TimeFrame enum.
     • The QWebSocket instance lives for the lifetime of this object; on
       reconnect we simply close() then open() with the new URL to reuse the
       same socket object.
     • TODO – exponential back-off after 3 consecutive disconnects and an
       on-screen status indicator inside ChartWidget.
   ========================================================================= */

#ifndef LIVEDATAMANAGER_H
#define LIVEDATAMANAGER_H

#include "timeframe.h"
#include "asset.h"

#include <QDebug>
#include <QUrl>
#include <QObject>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

class ChartWidget;

class LiveDataManager : public QObject
{
    Q_OBJECT

public:
    explicit LiveDataManager(QObject *parent = nullptr);
    ~LiveDataManager();

    void connectToWebSocket();        // open or reopen feed
    void changeWebSocketUrl();        // rebuild URL + reconnect
    void timeFrameChange(TimeFrame t);
    void assetChange(Asset a);

signals:
    /* timestamp (ms UTC), O/H/L/C, x==true when candle closed */
    void sendTick(qint64 timestamp, double open, double high,
                  double low, double close, bool x);

private slots:
    void onTextMessageReceived(const QString &message);
    void onDisconnected();

private:
    TimeFrame   timeframe {OneMinute};
    Asset       asset     {BTCUSDT};
    QString     url;
    QWebSocket *websocket {nullptr};

    ChartWidget *chartWidget {nullptr};   // optional back-reference
};

#endif // LIVEDATAMANAGER_H
