/* =========================================================================
   ChartManager.cpp – implementation of ChartManager.h
   -------------------------------------------------------------------------
   Glues HistoricalDataManager, LiveDataManager, and ChartWidget together.

     • Loads a back-fill of candles, then appends live ticks in real time.
     • Builds / updates one QCandlestickSeries and notifies ChartWidget via
       seriesUpdated() so the chart repaints automatically.
     • Handles GUI requests to change asset or timeframe by clearing the
       current series, re-loading history, and resuming live streaming.
   ========================================================================= */

#include "chartmanager.h"
#include "chartwidget.h"
#include <QDebug>
#include <QMetaObject>
#include <algorithm>

/* ------------------------------ ctor ---------------------------------- */
ChartManager::ChartManager(ChartWidget *chartWidget, QObject *parent)
    : QObject(parent)
    , historicalDataManager(new HistoricalDataManager(this))
    , liveDataManager(new LiveDataManager(this))
    , chartWidget(chartWidget)
    , series(nullptr)
    , currentLiveCandle(nullptr)
    , m_currentTimeFrame(OneMinute)
{
    qDebug() << "[ChartManager] Constructor called.";

    connect(historicalDataManager, &HistoricalDataManager::historicalDataReceived,
            this, &ChartManager::onHistoricalDataReceived);

    connect(liveDataManager, &LiveDataManager::sendTick,
            this, &ChartManager::onLiveTick);

    connect(chartWidget, &ChartWidget::assetChange,
            this, &ChartManager::onAssetChange);
    connect(chartWidget, &ChartWidget::timeframeChange,
            this, &ChartManager::onTimeFrameChange);
}

ChartManager::~ChartManager(){
    qDebug() << "[ChartManager] Destructor.";
}

void ChartManager::loadHistoricalData(){
    qDebug() << "[ChartManager] loadHistoricalData()";
    historicalDataManager->fetchHistoricalData();
}

void ChartManager::startLiveData(){
    qDebug() << "[ChartManager] startLiveData()";
    liveDataManager->connectToWebSocket();
}

void ChartManager::timeFrameChange(TimeFrame t){
    qDebug() << "[ChartManager] timeFrameChange(t =" << t << ")";
    clearSeries();
    m_currentTimeFrame = t; // store it
    historicalDataManager->timeFrameChange(t);
    liveDataManager->timeFrameChange(t);
}

void ChartManager::assetChange(Asset a){
    qDebug() << "[ChartManager] assetChange(a =" << a << ")";
    clearSeries();
    historicalDataManager->assetChange(a);
    liveDataManager->assetChange(a);
}

/* ------------------------- callbacks ---------------------------------- */
void ChartManager::onHistoricalDataReceived(const QCandlestickSeries *histSeries){
    qDebug() << "[ChartManager] onHistoricalDataReceived(). Count:" << histSeries->count();

    clearSeries();

    series = const_cast<QCandlestickSeries*>(histSeries);
    currentLiveCandle = nullptr;

    emit seriesUpdated(series);
}

void ChartManager::onLiveTick(qint64 timestamp, double open, double high,
                              double low, double close, bool closed){

    if (!series) {
        qWarning() << "[ChartManager] No historical data loaded; ignoring live tick.";
        return;
    }

    if (!currentLiveCandle) {
        currentLiveCandle = new QCandlestickSet(timestamp);
        currentLiveCandle->setOpen(open);
        currentLiveCandle->setHigh(high);
        currentLiveCandle->setLow(low);
        currentLiveCandle->setClose(close);
        series->append(currentLiveCandle);
    } else {
        currentLiveCandle->setHigh(std::max(currentLiveCandle->high(), high));
        currentLiveCandle->setLow(std::min(currentLiveCandle->low(), low));
        currentLiveCandle->setClose(close);
    }

    emit seriesUpdated(series);

    if (closed) {
        currentLiveCandle = nullptr;
        emit seriesUpdated(series);
    }
}

void ChartManager::onAssetChange(int assetIndex){
    qDebug() << "[ChartManager] onAssetChange(int) =>" << assetIndex;
    clearSeries();

    // Defer so chart sees nullptr first
    QMetaObject::invokeMethod(this, [=]() {
        Asset newAsset = static_cast<Asset>(assetIndex);
        historicalDataManager->assetChange(newAsset);
        liveDataManager->assetChange(newAsset);
    }, Qt::QueuedConnection);
}

void ChartManager::onTimeFrameChange(int timeframeIndex){
    qDebug() << "[ChartManager] onTimeFrameChange(int) =>" << timeframeIndex;
    clearSeries();

    QMetaObject::invokeMethod(this, [=]() {
        TimeFrame tf = static_cast<TimeFrame>(timeframeIndex);
        m_currentTimeFrame = tf;
        historicalDataManager->timeFrameChange(tf);
        liveDataManager->timeFrameChange(tf);
    }, Qt::QueuedConnection);
}

/* ------------------------- helpers ------------------------------------ */
void ChartManager::clearSeries(){
    if (series) {
        qDebug() << "[ChartManager] clearSeries => removing old series from chart.";
        emit seriesUpdated(nullptr);
        series = nullptr;
        currentLiveCandle = nullptr;
    }
}
