#include "Database.h"
#include <stdio.h>

Database::Database() {}
Database::~Database() { Close(); }

bool Database::Initialize(const char* dbPath)
{
    if (m_db) return true;

    int rc = sqlite3_open(dbPath, &m_db);
    if (rc != SQLITE_OK)
    {
        printf("[DB] Failed to open: %s\n", sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    if (!CreateTableIfNotExists())
    {
        Close();
        return false;
    }

    printf("[DB] Opened: %s\n", dbPath);
    return true;
}

void Database::Close()
{
    if (m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
        printf("[DB] Closed\n");
    }
}

bool Database::CreateTableIfNotExists()
{
    const char* sqlCreate =
        "CREATE TABLE IF NOT EXISTS Options ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "OPT1 INTEGER NOT NULL, "
        "OPT2 INTEGER NOT NULL, "
        "OPT3 INTEGER NOT NULL, "
        "SEQ INTEGER NOT NULL, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    char* err = nullptr;
    int rc = sqlite3_exec(m_db, sqlCreate, nullptr, nullptr, &err);
    if (rc != SQLITE_OK)
    {
        printf("[DB] Create table failed: %s\n", err ? err : "unknown");
        if (err) sqlite3_free(err);
        return false;
    }

    // 컬럼 존재 여부 확인
    bool hasSEQ = false;
    const char* sqlInfo = "PRAGMA table_info(Options);";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(m_db, sqlInfo, -1, &stmt, nullptr);
    if (rc == SQLITE_OK && stmt)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char* colName = sqlite3_column_text(stmt, 1); // name
            if (colName && sqlite3_stricmp(reinterpret_cast<const char*>(colName), "SEQ") == 0)
            {
                hasSEQ = true;
                break;
            }
        }
    }
    if (stmt) sqlite3_finalize(stmt);

    if (!hasSEQ)
    {
        // 기존 테이블에 SEQ 컬럼 추가 (NOT NULL이므로 DEFAULT 필요)
        const char* sqlAddSeq = "ALTER TABLE Options ADD COLUMN SEQ INTEGER NOT NULL DEFAULT 0;";
        rc = sqlite3_exec(m_db, sqlAddSeq, nullptr, nullptr, &err);
        if (rc != SQLITE_OK)
        {
            printf("[DB] Add SEQ column failed: %s\n", err ? err : "unknown");
            if (err) sqlite3_free(err);
            return false;
        }

        // 필요 시 인덱스 추가 (실패해도 테이블 사용 가능)
        const char* sqlIdx = "CREATE INDEX IF NOT EXISTS idx_options_seq ON Options(SEQ);";
        rc = sqlite3_exec(m_db, sqlIdx, nullptr, nullptr, &err);
        if (rc != SQLITE_OK)
        {
            printf("[DB] Create index failed: %s\n", err ? err : "unknown");
            if (err) sqlite3_free(err);
        }

        printf("[DB] SEQ column added with DEFAULT 0\n");
    }

    return true;
}

bool Database::SaveOptions(int seq, int opt1, int opt2, int opt3)
{
    if (!m_db) return false;

    const char* sql = "INSERT INTO Options (OPT1, OPT2, OPT3, SEQ) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        printf("[DB] Prepare failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }

    sqlite3_bind_int(stmt, 1, opt1);
    sqlite3_bind_int(stmt, 2, opt2);
    sqlite3_bind_int(stmt, 3, opt3);
    sqlite3_bind_int(stmt, 4, seq); // ← 누락된 바인딩 추가

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        printf("[DB] Insert failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }

    printf("[DB] Saved:[SEQ=%d] OPT1=%d OPT2=%d OPT3=%d\n", seq, opt1, opt2, opt3);
    return true;
}

bool Database::LoadOptions(int& opt1, int& opt2, int& opt3)
{
    if (!m_db) return false;

    const char* sql = "SELECT OPT1, OPT2, OPT3 FROM Options ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        printf("[DB] Prepare failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        opt1 = sqlite3_column_int(stmt, 0);
        opt2 = sqlite3_column_int(stmt, 1);
        opt3 = sqlite3_column_int(stmt, 2);
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}