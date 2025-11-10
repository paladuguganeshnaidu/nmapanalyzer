-- init_db.sql: create table for storing scan results
CREATE TABLE IF NOT EXISTS scan_results (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    target TEXT NOT NULL,
    level TEXT,
    result TEXT,
    scanned_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
