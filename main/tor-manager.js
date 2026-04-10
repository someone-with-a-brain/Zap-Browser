const { session } = require('electron');

async function enableTorProxy() {
    // Tor defaults to 127.0.0.1:9050 for SOCKS5
    const proxyConfig = {
        proxyRules: 'socks5://127.0.0.1:9050',
        proxyBypassRules: 'localhost, 127.0.0.1'
    };
    
    await session.defaultSession.setProxy(proxyConfig);
    console.log('[Tor] Proxy enabled: socks5://127.0.0.1:9050');
}

async function disableTorProxy() {
    await session.defaultSession.setProxy({ proxyRules: '' });
    console.log('[Tor] Proxy disabled');
}

module.exports = {
    enableTorProxy,
    disableTorProxy
};
