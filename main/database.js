const Database = require('better-sqlite3');
const path = require('path');
const { app } = require('electron');

let db;

function initDatabase() {
    const userDataPath = app.getPath('userData');
    const dbPath = path.join(userDataPath, 'zap_history.db');
    
    db = new Database(dbPath);
    
    // Create tables
    db.exec(`
        CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT NOT NULL,
            title TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        
        CREATE TABLE IF NOT EXISTS bookmarks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT NOT NULL UNIQUE,
            title TEXT,
            added_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    `);
    
    console.log(`[Database] Initialized at ${dbPath}`);
}

function recordVisit(url, title) {
    if (!db) return;
    const stmt = db.prepare('INSERT INTO history (url, title) VALUES (?, ?)');
    stmt.run(url, title);
}

function getRecentHistory(limit = 50) {
    if (!db) return [];
    const stmt = db.prepare('SELECT * FROM history ORDER BY timestamp DESC LIMIT ?');
    return stmt.all(limit);
}

function clearHistory() {
    if (!db) return;
    db.prepare('DELETE FROM history').run();
}

module.exports = {
    initDatabase,
    recordVisit,
    getRecentHistory,
    clearHistory
};
