// ─── Elements ───
const addressBar = document.getElementById('address-bar');
const webview = document.getElementById('wv-1');
const aiToggleBtn = document.getElementById('ai-toggle-btn');
const aiSidebar = document.getElementById('ai-sidebar');
const closeAiBtn = document.getElementById('close-ai-btn');
const torToggleInput = document.getElementById('tor-toggle-input');
const backBtn = document.getElementById('back-btn');
const forwardBtn = document.getElementById('forward-btn');
const reloadBtn = document.getElementById('reload-btn');

// ─── Initialization ───
window.addEventListener('DOMContentLoaded', async () => {
    const version = await window.zapAPI.getAppVersion();
    document.getElementById('app-version').innerText = version;
});

// ─── Navigation ───
addressBar.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
        let url = addressBar.value;
        if (!url.startsWith('http://') && !url.startsWith('https://')) {
            if (url.includes('.') && !url.includes(' ')) {
                url = 'https://' + url;
            } else {
                url = 'https://duckduckgo.com/?q=' + encodeURIComponent(url);
            }
        }
        webview.src = url;
        addressBar.blur();
    }
});

backBtn.addEventListener('click', () => {
    if (webview.canGoBack()) webview.goBack();
});

forwardBtn.addEventListener('click', () => {
    if (webview.canGoForward()) webview.goForward();
});

reloadBtn.addEventListener('click', () => {
    webview.reload();
});

webview.addEventListener('did-start-loading', () => {
    document.getElementById('nav-bar').classList.add('loading');
});

webview.addEventListener('did-stop-loading', () => {
    const url = webview.getURL();
    const title = webview.getTitle();
    
    document.getElementById('nav-bar').classList.remove('loading');
    addressBar.value = url;
    
    // Record to local database
    if (url && url !== 'about:blank') {
        window.zapAPI.recordVisit(url, title);
    }
});

// ─── UI Toggles ───
aiToggleBtn.addEventListener('click', () => {
    aiSidebar.classList.toggle('collapsed');
});

closeAiBtn.addEventListener('click', () => {
    aiSidebar.classList.add('collapsed');
});

torToggleInput.addEventListener('change', (e) => {
    const enabled = e.target.checked;
    window.zapAPI.toggleTor(enabled);
    
    const torStatus = document.getElementById('tor-status');
    if (enabled) {
        torStatus.innerText = 'Tor: Initializing...';
        torStatus.style.color = '#5856d6';
    } else {
        torStatus.innerText = 'Tor: Disconnected';
        torStatus.style.color = '';
    }
});

// ─── Placeholder AI Logic ───
const sendAiBtn = document.getElementById('send-ai-btn');
const aiPrompt = document.getElementById('ai-prompt');
const aiChatHistory = document.getElementById('ai-chat-history');

sendAiBtn.addEventListener('click', async () => {
    const text = aiPrompt.value.trim();
    if (!text) return;

    // Add user message
    const userDiv = document.createElement('div');
    userDiv.className = 'ai-message user';
    userDiv.innerText = text;
    aiChatHistory.appendChild(userDiv);
    aiPrompt.value = '';
    aiChatHistory.scrollTop = aiChatHistory.scrollHeight;

    // Show "thinking" state
    const botDiv = document.createElement('div');
    botDiv.className = 'ai-message bot';
    botDiv.innerText = "...";
    aiChatHistory.appendChild(botDiv);

    try {
        const response = await window.zapAPI.askAI(text);
        botDiv.innerText = response;
    } catch (err) {
        botDiv.innerText = "Error communicating with AI.";
    }
    
    aiChatHistory.scrollTop = aiChatHistory.scrollHeight;
});
