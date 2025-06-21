/* =========================================================================
   TradeManager.cpp – implementation of TradeManager.h
   -------------------------------------------------------------------------
   Lightweight “command bus” that converts a GUI ticket into a concrete
   Trade object.

     • Receives placeTrade(…) from DisplayManager when the trader presses
       “Market” in ExecutionWidget.
     • Instantiates a new Trade with the exact parameters and emits
       openTrade(trade) so WebSocketClient (or any other upstream slot) can
       serialise it to the cloud.
     • Holds zero business state, so unit-tests can create a dummy
       DisplayManager, fire placeTrade(…), and assert that exactly one
       Trade* is emitted.

   NOTE – Ownership of the freshly allocated Trade is transferred to the
          receiver of openTrade(trade); TradeManager never deletes it.
   ========================================================================= */

#include "trademanager.h"
#include "displaymanager.h"

/* ----------------------------------------------------------------------
   ctor – wire DisplayManager signal → local slot
   -------------------------------------------------------------------- */
TradeManager::TradeManager(DisplayManager* displayManager,
                           QObject* parent)
    : QObject{parent},
    displayManager(displayManager)
{
    connect(displayManager, &DisplayManager::placeTrade,
            this,           &TradeManager::executeTrade);
}

/* dtor – nothing to clean up -------------------------------------- */
TradeManager::~TradeManager() = default;

/* ------------------------------------------------------------------
   Build Trade object and fan it out
   ---------------------------------------------------------------- */
void TradeManager::executeTrade(double stopLoss,  double takeProfit,
                                double size,      int    asset,
                                double openPrice, QString type,
                                QString position)
{
    Trade *newTrade = new Trade(stopLoss, takeProfit, size,
                                static_cast<Asset>(asset),
                                openPrice, type, position);

    emit openTrade(newTrade);   // ownership moves to receiver
}
