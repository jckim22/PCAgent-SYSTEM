#pragma once
#include "sqlite3.h"

class Database
{
public:
    Database();
    ~Database();

    bool Initialize(const char* dbPath);
    void Close();

    bool SaveOptions(int seq, int opt1, int opt2, int opt3);
    bool LoadOptions(int& opt1, int& opt2, int& opt3);

private:
    sqlite3* m_db = nullptr;
    bool CreateTableIfNotExists();
};