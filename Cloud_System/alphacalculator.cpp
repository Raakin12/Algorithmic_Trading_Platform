/* =========================================================================
   AlphaCalculator.cpp – implementation of Alphacalculator.h
   -------------------------------------------------------------------------
   Streaming Jensen’s alpha with a 30‑day sliding window.
   Called by AccountServer whenever a trade closes or a new benchmark
   return is available.
   ========================================================================= */

#include "alphacalculator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <numeric>
#include <cmath>
#include <QtDebug>

// ---------------------------- ctor -------------------------------------
AlphaCalculator::AlphaCalculator(QObject *parent)
    : QObject(parent) {}

// ------------------------ helpers (static) -----------------------------
namespace {
inline double mean(const std::deque<double>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}
inline double variance(const std::deque<double>& x, double mx) {
    double s = 0.0;
    for (double v : x) s += (v - mx) * (v - mx);
    return s / (x.size() - 1);
}
inline double covariance(const std::deque<double>& x,
                         const std::deque<double>& y,
                         double mx, double my) {
    double s = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
        s += (x[i] - mx) * (y[i] - my);
    return s / (x.size() - 1);
}
}

/**
 * Record a realised trade return for user @p u on date @p d.
 * The return is size‑weighted so bigger trades influence alpha more.
 */
void AlphaCalculator::addTrade(int u, const QDate& d,
                               double rp, double w) {
    auto& bucket = buckets[u][d];
    bucket.wRp += w * rp;
    bucket.w   += w;
}

/** Add benchmark return for a given day. When both sides of that day are
 *  known (trade + benchmark) we recompute alpha. */
void AlphaCalculator::addBenchmark(int u, const QDate& d,
                                   double rb, double w) {
    auto& bucket = buckets[u][d];
    bucket.wRb += w * rb;
    bucket.w   += w;

    if (bucket.wRp != 0.0)
        computeAlpha(u);
}

// -----------------------------------------------------------------------
// Internal
// -----------------------------------------------------------------------

// Sliding‑window Jensen’s alpha (ordinary least squares beta).
void AlphaCalculator::computeAlpha(int u) {
    auto& dq = sliding[u];
    dq.clear();

    const auto& dayMap = buckets[u];
    for (auto it = dayMap.cbegin();
         it != dayMap.cend() && dq.size() < BUCKET_WINDOW;
         ++it) {
        const Bucket& b = it.value();
        if (b.w == 0.0) continue; // skip days without both legs
        dq.push_front({ b.wRp / b.w, b.wRb / b.w });
    }

    if (dq.size() < 3) return; // not enough points yet

    std::deque<double> Rp, Rb;
    for (const auto& p : dq) { Rp.push_back(p.rp); Rb.push_back(p.rb); }

    double mp = mean(Rp), mb = mean(Rb);
    double beta = covariance(Rp, Rb, mp, mb) / variance(Rb, mb);
    double alpha = mp - beta * mb;

    emit alphaUpdated(u, alpha);
}

// -----------------------------------------------------------------------
// Rebuild buckets from DB (used on cold start)
// -----------------------------------------------------------------------

void AlphaCalculator::rebuildBucketFromDb(int u, const QDate& d,
                                          QSqlDatabase& db) {
    if (buckets[u].contains(d)) return;

    const QString start = d.startOfDay().toString(Qt::ISODate);
    const QString end   = d.addDays(1).startOfDay().toString(Qt::ISODate);

    QSqlQuery q(db);
    q.prepare("SELECT size, openPrice, closingPrice "
              "FROM \"Trade_History\" "
              "WHERE user_id = :uid AND date >= :s AND date < :e");
    q.bindValue(":uid", u);
    q.bindValue(":s",   start);
    q.bindValue(":e",   end);

    if (!q.exec()) {
        qWarning() << "[AlphaCalc] rebuild SQL error:" << q.lastError();
        return;
    }

    while (q.next()) {
        double size = q.value(0).toDouble();
        double op   = q.value(1).toDouble();
        double cl   = q.value(2).toDouble();

        double rp = (cl - op) / op;        // individual trade return
        double w  = std::abs(size * op);   // notional weight
        addTrade(u, d, rp, w);
    }
}
