/* =========================================================================
   ChartWidget.cpp – implementation of ChartWidget.h
   -------------------------------------------------------------------------
   QWidget wrapper that hosts a Qt Charts candlestick view plus an overlaid
   QML toolbar (asset + timeframe buttons).

     • Builds a vertically-stacked layout: QML controls on top, chart frame
       beneath.
     • Receives QCandlestickSeries* updates from ChartManager and redraws
       the view in real time (updateChart).
     • Emits assetChange(int) and timeframeChange(int) when the user taps
       toolbar buttons; ChartManager listens and reloads data.
     • Maintains a dashed line (lastPriceLine) at the most recent close and
       a timer-driven animateOpenCandle() to keep the active candle lively.
   ========================================================================= */

#include "chartwidget.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QVBoxLayout>
#include <QDebug>
#include <QDateTime>
#include <QBrush>
#include <QPen>
#include <QtCharts/QCandlestickSet>
#include <limits>

static const int maxCandlesToShow = 100;  // visible window
static const int BufferCandles    = 4;    // right-hand gap

/* -------------------------------------------------------------------------
   ctor – build UI, theme chart, wire context + timer
   ------------------------------------------------------------------------- */
ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
    , qmlWidget(new QQuickWidget(this))
    , frameContainer(new QFrame(this))
    , chart(new QChart())
    , chartView(new QChartView(chart, frameContainer))
    , chartManager(nullptr)
    , axisX(new QDateTimeAxis())
    , axisY(new QValueAxis())
    , series(nullptr)
    , lastPriceLine(new QLineSeries(this))
    , openCandleTimer(new QTimer(this))
    , lastAxisMin(QDateTime())
    , lastAxisMax(QDateTime())
    , lastMinY(0.0)
    , lastMaxY(0.0)
{
    qDebug() << "[ChartWidget] Constructor called.";

    /* ---------- QML toolbar --------------------------------------- */
    qmlWidget->engine()->rootContext()->setContextProperty("chartWidgetCpp", this);
    qmlWidget->setSource(QUrl(QStringLiteral("qrc:/Charting_System/ChartWidget.qml")));
    qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    /* ---------- Layout (QML top, chart bottom) -------------------- */
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    setLayout(outerLayout);
    outerLayout->addWidget(qmlWidget);

    frameContainer->setObjectName("ChartFrame");
    auto *frameLayout = new QVBoxLayout(frameContainer);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);
    frameLayout->addWidget(chartView);
    outerLayout->addWidget(frameContainer);

    /* ---------- Chart aesthetics ---------------------------------- */
    chartView->setStyleSheet("background: transparent; border: none;");
    chartView->setFrameShape(QFrame::NoFrame);

    QLinearGradient bg;
    bg.setStart(QPointF(0, 0));
    bg.setFinalStop(QPointF(0, 1));
    bg.setColorAt(0.0, QColor("#1A1A1A"));
    bg.setColorAt(1.0, QColor("#0C0C0C"));

    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->legend()->hide();
    chart->setBackgroundRoundness(0);
    chart->setBackgroundPen(Qt::NoPen);
    chart->setBackgroundBrush(bg);
    chart->setPlotAreaBackgroundBrush(Qt::transparent);
    chart->setPlotAreaBackgroundVisible(true);

    /* X axis (time) + Y axis (price) */
    axisX->setFormat("hh:mm");
    axisX->setLabelsColor(QColor("#F0B90B"));
    axisY->setLabelsColor(QColor("#F0B90B"));

    QPen axisPen(QColor("#F0B90B"));
    axisPen.setWidth(1);
    axisX->setLinePen(axisPen);
    axisY->setLinePen(axisPen);
    axisX->setGridLineVisible(false);
    axisY->setGridLineVisible(false);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    /* ---------- dashed last-price guide --------------------------- */
    {
        QPen linePen(QColor("#F0B90B"), 1.5, Qt::DashLine);
        linePen.setCosmetic(true);
        lastPriceLine->setPen(linePen);
        lastPriceLine->setUseOpenGL(true);
        chart->addSeries(lastPriceLine);
        lastPriceLine->attachAxis(axisX);
        lastPriceLine->attachAxis(axisY);
    }

    /* ---------- animate active candle ----------------------------- */
    connect(openCandleTimer, &QTimer::timeout,
            this, &ChartWidget::animateOpenCandle);
    openCandleTimer->start(1000);   // repaint every second
}

ChartWidget::~ChartWidget(){
    qDebug() << "[ChartWidget] Destructor.";
}

/* Inject ChartManager dependency and subscribe to seriesUpdated() */
void ChartWidget::setChartManager(ChartManager *manager){
    chartManager = manager;
    connect(chartManager, &ChartManager::seriesUpdated,
            this,          &ChartWidget::updateChart);
}

void ChartWidget::loadHistoricalData(){
    if (!chartManager) return;
    qDebug() << "[ChartWidget] loadHistoricalData()";
    chartManager->loadHistoricalData();
}

void ChartWidget::startLiveData(){
    if (!chartManager) return;
    qDebug() << "[ChartWidget] startLiveData()";
    chartManager->startLiveData();
}

/* ---------------------- toolbar callbacks ------------------------ */
Q_INVOKABLE void ChartWidget::onAssetButtonClicked(int assetValue){
    qDebug() << "[ChartWidget] onAssetButtonClicked =>" << assetValue;
    emit assetChange(assetValue);
}

Q_INVOKABLE void ChartWidget::onTimeframeButtonClicked(int timeframeValue){
    qDebug() << "[ChartWidget] onTimeframeButtonClicked =>" << timeframeValue;
    emit timeframeChange(timeframeValue);
}

/* -----------------------------------------------------------------
   updateChart() – redraws view whenever ChartManager pushes series
   ----------------------------------------------------------------- */
void ChartWidget::updateChart(QCandlestickSeries *series){

    if (!series) {                              // clear request
        qDebug() << "[ChartWidget] updateChart(nullptr) => removing old series.";
        chart->removeSeries(this->series);
        this->series = nullptr;
        lastPriceLine->clear();
        chart->update();
        return;
    }

    /* replace old series if needed */
    if (!chart->series().contains(series)) {
        chart->removeSeries(this->series);
        this->series = series;
        chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }

    /* slice to visible window ------------------------------------ */
    QList<QCandlestickSet*> allSets = series->sets();
    if (allSets.isEmpty()) {
        lastPriceLine->clear();
        return;
    }

    QList<QCandlestickSet*> visibleSets = allSets;
    if (allSets.size() > maxCandlesToShow)
        visibleSets = allSets.mid(allSets.size() - maxCandlesToShow);

    /* compute X-axis range */
    qint64 firstTs = visibleSets.first()->timestamp();
    qint64 lastTs  = visibleSets.last()->timestamp();
    qint64 msPer   = candleDurationMs(chartManager->getCurrentTimeFrame());
    qint64 span    = (lastTs - firstTs) + BufferCandles * msPer;

    QDateTime axisMin = QDateTime::fromMSecsSinceEpoch(firstTs);
    QDateTime axisMax = QDateTime::fromMSecsSinceEpoch(firstTs + span);

    /* compute Y-axis range with 30 % padding */
    double newMinY = std::numeric_limits<double>::max();
    double newMaxY = std::numeric_limits<double>::lowest();
    for (auto s : visibleSets) {
        newMinY = qMin(newMinY, s->low());
        newMaxY = qMax(newMaxY, s->high());
    }
    double pad = (newMaxY - newMinY) * 0.3;
    double minY = newMinY - pad;
    double maxY = newMaxY + pad;

    /* update axes only when changed */
    if (axisMin != lastAxisMin || axisMax != lastAxisMax) {
        axisX->setRange(axisMin, axisMax);
        axisX->setTickCount(6);
        lastAxisMin = axisMin;
        lastAxisMax = axisMax;
    }
    if (!qFuzzyCompare(minY, lastMinY) || !qFuzzyCompare(maxY, lastMaxY)) {
        axisY->setRange(minY, maxY);
        axisY->applyNiceNumbers();
        axisY->setTickCount(6);
        lastMinY = minY;
        lastMaxY = maxY;
    }

    /* style series */
    series->setBodyOutlineVisible(true);
    series->setPen(QPen(QColor("#F0B90B"), 0.2));
    series->setIncreasingColor(QColor("#44BB44"));
    series->setDecreasingColor(QColor("#FF4444"));
    series->setBodyWidth(0.6);

    /* dashed last-price guide */
    QCandlestickSet *lastCandle = allSets.last();
    double lastClose = lastCandle->close();
    QColor guideColor = (lastClose >= lastCandle->open()) ? QColor("#44BB44")
                                                          : QColor("#FF4444");

    lastPriceLine->clear();
    lastPriceLine->append(axisMin.toMSecsSinceEpoch(), lastClose);
    lastPriceLine->append(axisMax.toMSecsSinceEpoch(), lastClose);

    QPen dashedPen(guideColor, 1, Qt::DashLine);
    dashedPen.setCosmetic(true);
    lastPriceLine->setPen(dashedPen);

    /* ensure guide is top-most */
    if (chart->series().isEmpty() || chart->series().last() != lastPriceLine) {
        chart->removeSeries(lastPriceLine);
        chart->addSeries(lastPriceLine);
        lastPriceLine->attachAxis(axisX);
        lastPriceLine->attachAxis(axisY);
    }

    chart->update();
}

/* trivial repaint for the still-forming candle */
void ChartWidget::animateOpenCandle(){
    if (series) chart->update();
}

/* Convert enum TimeFrame → milliseconds */
qint64 ChartWidget::candleDurationMs(TimeFrame tf){
    switch (tf) {
    case OneMinute:     return   60'000;
    case FiveMinute:    return  300'000;
    case FifteenMinute: return  900'000;
    case OneHour:       return 3'600'000;
    case FourHour:      return 14'400'000;
    case OneDay:        return 86'400'000;
    }
    return 60'000;
}
