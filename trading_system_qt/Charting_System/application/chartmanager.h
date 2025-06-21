/* =========================================================================
   ChartManager.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Orchestrates price-chart data flow for ChartWidget.

     • Fetches a back-fill of candles via HistoricalDataManager, then starts
       LiveDataManager for streaming ticks.
     • Maintains one QCandlestickSeries that ChartWidget renders; emits
       seriesUpdated(series) whenever new candles arrive.
     • Reacts to GUI controls for assetChange() and timeFrameChange() so the
       user can switch symbols or durations on the fly.

   Design notes
     • A live tick updates currentLiveCandle. When the exchange flags the
       candle as *closed*, the set is appended to the series—so the chart
       grows in real time without clearing or re-loading the whole dataset.
     • clearSeries() + loadHistoricalData() gives a clean slate whenever the
       user changes asset or timeframe, then live streaming resumes.
     • TODO – add interactive chart tools (e.g., trend-line drawing, simple
       fib retracements, and right-click remove) so users can annotate price
       action directly in the GUI.
   ========================================================================= */

#ifndef CHARTMANAGER_H
#define CHARTMANAGER_H

#include <QObject>
#include <QCandlestickSeries>
#include <QCandlestickSet>

#include "historicaldatamanager.h"
#include "livedatamanager.h"
#include "timeframe.h"
#include "asset.h"

class ChartWidget;

class ChartManager : public QObject
{
    Q_OBJECT
public:
    explicit ChartManager(ChartWidget *chartWidget, QObject *parent = nullptr);
    ~ChartManager();

    void loadHistoricalData();            // pull initial candle set
    void startLiveData();                 // begin streaming ticks

    void timeFrameChange(TimeFrame t);    // called by GUI controls
    void assetChange(Asset a);

    TimeFrame getCurrentTimeFrame() const { return m_currentTimeFrame; }

signals:
    void seriesUpdated(QCandlestickSeries *series);   // ChartWidget redraw

private slots:
    void onHistoricalDataReceived(const QCandlestickSeries *histSeries);
    void onLiveTick(qint64 timestamp, double open, double high,
                    double low, double close, bool closed);

    void onAssetChange(int assetIndex);       // GUI combobox hooks
    void onTimeFrameChange(int timeframeIndex);

private:
    void clearSeries();                       // drop candles before reload

    HistoricalDataManager *historicalDataManager;
    LiveDataManager       *liveDataManager;
    ChartWidget           *chartWidget;

    QCandlestickSeries    *series;            // live series bound to chart
    QCandlestickSet       *currentLiveCandle; // accumulating tick bucket

    TimeFrame m_currentTimeFrame;
};

#endif // CHARTMANAGER_H
