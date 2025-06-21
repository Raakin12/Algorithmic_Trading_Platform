/* =========================================================================
   TradeHistoryWidget.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Embeds TradeHistory.qml inside a QQuickWidget and exposes the trader’s
   historical fills as a QML-friendly QVariantList.

     • Subscribes to Account::tradeHistoryUpdated(...) and converts the
       QList<TradeHistory*> payload into a QVariantList of plain maps that
       QML tables / ListViews can consume.
     • Declares a Q_PROPERTY so QML can bind directly:
         ListView { model: tradeHistoryWidgetBackend.tradeHistory }
     • getQuickWidget() lets callers (AccountWidget / MainWindow) dock the
       widget wherever they like in a classic QWidget layout.

   Design notes
     • userTradeHistory is cached in C++ to avoid rebuilding the list on
       every delegate refresh; it is only regenerated when the Account
       backend emits an update.
     • The QML root object is cached (qmlRootObject) for any future
       property pushes without look-ups.
   ========================================================================= */

#ifndef TRADEHISTORYWIDGET_H
#define TRADEHISTORYWIDGET_H

#include <QObject>
#include <QVariantList>
#include <QQuickWidget>

#include "account.h"
#include "TradeHistory.h"

class TradeHistoryWidget : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList tradeHistory READ tradeHistory NOTIFY tradeHistoryUpdated)

public:
    explicit TradeHistoryWidget(QWidget* parentWidget = nullptr,
                                QObject* parent       = nullptr);
    ~TradeHistoryWidget();

    QQuickWidget* getQuickWidget() const;   // expose for docking
    QVariantList  tradeHistory()   const;   // Q_PROPERTY getter

public slots:
    /* Slot wired to Account::tradeHistoryUpdated(...) */
    void onTradeHistoryUpdated(const QList<TradeHistory*> &history);

signals:
    void tradeHistoryUpdated();             // notifies QML bindings

private:
    QQuickWidget* quickWidget      {nullptr};
    QQuickItem*   qmlRootObject    {nullptr};
    Account*      account          {nullptr};
    QVariantList  userTradeHistory;         // cached QML-ready list
};

#endif // TRADEHISTORYWIDGET_H
