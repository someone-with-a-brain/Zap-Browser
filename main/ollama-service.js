const axios = require('axios');

const OLLAMA_URL = 'http://localhost:11434/api/generate';

async function generateResponse(prompt, model = 'llama3') {
    try {
        const response = await axios.post(OLLAMA_URL, {
            model: model,
            prompt: prompt,
            stream: false
        });
        
        return response.data.response;
    } catch (error) {
        console.error('[Ollama] Error:', error.message);
        if (error.code === 'ECONNREFUSED') {
            return "Error: Could not connect to Ollama. Please ensure Ollama is running locally on port 11434.";
        }
        return `Error: ${error.message}`;
    }
}

module.exports = {
    generateResponse
};
