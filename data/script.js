// Set up the BPM chart
const bpmCtx = document.getElementById('bpmChart').getContext('2d');
const bpmChart = new Chart(bpmCtx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'BPM (Heart Rate)',
            data: [],
            borderColor: 'rgba(255, 0, 0, 1)',
            backgroundColor: 'rgba(255, 0, 0, 0.2)',
            borderWidth: 2,
            fill: true
        }]
    },
    options: {
        responsive: true,
        scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'BPM' } }
        }
    }
});

// Set up the SpO2 chart
const spo2Ctx = document.getElementById('spo2Chart').getContext('2d');
const spo2Chart = new Chart(spo2Ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'SpO2 (%)',
            data: [],
            borderColor: 'rgba(0, 123, 255, 1)',
            backgroundColor: 'rgba(0, 123, 255, 0.2)',
            borderWidth: 2,
            fill: true
        }]
    },
    options: {
        responsive: true,
        scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'SpO2 (%)' }, min: 0, max: 100 }
        }
    }
});

// Set up the body and weather temperature chart
const tempCtx = document.getElementById('tempChart').getContext('2d');
const tempChart = new Chart(tempCtx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            {
                label: 'Body Temperature (°C)',
                data: [],
                borderColor: 'rgba(255, 99, 132, 1)',
                backgroundColor: 'rgba(255, 99, 132, 0.2)',
                borderWidth: 2,
                fill: true
            },
            {
                label: 'Weather Temperature (°C)',
                data: [],
                borderColor: 'rgba(54, 162, 235, 1)',
                backgroundColor: 'rgba(54, 162, 235, 0.2)',
                borderWidth: 2,
                fill: true
            }
        ]
    },
    options: {
        responsive: true,
        scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'Temperature (°C)' } }
        }
    }
});

// Set up the humidity chart
const humidityCtx = document.getElementById('humidityChart').getContext('2d');
const humidityChart = new Chart(humidityCtx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Humidity (%)',
            data: [],
            borderColor: 'rgba(75, 192, 192, 1)',
            backgroundColor: 'rgba(75, 192, 192, 0.2)',
            borderWidth: 2,
            fill: true
        }]
    },
    options: {
        responsive: true,
        scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'Humidity (%)' } }
        }
    }
});

// Function to fetch data from the ESP32 server
async function fetchSensorData() {
    try {
        const response = await fetch('http://192.168.1.7/data'); // Replace with your ESP32 IP
        const data = await response.json();

        const now = new Date().toLocaleTimeString();

        // Update BPM Chart
        bpmChart.data.labels.push(now);
        bpmChart.data.datasets[0].data.push(data.bpm);
        bpmChart.update();

        // Update SpO2 Chart
        spo2Chart.data.labels.push(now);
        spo2Chart.data.datasets[0].data.push(data.spo2);
        spo2Chart.update();

        // Update Body and Weather Temperature Chart
        tempChart.data.labels.push(now);
        tempChart.data.datasets[0].data.push(data.bodyTemperature);
        tempChart.data.datasets[1].data.push(data.weatherTemperature);
        tempChart.update();

        // Update Humidity Chart
        humidityChart.data.labels.push(now);
        humidityChart.data.datasets[0].data.push(data.humidity);
        humidityChart.update();

        // Update Stats
        document.getElementById('avg-bpm').textContent = `${data.bpm} BPM`;
        document.getElementById('avg-spo2').textContent = `${data.spo2} %`;
        document.getElementById('avg-temp').textContent = `${data.bodyTemperature} °C`;
        document.getElementById('avg-weather-temp').textContent = `${data.weatherTemperature} °C`;
        document.getElementById('avg-humidity').textContent = `${data.humidity} %`;

        // Check for Thresholds
        const thresholdWarning = document.getElementById('warning');
        if (data.warning) {
            thresholdWarning.style.display = 'block';
            thresholdWarning.textContent = `Warning: Threshold Exceeded!`;
        } else {
            thresholdWarning.style.display = 'none';
        }

    } catch (error) {
        console.error("Error fetching data from ESP:", error);
    }
}

// Set an interval to fetch data every 2 seconds
setInterval(fetchSensorData, 2000);
