document.getElementById('scanForm').addEventListener('submit', function(e) {
    e.preventDefault();
    
    const target = document.getElementById('target').value;
    const level = document.getElementById('level').value;
    
    // Show loading
    document.getElementById('loading').classList.remove('hidden');
    document.getElementById('results').classList.add('hidden');
    
    // Perform scan
    fetch(`/scan?target=${encodeURIComponent(target)}&level=${level}`)
        .then(response => response.json())
        .then(data => {
            // Hide loading
            document.getElementById('loading').classList.add('hidden');
            
            // Display results
            const resultContent = document.getElementById('resultContent');
            resultContent.innerHTML = `
                <div class="result-section">
                    <h3>Scan Information</h3>
                    <p><strong>Target:</strong> ${data.target}</p>
                    <p><strong>IP Address:</strong> ${data.ip}</p>
                    <p><strong>Timestamp:</strong> ${data.timestamp}</p>
                    <p><strong>Scan Level:</strong> ${data.level}</p>
                </div>
                
                <div class="result-section">
                    <h3>Basic Scan Results</h3>
                    <pre>${data.result.basic || 'No data'}</pre>
                </div>
                
                ${data.result.medium ? `
                <div class="result-section">
                    <h3>Medium Scan Results</h3>
                    <pre>${data.result.medium}</pre>
                </div>` : ''}
                
                ${data.result.advanced ? `
                <div class="result-section">
                    <h3>Advanced Scan Results</h3>
                    <pre>${data.result.advanced}</pre>
                </div>` : ''}
                
                ${data.result.expert ? `
                <div class="result-section">
                    <h3>Expert Scan Results</h3>
                    <pre>${data.result.expert}</pre>
                </div>` : ''}
            `;
            
            document.getElementById('results').classList.remove('hidden');
        })
        .catch(error => {
            document.getElementById('loading').classList.add('hidden');
            alert('Scan failed: ' + error.message);
        });
});
// Fetch status and history on load
function fetchStatus() {
    fetch('/status')
        .then(r => r.json())
        .then(s => {
            document.getElementById('statusText').textContent = s.db_ok ? 'DB connected' : 'DB not available';
            document.getElementById('storageInfo').textContent = ' Storage: ' + (s.storage || 'data/');
        })
        .catch(() => {
            document.getElementById('statusText').textContent = 'error';
        });
}

function fetchHistory() {
    fetch('/history')
        .then(r => r.json())
        .then(list => {
            const div = document.getElementById('historyList');
            if (!Array.isArray(list) || list.length === 0) {
                div.textContent = 'No history available.';
                return;
            }
            div.innerHTML = '';
            list.forEach(item => {
                const itemDiv = document.createElement('div');
                itemDiv.className = 'history-item';
                itemDiv.innerHTML = `<strong>${item.target}</strong> (${item.ip}) - ${item.timestamp} - L${item.level} <button data-file='${item.filename}'>View</button>`;
                const btn = itemDiv.querySelector('button');
                btn.addEventListener('click', () => {
                    // open the file in a new tab via /data/ (server must serve data/ if needed)
                    const fname = btn.getAttribute('data-file');
                    if (fname) {
                        window.open('/' + fname, '_blank');
                    }
                });
                div.appendChild(itemDiv);
            });
        })
        .catch(() => {
            document.getElementById('historyList').textContent = 'Failed to load history.';
        });
}

// Run on load
fetchStatus();
fetchHistory();