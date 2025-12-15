#include "Database.h"
#include <windows.h>
#include <stdio.h>
#include <string>

Database::Database() : m_db(nullptr) {}
Database::~Database() { Close(); }

bool Database::Initialize(const char* dbPath) {
    if (m_db) return true;

    int rc = sqlite3_open(dbPath, &m_db);
    if (rc != SQLITE_OK) {
        printf("[DB] Failed to open: %s\n", sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    if (!CreateTableIfNotExists()) {
        Close();
        return false;
    }
    if (!CreateBrowserUrlsTable()) {
        Close();
        return false;
    }

    printf("[DB] Opened: %s\n", dbPath);
    return true;
}

void Database::Close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
        printf("[DB] Closed\n");
    }
}

bool Database::CreateTableIfNotExists() {
    const char* sqlCreate =
        "CREATE TABLE IF NOT EXISTS Options ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " OPT1 INTEGER NOT NULL,"
        " OPT2 INTEGER NOT NULL,"
        " OPT3 INTEGER NOT NULL,"
        " SEQ INTEGER NOT NULL,"
        " timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, sqlCreate, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        printf("[DB] Create table failed: %s\n", err ? err : "unknown");
        if (err) sqlite3_free(err);
        return false;
    }

    bool hasSEQ = false;
    const char* sqlInfo = "PRAGMA table_info(Options);";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(m_db, sqlInfo, -1, &stmt, nullptr);
    if (rc == SQLITE_OK && stmt) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* colName = sqlite3_column_text(stmt, 1);
            if (colName && sqlite3_stricmp(reinterpret_cast<const char*>(colName), "SEQ") == 0) {
                hasSEQ = true;
                break;
            }
        }
    }
    if (stmt) sqlite3_finalize(stmt);

    if (!hasSEQ) {
        const char* sqlAddSeq = "ALTER TABLE Options ADD COLUMN SEQ INTEGER NOT NULL DEFAULT 0;";
        rc = sqlite3_exec(m_db, sqlAddSeq, nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            printf("[DB] Add SEQ column failed: %s\n", err ? err : "unknown");
            if (err) sqlite3_free(err);
            return false;
        }
        const char* sqlIdx = "CREATE INDEX IF NOT EXISTS idx_options_seq ON Options(SEQ);";
        rc = sqlite3_exec(m_db, sqlIdx, nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            printf("[DB] Create index failed: %s\n", err ? err : "unknown");
            if (err) sqlite3_free(err);
        }
        printf("[DB] SEQ column added with DEFAULT 0\n");
    }
    return true;
}

bool Database::CreateUrlLogsTableIfNotExists() {
    const char* sqlCreate =
        "CREATE TABLE IF NOT EXISTS UrlLogs ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " proc_name TEXT NOT NULL,"
        " pid INTEGER NOT NULL,"
        " method TEXT,"
        " scheme TEXT,"
        " host TEXT,"
        " port INTEGER,"
        " path TEXT,"
        " full_url TEXT,"
        " timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, sqlCreate, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        printf("[DB] Create UrlLogs failed: %s\n", err ? err : "unknown");
        if (err) sqlite3_free(err);
        return false;
    }
    const char* sqlIdx = "CREATE INDEX IF NOT EXISTS idx_urllogs_pid_time ON UrlLogs(pid, timestamp);";
    rc = sqlite3_exec(m_db, sqlIdx, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        printf("[DB] Create UrlLogs index failed: %s\n", err ? err : "unknown");
        if (err) sqlite3_free(err);
    }
    return true;
}

bool Database::SaveOptions(int seq, int opt1, int opt2, int opt3) {
    if (!m_db) return false;
    const char* sql = "INSERT INTO Options (OPT1, OPT2, OPT3, SEQ) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        printf("[DB] Prepare failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_int(stmt, 1, opt1);
    sqlite3_bind_int(stmt, 2, opt2);
    sqlite3_bind_int(stmt, 3, opt3);
    sqlite3_bind_int(stmt, 4, seq);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        printf("[DB] Insert failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }
    printf("[DB] Saved:[SEQ=%d] OPT1=%d OPT2=%d OPT3=%d\n", seq, opt1, opt2, opt3);
    return true;
}

bool Database::LoadOptions(int& opt1, int& opt2, int& opt3) {
    if (!m_db) return false;
    const char* sql = "SELECT OPT1, OPT2, OPT3 FROM Options ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        printf("[DB] Prepare failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        opt1 = sqlite3_column_int(stmt, 0);
        opt2 = sqlite3_column_int(stmt, 1);
        opt3 = sqlite3_column_int(stmt, 2);
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}

bool Database::SaveUrlLog(const char* procName, int pid, const char* method,
    const char* scheme, const char* host, int port,
    const char* path, const char* fullUrl) {
    if (!m_db) return false;
    const char* sql =
        "INSERT INTO UrlLogs (proc_name, pid, method, scheme, host, port, path, full_url) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        printf("[DB] Prepare UrlLog failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_text(stmt, 1, procName ? procName : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, pid);
    sqlite3_bind_text(stmt, 3, method ? method : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, scheme ? scheme : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, host ? host : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, port);
    sqlite3_bind_text(stmt, 7, path ? path : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, fullUrl ? fullUrl : "", -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        printf("[DB] Insert UrlLog failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }
    printf("[DB] UrlLog saved: %s\n", fullUrl ? fullUrl : "");
    return true;
}

bool Database::CreateBrowserUrlsTable() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS BrowserUrls ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "browser_name TEXT NOT NULL, "
        "url TEXT NOT NULL, "
        "window_title TEXT, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    char* err = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        printf("[DB] Create BrowserUrls table failed: %s\n", err ? err : "unknown");
        if (err) sqlite3_free(err);
        return false;
    }

    const char* sqlIdx = "CREATE INDEX IF NOT EXISTS idx_urls_timestamp ON BrowserUrls(timestamp);";
    sqlite3_exec(m_db, sqlIdx, nullptr, nullptr, nullptr);

    return true;
}

bool Database::SaveBrowserUrl(
    const std::wstring& browserName,
    const std::wstring& url,
    const std::wstring& windowTitle)
{
    if (!m_db) return false;

    const char* sql =
        "INSERT INTO BrowserUrls (browser_name, url, window_title) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        printf("[DB] Prepare BrowserUrl failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }

    // UTF-16 → UTF-8 변환 (간단히 WideCharToMultiByte 사용)
    auto toUtf8 = [](const std::wstring& ws) -> std::string {
        if (ws.empty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &result[0], len, nullptr, nullptr);
        result.resize(strlen(result.c_str()));
        return result;
        };

    std::string browser = toUtf8(browserName);
    std::string urlUtf8 = toUtf8(url);
    std::string title = toUtf8(windowTitle);

    sqlite3_bind_text(stmt, 1, browser.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, urlUtf8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, title.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        printf("[DB] Insert BrowserUrl failed: %s\n", sqlite3_errmsg(m_db));
        return false;
    }

    return true;
}

std::vector<std::tuple<std::wstring, std::wstring, std::wstring>>
Database::GetRecentUrls(int count) {
    std::vector<std::tuple<std::wstring, std::wstring, std::wstring>> result;
    if (!m_db) return result;

    const char* sql =
        "SELECT browser_name, url, window_title FROM BrowserUrls "
        "ORDER BY timestamp DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    sqlite3_bind_int(stmt, 1, count);

    auto toWide = [](const char* utf8) -> std::wstring {
        if (!utf8) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
        std::wstring ws(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &ws[0], len);
        ws.resize(wcslen(ws.c_str()));
        return ws;
        };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* browser = (const char*)sqlite3_column_text(stmt, 0);
        const char* url = (const char*)sqlite3_column_text(stmt, 1);
        const char* title = (const char*)sqlite3_column_text(stmt, 2);

        result.push_back(std::make_tuple(
            toWide(browser),
            toWide(url),
            toWide(title)
        ));
    }

    sqlite3_finalize(stmt);
    return result;
}