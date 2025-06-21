/* =========================================================================
   TradeWidget.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   QML table that lists every open position and lets the trader hit “X”
   to close a specific trade.

     • syncTrades() is called whenever DisplayManager says tradeMapUpdated;
       it converts the QMap<Trade*, PnL> into a QVariantList so the QML
       ListView can repaint.
     • onCloseTradeClicked(id) is invoked from QML when the user presses
       the close-button next to a row; the signal closeTradePressed(id)
       bubbles up to DisplayManager → TradeServer.
     • assetToString(Asset) maps enum to a short symbol for display.

   Design notes
     • Keeps no timers and no direct socket hooks: relies entirely on
       DisplayManager signals, so there’s zero threading or networking
       in this layer.
   ========================================================================= */

#ifndef TRADEWIDGET_H
#define TRADEWIDGET_H

#include <QObject>
#include <QQuickWidget>
#include <QQuickItem>
#include "displaymanager.h"

class TradeWidget : public QObject
{
    Q_OBJECT
public:
    explicit TradeWidget(QWidget* parentWidget = nullptr,
                         QObject* parent = nullptr);
    ~TradeWidget();

    QQuickWidget* widget() const;
    QString assetToString(Asset a);

    void setDisplayManager(DisplayManager *displayManager);

    Q_INVOKABLE void onCloseTradeClicked(QString tradeID);

signals:
    void closeTradePressed(const QString &tradeID);

private slots:
    void onTradeMapUpdated();

private:
    DisplayManager* displayManager {nullptr};
    QQuickWidget*   quickWidget    {nullptr};
    QQuickItem*     qmlRootObject  {nullptr};

    void syncTrades(const QMap<Trade*, double>& trades);
};

#endif // TRADEWIDGET_H
