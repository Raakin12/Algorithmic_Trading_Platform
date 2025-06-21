/* =========================================================================
   Trade.cpp – implementation of Trade.h
   -------------------------------------------------------------------------
   Simple data holder for individual trades; only responsibilities are
   ID generation and basic getters/setters. No heavy business logic here –
   performance‑critical operations occur in higher‑level managers.
   ========================================================================= */

#include "trade.h"

// Static counter ensures every auto‑generated ID is unique across the
// process lifetime.
int Trade::tradeCounter = 0;

// Build a unique ID: Trade_<YYYYMMDDhhmmssmmm>_<incrementing counter>
void Trade::generateTradeID(){
    QString timestamp = QDateTime::currentDateTime()
    .toString("yyyyMMddhhmmsszzz");
    tradeID = "Trade_" + timestamp + "_" + QString::number(++tradeCounter);
}

/* -------------------------------------------------------------------------
   Constructor for existing trades (ID supplied by persistence layer).
   ------------------------------------------------------------------------- */
Trade::Trade(QString tradeID, double stopLoss, double takeProfit, double size,
             Asset asset, double openPrice, QString type, QString position)
    : tradeID(tradeID), stopLoss(stopLoss), takeProfit(takeProfit), size(size), asset(asset),
    openPrice(openPrice), type(type), position(position)
{
}

/* -------------------------------------------------------------------------
   Constructor for new trades – auto‑generates a unique ID.
   ------------------------------------------------------------------------- */
Trade::Trade(double stopLoss, double takeProfit, double size,
             Asset asset, double openPrice, QString type, QString position)
    : stopLoss(stopLoss), takeProfit(takeProfit), size(size), asset(asset),
    openPrice(openPrice), type(type), position(position)
{
    generateTradeID();
}

Trade::~Trade() {}

// Basic getters/setters follow – trivial, no extra comments needed.

QString Trade::getTradeID() const {
    return tradeID;
}

double Trade::getStopLoss() const {
    return stopLoss;
}

void Trade::setStopLoss(double stopLoss) {
    this->stopLoss = stopLoss;
}

double Trade::getTakeProfit() const {
    return takeProfit;
}

void Trade::setTakeProfit(double takeProfit) {
    this->takeProfit = takeProfit;
}

double Trade::getSize() const {
    return size;
}

Asset Trade::getAsset() const {
    return asset;
}

double Trade::getOpenPrice() const {
    return openPrice;
}

QString Trade::getType() const {
    return type;
}

QString Trade::getPosition() const {
    return position;
}

void Trade::setPosition(const QString &position) {
    this->position = position;
}
