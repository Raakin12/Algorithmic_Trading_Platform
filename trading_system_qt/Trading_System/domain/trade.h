/* =========================================================================
   Trade.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Immutable record of a single position (open or closed).

   Key fields
   • tradeID – auto‑generated if not supplied; guarantees uniqueness for
     DB keys and UI diff views.
   • stopLoss / takeProfit – expressed in price units; 0 means "unset".
   • size – positive = long, negative = short; absolute value is quantity.
   • asset – symbol, tick size, and exchange metadata (see Asset.h).
   • openPrice – fill price at entry.
   • type – e.g., "market".
   • position – lifecycle status: "OPEN", "CLOSED".

   Design notes
   • Simple POD‑style class: getters only, no mutating logic beyond
     setStopLoss(), setTakeProfit(), and setPosition(). Risk checks happen
     at a higher layer (TradeManager).
   • tradeCounter provides deterministic IDs when backend‑generated; helps
     with offline unit tests where no UUID service is available.
   • No timestamps here – persisted in TradeHistory table alongside this
     schema to keep the core object small.
   • TODO (beta) – add limit and stop orders
   ========================================================================= */


#ifndef TRADE_H
#define TRADE_H

#include "asset.h"

#include <QString>
#include <QDateTime>



class Trade
{
public:
    Trade(QString tradeID, double stopLoss, double takeProfit, double size,
          Asset asset, double openPrice, QString type, QString position);

    Trade(double stopLoss, double takeProfit, double size,
          Asset asset, double openPrice, QString type, QString position);

    ~Trade();

    QString getTradeID() const;

    double getStopLoss() const;
    void setStopLoss(double stopLoss);

    double getTakeProfit() const;
    void setTakeProfit(double takeProfit);

    double getSize() const;

    Asset getAsset() const;

    double getOpenPrice() const;

    QString getType() const;

    QString getPosition() const;
    void setPosition(const QString &position);

private:
    static int tradeCounter;
    QString tradeID;
    double stopLoss;
    double takeProfit;
    double size;
    Asset asset;
    double openPrice;
    QString type;
    QString position;

    void generateTradeID();  // Example: "Trade_20250526104512345_7"
};

#endif // TRADE_H
