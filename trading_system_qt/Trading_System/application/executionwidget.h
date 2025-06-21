/* =========================================================================
   ExecutionWidget.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   QML-backed order-ticket panel where the trader enters size, stop-loss,
   take-profit and selects LONG vs SHORT.

     • Displays live bid / ask from DisplayManager so the user sees the
       latest quote before clicking “Market”.
     • onPlaceMarketTradeRequested(…) validates the inputs in QML, then
       forwards the details to DisplayManager via newTradePlaced(…).
     • onAssetChange(int) keeps the chart, quote banner, and ticket in sync
       when the user picks a new symbol.

   Design notes
     • Thin UI layer – all risk checks and cloud I/O live in DisplayManager;
       ExecutionWidget never blocks the GUI thread.
     • Signal handleOrderResult(bool) lets the QML side flash green/red
       feedback after DisplayManager confirms acceptance.
     • TODO – add keyboard shortcuts (Enter = submit, Esc = clear) for
       keyboard-centric scalpers.
   ========================================================================= */

#ifndef EXECUTIONWIDGET_H
#define EXECUTIONWIDGET_H

#include <QObject>
#include <QQuickWidget>
#include <QQuickItem>
#include "displaymanager.h"

class ExecutionWidget : public QObject
{
    Q_OBJECT
public:
    explicit ExecutionWidget(QWidget* parentWidget = nullptr,
                             QObject* parent = nullptr);
    ~ExecutionWidget();

    QQuickWidget* widget() const;
    void setDisplayManager(DisplayManager *displayManager);

    Q_INVOKABLE void onPlaceMarketTradeRequested(double openPrice,
                                                 double stopLoss,
                                                 double takeProfit,
                                                 double size,
                                                 int assetIndex,
                                                 QString position);

    Q_INVOKABLE void onAssetChange(int asset);

signals:
    void newTradePlaced(double stopLoss,
                        double takeProfit,
                        double size,
                        int asset,
                        double openPrice,
                        QString type,
                        QString position);

private slots:
    void onLiveAssetPrice(double bid, double sell);
    void handleOrderResult(bool success);

private:
    QQuickWidget   *quickWidget;
    QQuickItem     *qmlRootObject;
    DisplayManager *displayManager;
};

#endif // EXECUTIONWIDGET_H
