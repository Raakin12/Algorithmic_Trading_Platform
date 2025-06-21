/* =========================================================================
   HistoricalDataManager.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Pulls back-fill candles from the exchange REST API and converts them into
   a QCandlestickSeries for the chart.

     • fetchHistoricalData() issues the HTTP request; on reply,
       parseHistoricalData() converts JSON/CSV → QCandlestickSeries and
       emits historicalDataReceived(series).
     • timeFrameChange() and assetChange() update the REST URL template and
       trigger a new fetch so the chart reloads when the user switches
       symbol or duration.

   Design notes
     • changeNetworkUrl() rebuilds the endpoint string whenever timeframe or
       asset changes, keeping fetchHistoricalData() stateless.
     • QNetworkAccessManager lives for the life of this object so multiple
       requests can pipeline without re-allocating sockets.
   ========================================================================= */

#ifndef HISTORICALDATAMANAGER_H
#define HISTORICALDATAMANAGER_H

#include <QObject>
#include <QCandlestickSeries>
#include <QNetworkAccessManager>
#include "timeframe.h"
#include "asset.h"

class HistoricalDataManager : public QObject
{
    Q_OBJECT
public:
    explicit HistoricalDataManager(QObject *parent = nullptr);
    ~HistoricalDataManager();

    void fetchHistoricalData();                 // fire HTTP request
    void parseHistoricalData(const QByteArray &data);
    void timeFrameChange(TimeFrame t);          // update REST URL
    void assetChange(Asset a);

signals:
    void historicalDataReceived(const QCandlestickSeries *series);

private:
    /* current parameters for the REST endpoint */
    TimeFrame timeframe {OneMinute};
    Asset     asset     {BTCUSDT};
    QString   url;

    QNetworkAccessManager *networkManager {nullptr};

    void changeNetworkUrl();                   // rebuild `url`
};

#endif // HISTORICALDATAMANAGER_H
