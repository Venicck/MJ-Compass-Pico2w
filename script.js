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
let btRecievedData = ''; // 受信データを一時的に保存する変数
let rxCharacteristic = null; 
let txCharacteristic = null;
function LOG(str, type=0) { // type: 0=info, 1=success, 2=error
    let date = new Date().toLocaleTimeString();
    const logArea = document.getElementById('logger');
    logArea.style.display = 'flex';
    const p = document.createElement('div');
    p.classList.add('item');
    p.classList.add(type === 0 ? 'info' : type === 1 ? 'success' : 'error');
    p.innerHTML = `<div class="icon">${type === 0 ? 'ℹ️' : type === 1 ? '✅' : '❌'}</div><div class="context">[${date}] ${str.replace(/\n/g, '<br>')}</div>`;
    logArea.appendChild(p);
    logArea.scrollTop = logArea.scrollHeight;
    console.log(`[${date}] ` + str);
}

// 要素の表示・非表示を切り替える関数
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

// ---------------------------------
// 基本変数
// ---------------------------------
const names = ["東", "南", "西", "北"];
let scores = [25000, 25000, 25000, 25000];
let reaches = [false, false, false, false];
let sanma = false; // 三麻かどうか
let gamehalf = false; // 東場ならfalse、南場ならtrue
let oya = 0; // 親のプレイヤー番号（0-3）
let honba = 0; // 本場の数
let kyotaku = 0; // 供託の数

function RefreshStats() {
    for (let i = 0; i < 4; i++) {
        document.querySelector(`.stats#st-${i+1} > .box > .dir`).textContent = Whois(i);
        document.querySelector(`.score#sc-${i+1}`).textContent = scores[i];
        if (reaches[i]) {
            document.querySelector(`.score#sc-${i+1}`).classList.add("reach");
        } else {
            document.querySelector(`.score#sc-${i+1}`).classList.remove("reach");
        }
    }
    document.querySelector(".compass > .mid > .box > .text").innerHTML = gamehalf ? "南" : "東" + `${oya+1}局<br>${honba}本場<br>供託 ${kyotaku}`;
    if (sanma) {
        document.querySelector(".compass > .mid > .box > .score#sc-4").style.visibility = "hidden";
        document.querySelector(".compass > .stats#st-4").style.visibility = "hidden";
    } else {
        document.querySelector(".compass > .mid > .box > .score#sc-4").style.visibility = "visible";
        document.querySelector(".compass > .stats#st-4").style.visibility = "visible";
    }
}

function ConfirmReset() {
    Send("RESET");
    Hide(".screen#reset");
}

function Whois(val) {
    if (sanma) {
        return names[(val+oya) % 3];
    } else {
        return names[(val+oya) % 4];
    }
} 

// ----------------------------------
// ロン・ツモの処理
// ----------------------------------

let ronPlayerPay = null;
let ronPlayerGet = null;
let tumoPlayer = null;
let selectedScore = null;

function SelectRon(playerIndex) {
    ronPlayerGet = playerIndex;
    Show("h2#msg", "block");
    Hide(".commands");
    Hide(".stats > .box > .btns");
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
    Show('.screen#ron');
    const grid = document.querySelector('.screen#ron > .btn-grid');
    grid.innerHTML = '';
    let list = ronPlayerGet === oya ? RonListOya : RonList;
    for (let i = 0; i < list.length; i++) {
        const score = list[i];
        const btn = document.createElement('button');
        btn.textContent = score;
        btn.id = `score-${score}`;
        btn.onclick = () => SelectScore(score);
        grid.appendChild(btn);
    }
    document.querySelector(".screen#ron > h2").textContent = `${Whois(ronPlayerPay)}家 → ${Whois(ronPlayerGet)}家へ支払い`;
}

function PayRon() {
    if (selectedScore === null) {
        alert("点数を選んでください");
        return;
    }
    const data = `RON:${ronPlayerGet}<-${ronPlayerPay}_${selectedScore}`;
    Send(data);
    CancelRon();
}

function PayTumo() {
    if (selectedScore === null) {
        alert("点数を選んでください");
        return;
    }
    const data = `TUMO:${tumoPlayer}<-${selectedScore.toString().replace("-", "_")}`;
    Send(data);
    CancelTumo();
}

function CancelRon() {
    selectedScore = null;
    ronPlayerPay = null;
    ronPlayerGet = null;
    for (i = 0; i < 4; i++) {
        let box = document.querySelector(`#st-${i+1} > .box`);
        box.style.cursor = "default";
        box.style.boxShadow = "0 0 10px rgba(255, 255, 255, 0.5)";
        box.onclick = null;
    }
    Hide("h2#msg");
    Hide("button#cancel");
    Show(".stats > .box > .btns", "flex");
    Show(".commands", "flex");
    document.querySelector("h2#msg").textContent = "";
}

function ConfirmTumo(playerIndex) {
    tumoPlayer = playerIndex;
    Show('.screen#tumo');
    document.querySelector(".screen#tumo > h2").textContent = `${Whois(tumoPlayer)}家のツモあがり`;
    const grid = document.querySelector('.screen#tumo > .btn-grid');
    grid.innerHTML = '';
    let list = tumoPlayer === oya ? TumoListOya : TumoList;
    for (let i = 0; i < list.length; i++) {
        const idx = i;
        let sco = null;
        if (tumoPlayer === oya) {
            sco = list[idx];
        } else {
            sco = `${list[idx][0]}-${list[idx][1]}`;
        }
        const btn = document.createElement('button');
        if (tumoPlayer === oya) {
            btn.textContent = sco + " All";
        } else {
            btn.textContent = sco;
        }
        btn.id = `score-${sco}`;
        btn.onclick = () => SelectScore(sco);
        grid.appendChild(btn);
    }
}

function CancelTumo() {
    selectedScore = null;
    tumoPlayer = null;
    Hide('.screen#tumo');
    Show(".stats > .box > .btns", "block");
    Show(".commands", "flex");
}

function SelectScore(score) {
    selectedScore = score;
    let tumoSc = document.querySelector(".screen#tumo");
    let ronSc = document.querySelector(".screen#ron");
    let sc = null;
    if (tumoSc.style.display != "none") sc = tumoSc;
    else sc = ronSc;
    let btns = sc.querySelectorAll(".btn-grid > button");
    btns.forEach(btn => {
        if (btn.id == `score-${score}`) {
            btn.style.backgroundColor = "#008800";
        } else {
            btn.style.backgroundColor = "#222222";
        }
    });
}

// ---------------------------------
// Pico 2w との無線処理
// ---------------------------------
// Bluetoothデバイスへの接続処理
async function Connect() {
    try {
        LOG('デバイスを検索中...\n', 0);
        bluetoothDevice = await navigator.bluetooth.requestDevice({
            filters: [{ services: [SERVICE_UUID] }]
        });

        bluetoothDevice.addEventListener('gattserverdisconnected', onDisconnected);
        await gattConnect();

    } catch (error) {
        LOG('接続失敗: ' + error + '\n', 2);
    }
}

// 受信準備
async function gattConnect() {
    LOG('接続中...\n', 0);
    const server = await bluetoothDevice.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);
    
    rxCharacteristic = await service.getCharacteristic(RX_CHARACTERISTIC_UUID);
    txCharacteristic = await service.getCharacteristic(TX_CHARACTERISTIC_UUID);
    await txCharacteristic.startNotifications();
    txCharacteristic.addEventListener('characteristicvaluechanged', handleDataReceived);

    LOG('JongPass からのデータ受信準備完了', 1);
    Hide(".screen#connection");
}

// 自動再接続
async function onDisconnected(event) {
    const device = event.target;
    LOG('通信が切れました。自動再接続を試みます...\n', 0);
    Show(".screen#connection");
    document.querySelector(".screen#connection > h1").textContent = "再接続しています...";
    let attempts = 0;
    while (!device.gatt.connected && attempts < 3) {
        try {
            LOG('自動再接続を試みています...\n', 0);
            await gattConnect();
            break;
        } catch (error) {
            attempts++;
            await new Promise(resolve => setTimeout(resolve, 2000));
        }
    }
}

// Picoからデータを受信したときの処理
function handleDataReceived(event) {
    let value = event.target.value;
    let decoder = new TextDecoder('utf-8');
    let receivedString = decoder.decode(value);
    btRecievedData += receivedString;
    if (btRecievedData.split("\n").length > 3) {
        LOG('受信: ' + btRecievedData.split("\n").slice(0, -1).join('\n'), 1);
        let str_scores = btRecievedData.split("\n")[0].split("_");
        let str_reaches = btRecievedData.split("\n")[1].split("_");
        let options = btRecievedData.split("\n")[2].split(" ");
        for (let i =0; i < 4; i++) {
            scores[i] = parseInt(str_scores[i]);
            reaches[i] = str_reaches[i] === "T" ? true : false;
        }
        sanma = options[0] == "T" ? true : false;
        gamehalf = options[1] == "T" ? true : false;
        oya = parseInt(options[2]);
        honba = parseInt(options[3]);
        kyotaku = parseInt(options[4]);
        
        RefreshStats();
        btRecievedData = ''; // リセット
    }
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
        
        LOG('送信完了: ' + cleanText + '\n', 1);
    } catch (error) {
        LOG('送信失敗: ' + error + '\n', 2);
    }
};

// ---------------------------------
// デバッグ用の関数
// ---------------------------------
let debugMode = 0; // 5回目のタップでデバッグモードに入る
function OpenDebug() {
    debugMode++;
    if (debugMode >= 5) {
        Show(".debug");
        debugMode = 0;
    }
}

async function DebugSend() {
    const input = document.querySelector(".debug > .debug-input");
    const val = input.value;
    input.value = '';
    await Send(val);
}