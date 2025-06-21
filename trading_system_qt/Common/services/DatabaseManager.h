/* =========================================================================
   DatabaseManager.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Singleton wrapper around a persistent PostgreSQL connection.

   Key features
   • Lazy‑init: initialize() returns immediately if the handle is already
     open.
   • One shared QSqlDatabase instance for the entire process, preventing
     duplicate connections and easing transaction scopes across modules.

   Design notes
   • Qt “QPSQL” driver; credentials are hard‑coded for this demo. Replace
     with environment variables or a secrets vault before production.
   • Connection stays open for the lifetime of the app to avoid reconnect
     overhead and to keep prepared‑statement plans cached.
   • TODO: add a liveness probe (simple "SELECT 1") inside initialize()
     after long network stalls.
   ========================================================================= */

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

class DatabaseManager
{
public:
    /** Singleton accessor */
    static DatabaseManager& getInstance() {
        static DatabaseManager instance;  // C++11 thread‑safe init
        return instance;
    }

    /** Shared connection handle */
    QSqlDatabase& getDatabase() {
        return db;
    }

    /**
     * Initialise the database connection.
     * @return true on success or if already open, false on failure.
     */
    bool initialize() {
        QSqlDatabase& db = getInstance().db;
        if (db.isOpen())
            return true;          // fast path

        db.setHostName("trading_postgres");
        db.setDatabaseName("trading_system_db");
        db.setUserName("trading");
        db.setPassword("Zapdos123");   // ⚠ replace before prod

        if (db.open()) {
            qDebug() << "Database connected successfully!";
            return true;
        } else {
            qWarning() << "Database connection failed:" << db.lastError().text();
            return false;
        }
    }

private:
    DatabaseManager() {
        db = QSqlDatabase::addDatabase("QPSQL");
    }
    ~DatabaseManager() {
        if (db.isOpen())
            db.close();
    }

    // Disable copy and assignment.
    DatabaseManager(const DatabaseManager&)            = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase db;
};

#endif // DATABASEMANAGER_H
