// Nordic UART Service (NUS) の世界共通のUUID
const SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const RX_CHARACTERISTIC_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // スマホからPicoへ【送信用】
const TX_CHARACTERISTIC_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // Picoからスマホへ【受信用】

const RonList = [1000, 1300, 1600, 2000, 2600, 3200, 3900, 5200, 6400, 8000, 12000, 16000, 24000, 32000];
const RonListOther = [
    [60, 1, 2000], [60, 2, 3900],
    [70, 1, 2300], [70, 2, 4500],
    [80, 1, 2600], [80, 2, 5200],
    [90, 1, 2900], [90, 2, 5800],
    [100, 1, 3200], [100, 2, 6400],
    [110, 2, 7100]
];
const TumoList = [
    [300, 500],   
    [400, 700],   
    [400, 800],   
    [500, 1000],  
    [700, 1300],  
    [800, 1600],  
    [1000, 2000], 
    [1300, 2600], 
    [1600, 3200], 
    [2000, 4000], 
    [3000, 6000], 
    [4000, 8000], 
    [6000, 12000],
    [8000, 16000]
];
const TumoListOther = [
    [60, 1, [1000, 500]], [60, 2, [2000, 1000]],
    [70, 1, [1200, 600]], [70, 2, [2300, 1200]],
    [80, 1, [1300, 700]], [80, 2, [2600, 1300]],
    [90, 1, [1500, 800]], [90, 2, [2900, 1500]],
    [100, 1, [1600, 800]], [100, 2, [3200, 1600]],
    [110, 2, [3600, 1800]]
];
const RonListOya = [1500, 2000, 2400, 2900, 3900, 4800, 5800, 7700, 9600, 12000, 18000, 24000, 36000, 48000];
const RonListOyaOther = [
    [60, 1, 2900], [60, 2, 5800],
    [70, 1, 3400], [70, 2, 6800],
    [80, 1, 3900], [80, 2, 7700],
    [90, 1, 4400], [90, 2, 8700],
    [100, 1, 4800], [100, 2, 9600],
    [110, 2, 10600]
];
const TumoListOya = [500, 700, 800, 1000, 1300, 1600, 2000, 2600, 3200, 4000, 6000, 8000, 12000, 16000];
const TumoListOyaOther = [
    [60, 1, 1000], [60, 2, 2000],
    [70, 1, 1200], [70, 2, 2300],
    [80, 1, 1300], [80, 2, 2600],
    [90, 1, 1500], [90, 2, 2900],
    [100, 1, 1600], [100, 2, 3200],
    [110, 2, 3600]
];


let bluetoothDevice = null;
let rxCharacteristic = null; // 追加
let txCharacteristic = null; // 追加
function LOG(str, color=null) {
    console.log(str);
}

// 全画面の表示・非表示を切り替える関数
function Hide(query) {
    const elements = document.querySelectorAll(query);
    elements.forEach(el => {
        el.style.display = 'none';
    });
}
function Show(query, display='flex') {
    const screens = document.querySelectorAll('.screen');
    screens.forEach(screen => {
        screen.style.display = 'none';
    });
    const elements = document.querySelectorAll(query);
    elements.forEach(el => {
        el.style.display = display;
    });
}

// 和了
let ronPlayerPay = null;
let ronPlayerGet = null;
let tumoPlayer = null;
let selectedScore = null;
function SelectRon(playerIndex) {
    ronPlayerGet = playerIndex;
    Show("h2#msg", "block");
    Hide(".commands");
    Hide(".stats > .box > .btns > button");
    Show("button#cancel", "block");
    document.querySelector("h2#msg").textContent = "支払う人を選んでください";
    document.querySelector("button#cancel").onclick = CancelRon;
    for (i = 0; i < 4; i++) {
        if (i === playerIndex) continue;
        let box = document.querySelector(`#st-${i+1} > .box`);
        box.style.cursor = "pointer";
        box.style.boxShadow = "0 0 10px rgba(207, 0, 0, 0.8)";
        const idx = i;
        box.onclick = () => ConfirmRon(idx);
    }
}
function ConfirmRon(playerIndex) {
    for (i = 0; i < 4; i++) {
        let box = document.querySelector(`#st-${i+1} > .box`);
        box.style.cursor = "default";
        box.style.boxShadow = "0 0 10px rgba(255, 255, 255, 0.5)";
        box.onclick = null;
    }

    ronPlayerPay = playerIndex;
    document.querySelector("h2#msg").textContent = "";
    Hide("h2#msg");
    Hide("button#cancel");
    Show(".commands", "flex");
    ShowRon();
    document.querySelector(".screen#ron > h2").textContent = `P${ronPlayerPay} → P${ronPlayerGet}`;
}

function CancelRon() {
    for (i = 0; i < 4; i++) {
        let box = document.querySelector(`#st-${i+1} > .box`);
        box.style.cursor = "default";
        box.style.boxShadow = "0 0 10px rgba(255, 255, 255, 0.5)";
        box.onclick = null;
    }
    Hide("h2#msg");
    Hide("button#cancel");
    Show(".stats > .box > .btns > button", "block");
    Show(".commands", "flex");
    document.querySelector("h2#msg").textContent = "";
}

function CancelTumo() {
    Hide('.screen#tumo');
    Show(".stats > .box > .btns > button", "block");
    Show(".commands", "flex");
}

function SelectScore(score) {
    selectedScore = score;
    const screen = document.querySelector('.screen[style="display: flex;"]');
    let btns = screen.querySelectorAll(".btn-grid > button");
    btns.forEach(btn => {
        if (btn.id == `score-${score}`) {
            btn.style.backgroundColor = "#008800";
        }   else {
            btn.style.backgroundColor = "#222222";
        }
    });
}

function ShowRon() {
    Show('.screen#ron');
    const grid = document.querySelector('.screen#ron > .btn-grid');
    grid.innerHTML = '';
    let list = ronPlayerPay === 0 ? RonListOya : RonList;
    for (let i = 0; i < list.length; i++) {
        const score = list[i];
        const btn = document.createElement('button');
        btn.textContent = score;
        btn.id = `score-${score}`;
        btn.onclick = () => SelectScore(score);
        grid.appendChild(btn);
    }
}

function ShowTumo(playerIndex) {
    tumoPlayer = playerIndex;
    Show('.screen#tumo');
    const grid = document.querySelector('.screen#tumo > .btn-grid');
    grid.innerHTML = '';
    let list = tumoPlayer === 0 ? TumoListOya : TumoList;
    for (let i = 0; i < list.length; i++) {
        if (tumoPlayer === 0) {
            var score = `${list[i]} All`;
        } else {
            var score = `${list[i][0]}-${list[i][1]}`;
        }
        const btn = document.createElement('button');
        btn.textContent = score;
        btn.id = `score-${score}`;
        btn.onclick = () => SelectScore(score);
        grid.appendChild(btn);
    }
}

// ---------------------------------
// Pico 2w との無線処理
// ---------------------------------
// 1. Bluetoothデバイスへの接続処理
async function Connect() {
    try {
        LOG('デバイスを検索中...\n');
        bluetoothDevice = await navigator.bluetooth.requestDevice({
            filters: [{ services: [SERVICE_UUID] }]
        });

        bluetoothDevice.addEventListener('gattserverdisconnected', onDisconnected);
        await gattConnect();

    } catch (error) {
        LOG('接続失敗: ' + error + '\n');
    }
}

// 内部的な接続・通知（Notification）の開始処理
async function gattConnect() {
    LOG('接続中...\n');
    const server = await bluetoothDevice.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);
    
    // 【修正】送信用と受信用の特性を別々に取得
    rxCharacteristic = await service.getCharacteristic(RX_CHARACTERISTIC_UUID);
    txCharacteristic = await service.getCharacteristic(TX_CHARACTERISTIC_UUID);

    // 受信用(TX)に対して通知を開始
    await txCharacteristic.startNotifications();
    txCharacteristic.addEventListener('characteristicvaluechanged', handleDataReceived);

    LOG('JongPass との同期が完了しました！\n');
    document.getElementById('send-data').disabled = false;
}

// 自動再接続
async function onDisconnected(event) {
    const device = event.target;
    LOG('通信が切れました。自動再接続を試みます...\n');
    document.getElementById('send-data').disabled = true;

    while (!device.gatt.connected) {
        try {
            LOG('自動再接続を試みています...\n');
            await gattConnect();
            break;
        } catch (error) {
            await new Promise(resolve => setTimeout(resolve, 2000));
        }
    }
}

// Picoからデータを受信したときの処理
function handleDataReceived(event) {
    let value = event.target.value;
    let decoder = new TextDecoder('utf-8');
    let receivedString = decoder.decode(value);
    
    LOG('受信: ' + receivedString + '\n');
}

// 2. データの送信処理
async function Send(value) {
    if (!rxCharacteristic) return;

    try {
        // 文字列の前後の余計な空白を排除
        const cleanText = value.trim(); 
        
        if (cleanText === "") return;

        const encoder = new TextEncoder();
        
        await rxCharacteristic.writeValueWithResponse(encoder.encode(cleanText));
        
        LOG('送信完了: ' + cleanText + '\n');
        textInput.value = '';
    } catch (error) {
        LOG('送信失敗: ' + error + '\n');
    }
};