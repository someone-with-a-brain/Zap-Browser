const { app, BrowserWindow, ipcMain, session } = require('electron');
const path = require('path');
const db = require('./database');
const ai = require('./ollama-service');
const tor = require('./tor-manager');

let mainWindow;

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        title: "ZAP Browser",
        titleBarStyle: 'hiddenInset', // Premium look on macOS
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            nodeIntegration: false,
            webviewTag: true // Needed for the tab system
        },
        backgroundColor: '#0a0a0a' // Dark background for premium feel
    });

    mainWindow.loadFile(path.join(__dirname, '../renderer/index.html'));

    // Open DevTools in development
    // mainWindow.webContents.openDevTools();

    mainWindow.on('closed', () => {
        mainWindow = null;
    });
}

// ─── Privacy Management ──────────────────────────────────────────────────────

function setupPrivacy() {
    // Disable telemetry and noisy features
    session.defaultSession.setPermissionRequestHandler((webContents, permission, callback) => {
        const allowedPermissions = ['notifications', 'fullscreen'];
        if (allowedPermissions.includes(permission)) {
            callback(true);
        } else {
            callback(false);
        }
    });
}

// ─── App Lifecycle ─────────────────────────────────────────────────────────

app.whenReady().then(() => {
    db.initDatabase();
    setupPrivacy();
    createWindow();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) createWindow();
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit();
});

// ─── IPC Handlers ────────────────────────────────────────────────────────────

ipcMain.handle('get-app-version', () => app.getVersion());

ipcMain.on('record-visit', (event, { url, title }) => {
    db.recordVisit(url, title);
});

ipcMain.handle('get-history', (event, limit) => {
    return db.getRecentHistory(limit);
});

ipcMain.on('clear-history', () => {
    db.clearHistory();
});

ipcMain.handle('ask-ai', async (event, prompt) => {
    return await ai.generateResponse(prompt);
});

ipcMain.on('toggle-tor', (event, enabled) => {
    console.log(`[Main] Tor toggle: ${enabled}`);
    if (enabled) {
        tor.enableTorProxy().catch(err => console.error(err));
    } else {
        tor.disableTorProxy().catch(err => console.error(err));
    }
});
