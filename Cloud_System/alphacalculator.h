/* =========================================================================
   AlphaCalculator.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Per‑user Jensen’s α calculator that runs entirely in memory.

   Key features
   • Stream‑oriented: call addTrade() when a trade closes and addBenchmark()
     when the daily market return arrives. Once both numbers for a date are
     present, the class emits alphaUpdated(userID, alpha).
   • Multi‑tenant: maintains isolated windows for every user ID.
   • Hot cache: avoids DB round‑trips during trading; supports
     rebuildBucketFromDb() to reload state after a restart.

   Design notes
   • Maintains Σ(weight·Rp) / Σ(weight) and Σ(weight·Rb) / Σ(weight) per day
     so updates are O(1).
   • sliding[user] keeps the last BUCKET_WINDOW pairs in a deque, allowing
     constant‑time expiry of stale data without scanning full maps.
   • α is computed as  mean(Rp) − β·mean(Rb), where β comes from ordinary
     least‑squares on the same window.
   • TODO: snapshot the aggregated window to disk to cut warm‑up time on
     server reboot.
   ========================================================================= */

#ifndef ALPHACALCULATOR_H
#define ALPHACALCULATOR_H

#include <QObject>
#include <QDate>
#include <QMap>
#include <deque>
#include <QtSql/QSqlDatabase>

constexpr int BUCKET_WINDOW = 30;   // length of the sliding window (days)

struct Bucket {
    double wRp = 0.0;   // Σ(weight · portfolio return)
    double wRb = 0.0;   // Σ(weight · benchmark return)
    double w   = 0.0;   // Σ(weight) – denominator for weighted means
};

class AlphaCalculator : public QObject
{
    Q_OBJECT
public:
    explicit AlphaCalculator(QObject *parent = nullptr);

    // Adds a realised trade return for a given day
    void addTrade(int    userID,
                  const  QDate &day,
                  double retPortfolio,
                  double weight);

    // Adds benchmark return for a given day (e.g., S&P 500 daily)
    void addBenchmark(int    userID,
                      const  QDate &day,
                      double retBenchmark,
                      double weight);

    // Rebuilds the in‑memory buckets from historical records, used at start‑up
    void rebuildBucketFromDb(int userID,
                             const QDate &fromDay,
                             QSqlDatabase &db);

signals:
    void alphaUpdated(int userID, double alpha);

private:
    void computeAlpha(int userID);

    QMap<int, QMap<QDate, Bucket>> buckets;   // per‑user day→bucket map
    struct Pair { double rp; double rb; };
    QMap<int, std::deque<Pair>> sliding;      // per‑user 30‑day sliding window
};

#endif // ALPHACALCULATOR_H
