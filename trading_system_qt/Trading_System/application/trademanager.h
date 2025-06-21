/* =========================================================================
   TradeManager.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Thin command-bus that receives user order requests from ExecutionWidget
   (via DisplayManager) and emits a fully-constructed Trade* object so the
   cloud-side WebSocketClient can serialise it.

     • executeTrade(…) is a private slot wired by DisplayManager; it
       assembles a new Trade instance, stamps a UUID if needed, and emits
       openTrade(trade) upstream.
     • Keeps zero state other than the DisplayManager pointer, so unit
       testing is trivial.

   Design notes
     • Trade objects are allocated with `new` and ownership is transferred
       to whatever slot receives openTrade(…), avoiding double-delete risk.
     • Future work: add a TradeFactory to support limit/stop-entry orders
       without overloading executeTrade().
   ========================================================================= */

#ifndef TRADEMANAGER_H
#define TRADEMANAGER_H

#include "trade.h"

#include <QObject>
#include <QList>
#include <QUuid>

class DisplayManager;

class TradeManager : public QObject
{
    Q_OBJECT
public:
    explicit TradeManager(DisplayManager* displayManager,
                          QObject* parent = nullptr);
    ~TradeManager();

signals:
    /* Emitted after the Trade object is assembled and ready to send */
    void openTrade(Trade *trade);

private slots:
    /* Called by DisplayManager when the user clicks “Market”. */
    void executeTrade(double stopLoss, double takeProfit, double size,
                      int asset, double openPrice, QString type,
                      QString position);

private:
    DisplayManager *displayManager {nullptr};
};

#endif // TRADEMANAGER_H
