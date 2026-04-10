const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('zapAPI', {
    getAppVersion: () => ipcRenderer.invoke('get-app-version'),
    toggleTor: (enabled) => ipcRenderer.send('toggle-tor', enabled),
    recordVisit: (url, title) => ipcRenderer.send('record-visit', { url, title }),
    getHistory: (limit) => ipcRenderer.invoke('get-history', limit),
    clearHistory: () => ipcRenderer.send('clear-history'),
    askAI: (prompt) => ipcRenderer.invoke('ask-ai', prompt),
    
    // Add more secure bridges here
});
