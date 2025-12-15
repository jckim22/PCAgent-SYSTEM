#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <tuple>

class Database {
public:
    Database();
    ~Database();

    bool Initialize(const char* dbPath);
    void Close();

    bool SaveOptions(int seq, int opt1, int opt2, int opt3);
    bool LoadOptions(int& opt1, int& opt2, int& opt3);

    bool SaveUrlLog(const char* procName, int pid, const char* method,
        const char* scheme, const char* host, int port,
        const char* path, const char* fullUrl);

    // 브라우저 URL 테이블 생성
    bool CreateBrowserUrlsTable();

    // URL 저장
    bool SaveBrowserUrl(
        const std::wstring& browserName,
        const std::wstring& url,
        const std::wstring& windowTitle
    );

    // 최근 URL 조회 (선택)
    std::vector<std::tuple<std::wstring, std::wstring, std::wstring>> GetRecentUrls(int count = 10);

private:
    sqlite3* m_db;
    bool CreateTableIfNotExists();
    bool CreateUrlLogsTableIfNotExists();
};