// CONFIGURATION
const NODEMCU_IP = "172.20.10.7"; // IP du NodeMCU
const WEBSOCKET_URL = `ws://${NODEMCU_IP}/ws`;

// SEUILS D'ALARME 
const SEUILS = {
    TEMP_EXTERIEUR: 30,     // °C
    LUMINOSITE: 700,        // lux
    TEMP_INTERIEUR: 35,     // °C
    BRUIT: 880,             // dB
    QUALITE_AIR: 1000       // ppm
};

// CONNEXION
let ws = null;
let startTime = new Date();

function connectToNodeMCU() {
    console.log("Tentative de connexion à:", WEBSOCKET_URL);
    ws = new WebSocket(WEBSOCKET_URL);
    
    ws.onopen = function() {
        console.log(" Connecté au NodeMCU");
        updateSystemStatus(true);
        ws.send("getReadings");
    };
    
    ws.onclose = function() {
        console.log(" Déconnecté");
        updateSystemStatus(false);
        setTimeout(connectToNodeMCU, 5000);
    };
    
    ws.onerror = function(error) {
        console.error("Erreur WebSocket:", error);
        updateSystemStatus(false);
    };
    
    ws.onmessage = function(event) {
        try {
            const data = JSON.parse(event.data);
            console.log("Données reçues:", data);
            displaySensorData(data);
        } catch (error) {
            console.error("Erreur parsing JSON:", error);
        }
    };
}

// AFFICHAGE DES DONNÉES AVEC NOUVELLE LOGIQUE D'ALARME
//function displaySensorData(data) {
  //  displayValue('DATA-MEGA-DISTANT-1', data['DATA-MEGA-DISTANT-1'], '°C');
    //displayValue('DATA-MEGA-DISTANT-2', data['DATA-MEGA-DISTANT-2'], 'lux');
 //   displayPressure('DATA-MEGA-DISTANT-3', data['DATA-MEGA-DISTANT-3']);
 //   displayValue('DATA-MEGA-DISTANT-5', data['DATA-MEGA-DISTANT-5'], 'dB');
 //   displayTempHumidity('DATA-MEGA-DISTANT-6', data['DATA-MEGA-DISTANT-6']);
 //   displayValue('DATA-MEGA-DISTANT-7', data['DATA-MEGA-DISTANT-7'], 'ppm');
    
 //   evaluateAlarms(data);
    
  //  updateTimestamps();
    //updateActiveSensorsCount(true);
  //  updateActiveSensorsCountFromData(data);
//}

function displaySensorData(data) {
    if (!data || Object.keys(data).length === 0) {
        console.warn("Données vides reçues, ignorées");
        return;
    }

    displayValue('DATA-MEGA-DISTANT-1', data['DATA-MEGA-DISTANT-1'] ?? '', '°C');
    displayValue('DATA-MEGA-DISTANT-2', data['DATA-MEGA-DISTANT-2'] ?? '', 'lux');
    displayPressure('DATA-MEGA-DISTANT-3', data['DATA-MEGA-DISTANT-3'] ?? '');
    displayValue('DATA-MEGA-DISTANT-5', data['DATA-MEGA-DISTANT-5'] ?? '', 'dB');
    displayTempHumidity('DATA-MEGA-DISTANT-6', data['DATA-MEGA-DISTANT-6'] ?? '');
    displayValue('DATA-MEGA-DISTANT-7', data['DATA-MEGA-DISTANT-7'] ?? '', 'ppm');

    evaluateAlarms(data);
    updateTimestamps();
    updateActiveSensorsCountFromData(data);
}


function evaluateAlarms(data) {
    // Extraire les valeurs numériques
    const tempExt = getNumber(data['DATA-MEGA-DISTANT-1']);
    const luminosite = getNumber(data['DATA-MEGA-DISTANT-2']);
    const bruit = getNumber(data['DATA-MEGA-DISTANT-5']);
    const qualiteAir = getNumber(data['DATA-MEGA-DISTANT-7']);
    
    // Extraire température intérieure
    let tempInt = 0;
    if (data['DATA-MEGA-DISTANT-6'] && data['DATA-MEGA-DISTANT-6'].includes('et')) {
        const parts = data['DATA-MEGA-DISTANT-6'].split('et');
        tempInt = getNumber(parts[0]);
    }
    
    // Évalue alarme extérieure
    const alarmeExtActive = (tempExt > SEUILS.TEMP_EXTERIEUR) || 
                           (luminosite > SEUILS.LUMINOSITE);
    
    // Évalue alarme intérieure
    const alarmeIntActive = (tempInt > SEUILS.TEMP_INTERIEUR) || 
                           (bruit > SEUILS.BRUIT) || 
                           (qualiteAir > SEUILS.QUALITE_AIR);
    
    // Affiche les alarmes
    displayAlarmeConditionnelle('DATA-MEGA-DISTANT-4', alarmeExtActive);
    displayAlarmeConditionnelle('DATA-MEGA-DISTANT-8', alarmeIntActive);
}

function getNumber(value) {
    if (!value) return 0;
    const num = parseFloat(value.toString().replace(/[^\d.-]/g, ''));
    return isNaN(num) ? 0 : num;
}

function displayAlarmeConditionnelle(elementId, estActive) {
    const element = document.getElementById(elementId);
    if (!element) return;
    
    if (estActive) {
        element.innerHTML = '<span class="alarm-status active">Oui</span>';
        const sensorItem = element.closest('.sensor-item');
        if (sensorItem) sensorItem.classList.add('alarm-active');
    } else {
        element.innerHTML = '<span class="alarm-status">Non</span>';
        const sensorItem = element.closest('.sensor-item');
        if (sensorItem) sensorItem.classList.remove('alarm-active');
    }
}

function displayValue(elementId, rawValue, defaultUnit) {
    const element = document.getElementById(elementId);
    if (!element) return;
    
    if (!rawValue || rawValue === '') {
        element.innerHTML = `<span class="value">--</span><span class="unit"> ${defaultUnit}</span>`;
        return;
    }
    
    let value = rawValue;
    let unit = defaultUnit;
    
    if (defaultUnit === '' && rawValue.includes(' ')) {
        const parts = rawValue.split(' ');
        value = parts[0];
        if (parts.length > 1) {
            unit = parts.slice(1).join(' ');
        }
    }
    
    element.innerHTML = `<span class="value">${value}</span><span class="unit"> ${unit}</span>`;
}

function displayPressure(elementId, rawValue) {
    const element = document.getElementById(elementId);
    if (!element) return;
    
    if (!rawValue || rawValue === '') {
        element.innerHTML = `<span class="value">--</span><span class="unit"> hPa</span>`;
        return;
    }
    
    let value = '--';
    let unit = 'hPa';
    
    if (rawValue.includes('Pa')) {
        const numStr = rawValue.replace('Pa', '').trim();
        const numValue = parseFloat(numStr);
        if (!isNaN(numValue)) {
            value = (numValue / 100).toFixed(2);
        }
    } else if (!isNaN(parseFloat(rawValue))) {
        value = (parseFloat(rawValue) / 100).toFixed(2);
    }
    
    element.innerHTML = `<span class="value">${value}</span><span class="unit"> ${unit}</span>`;
}

function displayTempHumidity(elementId, value) {
    const element = document.getElementById(elementId);
    if (!element) return;
    
    if (value && value.includes('et') && value.includes('°C') && value.includes('%')) {
        const parts = value.split('et');
        if (parts.length === 2) {
            const temp = parts[0].replace('°C', '').trim();
            const humidity = parts[1].replace('%', '').trim();
            element.innerHTML = `
                <div class="value-group">
                    <span class="value">${temp}</span>
                    <span class="unit">°C</span>
                </div>
                <div class="value-group">
                    <span class="value">${humidity}</span>
                    <span class="unit">%</span>
                </div>`;
            return;
        }
    }
    
    element.innerHTML = `
        <div class="value-group">
            <span class="value">--</span>
            <span class="unit">°C</span>
        </div>
        <div class="value-group">
            <span class="value">--</span>
            <span class="unit">%</span>
        </div>`;
}

function updateSystemStatus(isConnected) {
    const statusText = document.getElementById('sys-status-text');
    const statusDot = document.getElementById('sys-status-dot');
    if (statusText) statusText.textContent = isConnected ? 'Connecté' : 'Non connecté';
    if (statusDot) statusDot.className = isConnected ? 'status-dot active' : 'status-dot';
    
    const megaStatus = document.getElementById('mega-status');
    const megaStatusText = document.getElementById('mega-status-text');
    if (megaStatus && megaStatusText) {
        megaStatus.className = isConnected ? 'status-indicator active' : 'status-indicator waiting';
        megaStatusText.textContent = isConnected ? 'Connectée' : 'Non connectée';
    }
    
    const nodeStatus = document.getElementById('node-status');
    if (nodeStatus) {
        nodeStatus.innerHTML = isConnected 
            ? '<span class="status-indicator active"></span> Connecté'
            : '<span class="status-indicator waiting"></span> En attente...';
    }
    
    const wsStatus = document.getElementById('ws-status-text');
    const wsIndicator = document.getElementById('ws-status');
    if (wsStatus && wsIndicator) {
        wsStatus.textContent = isConnected ? 'Actif' : 'Inactif';
        wsIndicator.className = isConnected ? 'status-indicator active' : 'status-indicator waiting';
    }
    
    document.querySelectorAll('.section-status .status-dot').forEach(dot => {
        dot.classList.toggle('active', isConnected);
    });
    document.querySelectorAll('.section-status .status-text').forEach(text => {
        text.textContent = isConnected ? 'Actif' : 'Inactif';
    });
    
    if (!isConnected) {
        resetAllValues();
    }
}

function resetAllValues() {
    document.querySelectorAll('.sensor-display .value').forEach(el => {
        if (!el.classList.contains('alarm-status')) {
            el.textContent = '--';
        }
    });
    
    document.querySelectorAll('.alarm-status').forEach(el => {
        if (!el.classList.contains('active')) {
            el.textContent = 'Non';
        }
    });
    
    document.querySelectorAll('.sensor-item').forEach(item => {
        item.classList.remove('alarm-active');
    });
    
    updateActiveSensorsCount(false);
}

//function updateActiveSensorsCount(isConnected) {
  //  const activeSensorsEl = document.getElementById('active-sensors');
    //if (!activeSensorsEl) return;
    
    //if (!isConnected) {
        //activeSensorsEl.textContent = '0/8';
        //return;
    //}
    
   // let activeCount = 0;
   // const sensorIds = [
     //   'DATA-MEGA-DISTANT-1', 'DATA-MEGA-DISTANT-2', 'DATA-MEGA-DISTANT-3', 'DATA-MEGA-DISTANT-4',
       // 'DATA-MEGA-DISTANT-5', 'DATA-MEGA-DISTANT-6', 'DATA-MEGA-DISTANT-7', 'DATA-MEGA-DISTANT-8'
    //];
    
    //sensorIds.forEach(id => {
        //const element = document.getElementById(id);
      //  if (element) {
          //  const valueElement = element.querySelector('.value');
            //if (valueElement && valueElement.textContent !== '--') {
              //  activeCount++;
            //}
        //}
    //});
    
    //activeSensorsEl.textContent = `${activeCount}/8`;
//}

function updateActiveSensorsCountFromData(data) {
    const activeSensorsEl = document.getElementById('active-sensors');
    if (!activeSensorsEl) return;

    let activeCount = 0;
    const sensorKeys = [
        'DATA-MEGA-DISTANT-1',
        'DATA-MEGA-DISTANT-2',
        'DATA-MEGA-DISTANT-3',
        'DATA-MEGA-DISTANT-4',
        'DATA-MEGA-DISTANT-5',
        'DATA-MEGA-DISTANT-6',
        'DATA-MEGA-DISTANT-7',
        'DATA-MEGA-DISTANT-8'
    ];

    sensorKeys.forEach(key => {
        if (data[key] !== undefined && data[key] !== "") {
            activeCount++;
        }
    });

    activeSensorsEl.textContent = `${activeCount}/8`;
}


function updateTimestamps() {
    const now = new Date();
    const timeStr = now.toLocaleTimeString('fr-FR', {
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    });
    
    const ids = ['update-ext', 'update-int', 'last-update-time'];
    ids.forEach(id => {
        const el = document.getElementById(id);
        if (el) el.textContent = timeStr;
    });
}

function updateUptime() {
    const uptimeEl = document.getElementById('uptime');
    if (!uptimeEl) return;
    
    const now = new Date();
    const diff = now - startTime;
    
    const hours = Math.floor(diff / (1000 * 60 * 60));
    const minutes = Math.floor((diff % (1000 * 60 * 60)) / (1000 * 60));
    const seconds = Math.floor((diff % (1000 * 60)) / 1000);
    
    uptimeEl.textContent = 
        `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
}


// INITIALISATION
document.addEventListener('DOMContentLoaded', function() {
    console.log("📡 Initialisation du tableau de bord...");
    console.log("Seuils d'alarme configurés:");
    console.log("- Temp. extérieure >", SEUILS.TEMP_EXTERIEUR, "°C");
    console.log("- Luminosité >", SEUILS.LUMINOSITE, "lux");
    console.log("- Temp. intérieure >", SEUILS.TEMP_INTERIEUR, "°C");
    console.log("- Bruit >", SEUILS.BRUIT, "dB");
    console.log("- Qualité air >", SEUILS.QUALITE_AIR, "ppm");
    
    const dateEl = document.getElementById('current-date');
    if (dateEl) {
        dateEl.textContent = new Date().toLocaleDateString('fr-FR', {
            weekday: 'long',
            year: 'numeric',
            month: 'long',
            day: 'numeric'
        });
    }
    
    setInterval(updateUptime, 1000);
    connectToNodeMCU();
});