/* =========================================================================
   TradeHistory.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Lightweight value-object that represents a single historical trade.
     • Plain getters/setters so QML (or table models) can bind easily.
     • No behaviour beyond storing fields; all calculations are done
       elsewhere.
   ========================================================================= */

#ifndef TRADEHISTORY_H
#define TRADEHISTORY_H

#include <QString>
#include <QDate>
#include "asset.h"

class TradeHistory
{
public:
    TradeHistory() = default;

    /* simple getters / setters */
    QString getTradeID()   const { return tradeID; }
    void    setTradeID(const QString &tid) { tradeID = tid; }

    int     getUserID()    const { return userID; }
    void    setUserID(int uid)   { userID = uid; }

    double  getSize()      const { return size; }
    void    setSize(double s)    { size = s; }

    Asset   getAsset()     const { return asset; }
    void    setAsset(Asset a)    { asset = a; }

    double  getOpenPrice() const { return openPrice; }
    void    setOpenPrice(double op) { openPrice = op; }

    double  getClosingPrice() const { return closingPrice; }
    void    setClosingPrice(double cp) { closingPrice = cp; }

    double  getPnl()       const { return pnl; }
    void    setPnl(double p)    { pnl = p; }

    QDate   getDate()      const { return date; }
    void    setDate(const QDate &d) { date = d; }

private:
    /* stored fields */
    QString tradeID;

    int     userID          = 0;
    double  size            = 0.0;
    Asset   asset           = BTCUSDT;
    double  openPrice       = 0.0;
    double  closingPrice    = 0.0;
    double  pnl             = 0.0;
    QDate   date;
};

#endif // TRADEHISTORY_H
