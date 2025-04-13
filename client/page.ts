export const mainPage = `
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>MegaLamp Settings</title>
    <style>
        *, :after, :before {
            box-sizing: border-box;
            border: 0 solid #e5e7eb;
        }

        body {
            font-size: 1rem;
            font-family: Arial, sans-serif;
            background-color: #f0f0f0;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 10px;
            box-sizing: border-box;
        }

        .container {
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
            width: 100%;
            max-width: 300px;
            text-align: center;
        }

        h3 {
            color: #333;
            font-size: 1.2em;
            margin: 0 0 15px;
        }

        .flex {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 15px;
        }

        .connection {
            color: red;
        }
        
        .toggle-container {
            margin: 20px 0;
        }

        .toggle-label {
            font-size: 1em;
            color: #555;
        }

        .toggle-switch {
            position: relative;
            display: inline-block;
            width: 50px;
            height: 24px;
        }

        .toggle-switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: 0.4s;
            border-radius: 30px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 20px;
            width: 20px;
            left: 2px;
            bottom: 2px;
            background-color: white;
            transition: 0.4s;
            border-radius: 50%;
        }

        input:checked + .slider {
            background-color: #2196F3;
        }

        input:checked + .slider:before {
            transform: translateX(26px);
        }

        input[type="range"] {
            width: 100%;
            margin: 10px 0;
        }

        .color {
            width: 100%;
            height: 20px;
            background: linear-gradient(to right, red, yellow, green, cyan, blue, magenta, red);
            -webkit-appearance: none;
            appearance: none;
        }

        .color.temperature {
            background: linear-gradient(to right, #ff7f00, #ffd700, #ffffe0, #e0ffff, #add8e6);

        }

        .color::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 26px;
            height: 26px;
            background: white;
            border: 1px solid gray;
            border-radius: 50%;
            cursor: pointer;
        }

        .tabs {
            width: 100%;
            list-style: none;
            padding: 0;
            display: flex;
            justify-content: center;
            margin: 15px 0 0;
        }

        .tabs :first-child {
            border-bottom-left-radius: 0.5em;
            border-top-left-radius: 0.5em;
        }

        .tabs :last-child {
            border-bottom-right-radius: 0.5em;
            border-top-right-radius: 0.5em;
        }

        .tab {
            color: #555;
            cursor: pointer;
            padding: 10px;
            border: 1px solid #ccc;
            background-color: #ffffff;
            transition: background-color 0.3s;
        }

        .tab.active {
            background-color: #2196F3;
            color: white;
        }
    </style>
</head>
<body>
<div class="container">
    <h3>Управление MegaLamp</h3>
    <div id="connection" class='connection'>
        <strong>Нет соединения!</strong>
    </div>
    
    <div>
        <div class="toggle-container flex">
            <label class="toggle-label" for="state">Включение/выключение</label>
            <label class="toggle-switch">
                <input id="state" name="state" type="checkbox">
                <span class="slider"></span>
            </label>
        </div>

        <div class="toggle-container flex">
            <label class="toggle-label" for="lock">Блокировка жестов</label>
            <label class="toggle-switch">
                <input id="lock" name="lock" type="checkbox">
                <span class="slider"></span>
            </label>
        </div>

        <div class="toggle-container">
            <label class="toggle-label" for="brightness">Яркость</label>
            <input id="brightness" name="brightness" type="range" min="0" max="255" step="1">
        </div>

        <div class="toggle-container">
            <label class="toggle-label" for="color">Выберите оттенок:</label>
            <input id="color" class='color' name="color" type="range" min="0" max="255" step="1">
        </div>

        <div class="toggle-container">
            <label class="toggle-label">Режим</label>
            <ul class="tabs">
                <li class="tab" data-mode="0">Цвет</li>
                <li class="tab" data-mode="1">Натуральный</li>
                <li class="tab" data-mode="2">Огонь</li>
            </ul>
        </div>
    </div>
</div>
</body>
<script>
    let stateBright = [0, 0, 0];
    let stateColor = [0, 0, 0];
    let stateMode = 0;
    
    const connection = document.getElementById('connection');
    const brightness = document.getElementById('brightness');
    const color = document.getElementById('color');
    const tabs = document.querySelectorAll('.tab');

    // Получение текущих настроек при загрузке страницы
    fetch('/settings/config')
        .then(response => response.json())
        .then(data => {
            connection.style.display = 'none';
            const {lock, state, mode, bright, value} = data;
            stateBright = bright;
            stateColor = value;
            stateMode = mode;
            document.getElementById('lock').checked = lock;
            document.getElementById('state').checked = state;
            brightness.value = bright[mode];
            color.value = value[mode];
            tabs.forEach(tab => {
                const tabMode = +tab.dataset.mode;
                if (tabMode === mode) {
                    tab.classList.add('active');
                    if (tabMode === 1) {
                        color.classList.add('temperature');
                    }
                }
            });
        })
        .catch(error => console.error('Ошибка при получении состояния лампы:', error));

    // Обработчики событий для обновления настроек
    // Блокировка жестов (существующий обработчик)
    document.getElementById('lock').addEventListener('change', () => {
        const lock = document.getElementById('lock').checked;
        fetch('/settings/lock', {
            method: 'POST',
            body: JSON.stringify({lock}),
            headers: {
                'Content-Type': 'application/json'
            }
        })
            .then(response => response.json())
            .then(data => {
                if (data.status) {
                    const msg = lock ? 'включена' : 'отключена'
                    alert("Блокировка жестов " + msg);
                } else {
                    alert('Ошибка при сохранении настроек');
                }
            })
            .catch(error => console.error('Ошибка при обновлении блокировки:', error));
    });

    // Включение/выключение лампы
    document.getElementById('state').addEventListener('change', () => {
        const state = document.getElementById('state').checked;
        fetch('/settings/state', {
            method: 'POST',
            body: JSON.stringify({state}),
            headers: {
                'Content-Type': 'application/json'
            }
        })
            .then(response => response.json())
            .then(data => {
                if (data.status) {
                    const msg = data.state ? 'включена' : 'выключена'
                    console.log("Лампа " + msg);
                } else {
                    alert('Ошибка при сохранении настроек');
                }
            })
            .catch(error => console.error('Ошибка при обновлении состояния лампы:', error));
    });

    // Яркость
    document.getElementById('brightness').addEventListener('change', () => {
        const brightness = document.getElementById('brightness').value;
        stateBright[stateMode] = +brightness;
        fetch('/settings/bright', {
            method: 'POST',
            body: JSON.stringify({bright: stateBright}),
            headers: {
                'Content-Type': 'application/json'
            }
        })
            .then(response => response.json())
            .then(data => {
                if (data.status) {
                    console.log(\`Яркость изменена\`);
                } else {
                    alert('Ошибка при сохранении настроек');
                }
            })
            .catch(error => console.error('Ошибка при обновлении яркости:', error));
    });

    const slider = document.getElementById('color');
    slider.addEventListener('change', () => {
        const hue = slider.value;
        stateColor[stateMode] = +hue;
        fetch('/settings/color', {
            method: 'POST',
            body: JSON.stringify({color: stateColor}),
            headers: {
                'Content-Type': 'application/json'
            }
        })
            .then(response => response.json())
            .then(data => {
                if (data.status) {
                    console.log(\`Цвет изменен\`);
                } else {
                    alert('Ошибка при сохранении настроек');
                }
            })
            .catch(error => console.error('Ошибка при обновлении цвета:', error));
    });

    // Режим
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const mode = +tab.dataset.mode;
            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');

            brightness.value = stateBright[mode];
            color.value = stateColor[mode];
            stateMode = mode;

            if (mode === 1) {
                color.classList.add('temperature');
            } else {
                color.classList.remove('temperature');
            }

            fetch('/settings/mode', {
                method: 'POST',
                body: JSON.stringify({mode}),
                headers: {
                    'Content-Type': 'application/json'
                }
            })
                .then(response => response.json())
                .then(data => {
                    if (data.status) {
                        console.log(\`Режим установлен\`);
                    } else {
                        alert('Ошибка при сохранении настроек');
                    }
                })
                .catch(error => console.error('Ошибка при обновлении режима:', error));
        });
    });
</script>
</html>
`;