/* =========================================================================
   ChartWidget.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   QWidget wrapper that hosts a Qt Charts candlestick chart plus a small
   QML tool-bar for asset / timeframe buttons.

     • Receives QCandlestickSeries updates from ChartManager and redraws
       the chart in real time.
     • Emits assetChange(int) and timeframeChange(int) when the user taps
       a symbol or timeframe button in QML; ChartManager connects to these.
     • Uses animateOpenCandle() + QTimer to pulse the still-forming candle
       so traders can see the current minute/ hour evolve tick-by-tick.

   Design notes
     • candleDurationMs(tf) converts TimeFrame → milliseconds so the X-axis
       window can auto-scroll as new candles arrive.
     • lastPriceLine draws a horizontal guide at the most recent close; it
       updates on every tick without re-creating the whole chart.
     • TODO – add pinch-zoom and cross-hair inspection for granular study.
   ========================================================================= */

#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QQuickWidget>
#include <QFrame>
#include <QTimer>
#include <QDateTime>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCandlestickSeries>
#include <QtCharts/QLineSeries>

#include "chartmanager.h"

class ChartManager;

class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget();

    void setChartManager(ChartManager *manager);
    void loadHistoricalData();
    void startLiveData();

    /* Called from QML buttons ---------------------------------------- */
    Q_INVOKABLE void onAssetButtonClicked(int assetValue);
    Q_INVOKABLE void onTimeframeButtonClicked(int timeframeValue);

signals:
    void assetChange(int newAsset);           // forwarded to ChartManager
    void timeframeChange(int newTimeframe);

private slots:
    void updateChart(QCandlestickSeries *series);  // repaint on data push
    void animateOpenCandle();                      // pulse active candle

private:
    static qint64 candleDurationMs(TimeFrame tf);  // helper for axis range

    /* -------------- UI + data members ------------------------------ */
    QQuickWidget       *qmlWidget       {nullptr}; // top bar (asset / tf)
    QFrame             *frameContainer  {nullptr}; // holds chartView
    QChart             *chart           {nullptr};
    QChartView         *chartView       {nullptr};
    ChartManager       *chartManager    {nullptr};

    QDateTimeAxis      *axisX           {nullptr};
    QValueAxis         *axisY           {nullptr};
    QCandlestickSeries *series          {nullptr};
    QLineSeries        *lastPriceLine   {nullptr};

    QTimer             *openCandleTimer {nullptr};

    /* cached axis bounds for smooth auto-scroll */
    QDateTime          lastAxisMin;
    QDateTime          lastAxisMax;
    double             lastMinY {0.0};
    double             lastMaxY {0.0};
};

#endif // CHARTWIDGET_H
