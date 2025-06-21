/* =========================================================================
   HistoricalDataManager.cpp – implementation of HistoricalDataManager.h
   -------------------------------------------------------------------------
   Downloads back-fill candles from Binance’s REST API, converts the JSON
   array to a QCandlestickSeries, and emits historicalDataReceived(series).

     • changeNetworkUrl() rebuilds the endpoint whenever asset or timeframe
       changes, then immediately triggers fetchHistoricalData().
     • fetchHistoricalData() issues the GET request and hands the raw bytes
       to parseHistoricalData() on success.
     • parseHistoricalData() loops through the k-line JSON, builds candle
       sets, and appends them to a new series.

   NOTE – For demo speed we grab 1 000 rows max; raise &limit for larger
   back-fills or paginate until Binance’s 1 000-row soft cap is met.
   ========================================================================= */

#include "historicaldatamanager.h"
#include "qcandlestickset.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>

/* --------------------------- ctor / dtor --------------------------- */
HistoricalDataManager::HistoricalDataManager(QObject *parent)
    : QObject(parent)
    , timeframe(OneMinute)
    , asset(BTCUSDT)
    , url("https://api.binance.com/api/v3/klines?symbol=BTCUSDT&interval=1m&limit=100")
    , networkManager(new QNetworkAccessManager(this))
{
    qDebug() << "[HistoricalDataManager] Constructor. URL:" << url;
}

HistoricalDataManager::~HistoricalDataManager(){
    qDebug() << "[HistoricalDataManager] Destructor called.";
}

/* ------------------------------------------------------------------ */
/* Issue HTTP GET to Binance REST endpoint                            */
/* ------------------------------------------------------------------ */
void HistoricalDataManager::fetchHistoricalData(){
    qDebug() << "[HistoricalDataManager] fetchHistoricalData() =>" << url;

    QNetworkRequest req(url);
    QNetworkReply *reply = networkManager->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            parseHistoricalData(data);
        } else {
            qWarning() << "[HistoricalDataManager] Error fetching data:"
                       << reply->errorString();
        }
        reply->deleteLater();
    });
}

/* ------------------------------------------------------------------ */
/* Convert Binance k-lines JSON → QCandlestickSeries                  */
/* ------------------------------------------------------------------ */
void HistoricalDataManager::parseHistoricalData(const QByteArray &data){
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "[HistoricalDataManager] parse failed, not an array";
        return;
    }

    QJsonArray arr = doc.array();
    QCandlestickSeries *newSeries = new QCandlestickSeries();

    for (const QJsonValue &v : arr) {
        QJsonArray candle = v.toArray();
        if (candle.size() < 5) continue;   // guard bad rows

        qint64 ts   = candle[0].toDouble();
        double open = candle[1].toString().toDouble();
        double high = candle[2].toString().toDouble();
        double low  = candle[3].toString().toDouble();
        double close= candle[4].toString().toDouble();

        auto *set = new QCandlestickSet(ts);
        set->setOpen(open);
        set->setHigh(high);
        set->setLow(low);
        set->setClose(close);
        newSeries->append(set);
    }

    emit historicalDataReceived(newSeries);
}

/* ------------------------------------------------------------------ */
/* Helper – rebuild REST URL after asset / timeframe swap             */
/* ------------------------------------------------------------------ */
void HistoricalDataManager::changeNetworkUrl(){
    QString base = "https://api.binance.com/api/v3/klines";

    QString symbolStr;
    switch (asset) {
    case BTCUSDT: symbolStr = "BTCUSDT"; break;
    case ETHUSDT: symbolStr = "ETHUSDT"; break;
    case SOLUSDT: symbolStr = "SOLUSDT"; break;
    case XRPUSDT: symbolStr = "XRPUSDT"; break;
    }

    QString tfStr;
    switch (timeframe) {
    case OneMinute:     tfStr = "1m";  break;
    case FiveMinute:    tfStr = "5m";  break;
    case FifteenMinute: tfStr = "15m"; break;
    case OneHour:       tfStr = "1h";  break;
    case FourHour:      tfStr = "4h";  break;
    case OneDay:        tfStr = "1d";  break;
    }

    url = base + "?symbol=" + symbolStr +
          "&interval=" + tfStr +
          "&limit=1000";

    fetchHistoricalData();   // auto-refresh with new params
}

/* slots wired from ChartManager ------------------------------------ */
void HistoricalDataManager::timeFrameChange(TimeFrame t){
    timeframe = t;
    changeNetworkUrl();
}

void HistoricalDataManager::assetChange(Asset a){
    asset = a;
    changeNetworkUrl();
}
