#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int GREEN_LED = 26;
const char* ssid = "SRWC-ESP32";
const char* password = "SRWC12345";

WebServer server(80);

const size_t BLOCK_SIZE = 16 * 1024;
String testPayload;

// 0.91-inch OLED test configuration (commonly SSD1306, 128x32).
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// Rolling RSSI history for the 128-pixel-wide OLED graph.
int rssiHistory[128];
int rssiCount = 0;
unsigned long lastOledUpdate = 0;

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SRWC Performance Analyzer</title>
<style>
:root{--navy:#10263e;--blue:#1677ff;--green:#16b85f;--red:#ef3333;--purple:#7c3aed;--ink:#10213b;--muted:#64748b;--line:#e4ebf4;--card:#fff}
*{box-sizing:border-box}
body{margin:0;background:#f0f5fb;color:var(--ink);font-family:Inter,Segoe UI,Arial,sans-serif;font-size:14px}
header{min-height:82px;padding:15px 24px;background:linear-gradient(110deg,#10243a,#203c59);color:white;display:flex;align-items:center;justify-content:space-between;gap:18px}
.brand{display:flex;align-items:center;gap:16px}.wifi-icon{font-size:42px;color:#4095ff;line-height:1}
h1{font-size:25px;margin:0 0 5px;letter-spacing:-.4px}header p{margin:0;color:#d9e6f4;font-size:16px}
.connect-pill{background:#ffffff12;border:1px solid #ffffff14;padding:12px 16px;border-radius:13px;min-width:210px}
.connect-pill strong{font-size:15px}.connect-pill small{display:block;margin-top:5px;color:#d9e6f4}
.dot{display:inline-block;width:11px;height:11px;border-radius:50%;background:#19d879;margin-right:9px}
main{max-width:1800px;margin:auto;padding:14px 18px}
.layout{display:grid;grid-template-columns:minmax(250px,1fr) minmax(420px,1.85fr) minmax(280px,1.12fr);gap:14px;align-items:stretch}
.col{display:flex;flex-direction:column;gap:14px;min-width:0}
.card{background:var(--card);border:1px solid #e8eef6;border-radius:15px;padding:16px 18px;box-shadow:0 3px 12px #152e4a08;min-width:0}
.card h2{font-size:18px;margin:0 0 12px;letter-spacing:-.25px;display:flex;align-items:center;gap:10px}
.card h3{font-size:16px;margin:0 0 12px}
.icon{font-size:22px;color:var(--blue);width:25px;text-align:center;flex-shrink:0}
.sub{color:var(--muted);font-size:12px;line-height:1.45}
.connection{display:flex;align-items:center;gap:12px;padding:5px 0}
.big-dot{height:18px;width:18px;border-radius:50%;background:#18c66a;box-shadow:0 0 0 5px #18c66a18}
.connection b{font-size:16px}.connection small{display:block;margin-top:5px;color:#475d78}
.buttons{display:flex;gap:8px;flex-wrap:wrap}
button{border:0;border-radius:9px;padding:10px 13px;background:var(--blue);color:white;font-weight:650;cursor:pointer;font-size:13px;min-height:38px}
button:disabled{opacity:.45;cursor:not-allowed}
button.green{background:#17b965}button.red{background:#ef3333}button.gray{background:#53677f}
.led-buttons button{flex:1;min-width:100px;min-height:48px}
.metric-list{display:flex;flex-direction:column}
.metric-row{display:grid;grid-template-columns:28px 1fr auto;align-items:center;gap:9px;padding:10px 0;border-top:1px solid var(--line)}
.metric-row:first-child{border-top:0}.metric-icon{font-size:20px;color:#5b7895}
.metric-label{font-size:13px;color:#435b78}.metric-value{font-size:20px;font-weight:750;margin-top:3px;letter-spacing:-.3px;overflow-wrap:anywhere}
.edit-distance{width:72px;padding:7px;border:1px solid #d8e1ec;border-radius:7px;font-size:13px}
.hero-card{padding:16px 18px}
.hero-top{display:flex;justify-content:space-between;gap:10px;align-items:flex-start}
.hero-top h2{margin-bottom:4px}.hero-top p{margin:0;color:#526984}
.transfer-visual{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:13px 2px 15px;min-height:142px}
.device{text-align:center;font-weight:650;font-size:12px;min-width:90px}
.device-art{font-size:54px;line-height:1.15;filter:drop-shadow(0 5px 6px #0b2d4a22)}
.transfer-path{flex:1;display:flex;align-items:center;justify-content:center;gap:10px;color:#1599ff;min-width:40px}
.transfer-path .dash{height:3px;flex:1;min-width:15px;background:repeating-linear-gradient(90deg,#5eb8ff 0 5px,transparent 5px 12px)}
.file-chip{font-size:19px;border-radius:5px;padding:4px 6px;background:#e8f5ff;color:#1677ff}
.progress-box{padding:13px 14px;background:#f1f7ff;border:1px solid #e5f0ff;border-radius:11px}
.progress-meta{display:flex;justify-content:space-between;gap:12px;margin-bottom:10px;font-weight:650;color:#155fd0}
.progress-meta span:last-child{color:#405c7c;font-weight:500}
.progress-track{height:13px;border-radius:9px;background:#dce9f9;overflow:hidden}
#bar{height:100%;width:0;background:linear-gradient(90deg,#1677ff,#19b9e8);transition:width .15s}
.progress-bottom{display:flex;justify-content:space-between;gap:10px;margin-top:8px;font-size:13px}
.progress-bottom b{font-size:15px}
.distance-number{font-size:37px;font-weight:800;text-align:center;margin:8px 0 16px}
.scale{height:15px;border-radius:20px;background:linear-gradient(90deg,#1cc66a,#e3d943,#ff8a18);position:relative;margin:0 2px 8px}
.scale-marker{position:absolute;top:50%;left:0;transform:translate(-50%,-50%);width:24px;height:24px;border:4px solid white;background:#20bc69;box-shadow:0 0 0 2px #91b7d8;border-radius:50%;transition:left .25s}
.scale-labels{display:flex;justify-content:space-between;color:#344b67;font-size:11px}
.info{background:#eff6ff;border-radius:10px;padding:11px 12px;margin-top:13px;color:#3c536e;font-size:12px;line-height:1.5}
.info b{color:#1d65bf}
.graph-grid{display:grid;grid-template-columns:1fr 1fr;gap:14px}
.graph-card{padding:14px 15px 10px}
.graph-title{display:flex;align-items:center;justify-content:space-between;gap:8px;margin-bottom:5px}
.graph-title h2{font-size:16px;margin:0}
.graph-value{font-size:21px;font-weight:800;white-space:nowrap}
.graph-value.green-text{color:#13b35a}.graph-value.purple-text{color:#7738e8}.graph-value.red-text{color:#ef3333}
canvas{width:100%;height:145px;display:block}
.chart-caption{font-size:11px;color:#657991;text-align:center;margin-top:2px}
.stats-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.stat{background:#f3f7fc;border-radius:10px;text-align:center;padding:12px 5px}
.stat span{display:block;font-size:12px;color:#405775}.stat b{display:block;font-size:23px;margin-top:5px}
.good{color:#13ad59}.bad{color:#ef3333}.blue-text{color:#1677ff}
.packet-strip{display:flex;gap:4px;margin-top:13px;overflow:hidden}
.packet{height:19px;flex:1;min-width:4px;border-radius:2px;background:#18bd65}
.packet.lost{background:#ef4444}.packet.retry{background:#b4c4d3}
.bottom-grid{display:grid;grid-template-columns:1fr 1fr;gap:14px;margin-top:14px}
.table-wrap{overflow-x:auto}
table{width:100%;border-collapse:collapse;white-space:nowrap;font-size:12px}
th,td{text-align:left;padding:9px 8px;border-bottom:1px solid var(--line)}
th{color:#455a73;background:#f8fafd}
.hidden{display:none}
@media(max-width:1180px){.layout{grid-template-columns:minmax(230px,.9fr) minmax(380px,1.4fr)}.right-col{grid-column:1/-1;display:grid;grid-template-columns:1fr 1fr}.right-col .card{height:100%}}
@media(max-width:760px){header{padding:14px;align-items:flex-start}.brand{gap:9px}.wifi-icon{font-size:30px}h1{font-size:19px}header p{font-size:12px}.connect-pill{min-width:0;padding:8px;font-size:12px}.connect-pill small{font-size:10px}.layout{grid-template-columns:1fr}.right-col{grid-column:auto;display:flex}.graph-grid,.bottom-grid{grid-template-columns:1fr}main{padding:10px}.transfer-visual{min-height:110px}.device-art{font-size:40px}.card{padding:14px}.metric-value{font-size:18px}}
@media(min-width:1400px) and (min-height:850px){canvas{height:160px}.card{padding:17px 19px}.col{gap:12px}.layout{gap:12px}.graph-grid{gap:12px}}
</style>
</head>
<body>
<header>
  <div class="brand">
    <div class="wifi-icon">◔</div>
    <div>
      <h1>SRWC Performance Analyzer</h1>
      <p>Live Wireless Communication with ESP32</p>
    </div>
  </div>
  <div class="connect-pill">
    <strong><span class="dot" id="headerDot"></span><span id="headerStatus">Connecting</span></strong>
    <small>ESP32-SRWC (192.168.4.1)</small>
  </div>
</header>

<main>
<div class="layout">

  <div class="col left-col">
    <section class="card">
      <h2>Connection Status</h2>
      <div class="connection">
        <span class="big-dot" id="connectionDot"></span>
        <div><b id="connectionTitle">Checking ESP32...</b><small id="connectionIP">IP: 192.168.4.1</small></div>
      </div>
    </section>

    <section class="card">
      <h2><span class="icon">☼</span>Wireless LED Control</h2>
      <div class="buttons led-buttons">
        <button class="green" onclick="setLed('on')">⏻ LED ON</button>
        <button class="red" onclick="setLed('off')">⏻ LED OFF</button>
      </div>
      <p class="sub" id="ledStatus">LED control ready</p>
    </section>

    <section class="card">
      <h2>Real-Time Metrics</h2>
      <div class="metric-list">
        <div class="metric-row">
          <div class="metric-icon">↗</div><div><div class="metric-label">Distance (m)</div><div class="metric-value" id="metricDistance">1.0 m</div></div>
          <input class="edit-distance" id="distance" type="number" min="0" max="1000" step="0.5" value="1" aria-label="Measured distance in metres">
        </div>
        <div class="metric-row">
          <div class="metric-icon">◴</div><div><div class="metric-label">Throughput (Mbps)</div><div class="metric-value" id="metricRate">--</div></div>
        </div>
        <div class="metric-row">
          <div class="metric-icon">◷</div><div><div class="metric-label">Latency (ms)</div><div class="metric-value" id="metricLatency">--</div></div>
        </div>
        <div class="metric-row">
          <div class="metric-icon">⌁</div><div><div class="metric-label">Signal Strength (RSSI)</div><div class="metric-value" id="metricRssi">-- dBm</div></div>
        </div>
        <div class="metric-row">
          <div class="metric-icon">▤</div><div><div class="metric-label">Data Blocks Received</div><div class="metric-value" id="metricPackets">0</div></div>
        </div>
        <div class="metric-row">
          <div class="metric-icon">✖</div><div><div class="metric-label">Failed Requests</div><div class="metric-value" id="metricLost">0</div></div>
        </div>
      </div>
    </section>
  </div>

  <div class="col center-col">
    <section class="card hero-card">
      <div class="hero-top">
        <div>
          <h2><span class="icon">◉</span>Live Data Transfer Simulation</h2>
          <p>Real-time data transfer between ESP32 and laptop</p>
        </div>
        <button id="stop" class="red" onclick="stopTest()" disabled>■ Stop</button>
      </div>

      <div class="transfer-visual">
        <div class="device"><div class="device-art">💻</div>Laptop (Client)</div>
        <div class="transfer-path"><span class="dash"></span><span class="file-chip">▤</span><span class="dash"></span><span class="file-chip">▤</span><span class="dash"></span></div>
        <div class="device"><div class="device-art">📡</div>ESP32 (Access Point)</div>
      </div>

      <div class="progress-box">
        <div class="progress-meta"><span id="transferStatus">Ready to start a 100-block trial</span><span id="progressText">0 / 100 blocks</span></div>
        <div class="progress-track"><div id="bar"></div></div>
        <div class="progress-bottom"><span><b id="trialMB">0</b> / 1.56 MiB</span><b id="progressRate">-- Mbps</b></div>
      </div>
      <div class="buttons" style="margin-top:10px">
        <button id="start" onclick="startTest()">▶ Start 100-Block Trial</button>
        <span class="sub" style="align-self:center">100 successful × 16 KiB blocks per trial</span>
      </div>
    </section>

    <div class="graph-grid">
      <section class="card graph-card">
        <div class="graph-title"><h2><span class="icon">⌁</span>Live Throughput</h2><div class="graph-value green-text" id="graphRate">-- Mbps</div></div>
        <canvas id="rateChart" width="520" height="190"></canvas>
        <div class="chart-caption">Recent samples · Mbps · auto-scaled Y-axis</div>
      </section>
      <section class="card graph-card">
        <div class="graph-title"><h2><span class="icon">◷</span>Live Latency</h2><div class="graph-value purple-text" id="graphLatency">-- ms</div></div>
        <canvas id="latencyChart" width="520" height="190"></canvas>
        <div class="chart-caption">Request duration · milliseconds · auto-scaled Y-axis</div>
      </section>
      <section class="card graph-card">
        <div class="graph-title"><h2><span class="icon">⌁</span>Live Signal Strength (RSSI)</h2><div class="graph-value red-text" id="graphRssi">-- dBm</div></div>
        <canvas id="rssiChart" width="520" height="190"></canvas>
        <div class="chart-caption">Signal received at ESP32 · dBm · sensitive auto-scale</div>
      </section>
      <section class="card graph-card" style="grid-column:1/-1">
        <div class="graph-title">
          <h2><span class="icon">〰</span>Antenna Gain vs Frequency</h2>
          <div class="graph-value blue-text">2.400–2.4835 GHz</div>
        </div>
        <canvas id="gainFrequencyChart" width="1040" height="250" aria-label="Illustrative antenna gain versus frequency chart"></canvas>
        <div class="chart-caption">Full 2.4 GHz Wi-Fi band · gain shown in dBi · illustrative curve only — not measured or specific to this ESP32 antenna.</div>
        <p class="sub">The current hardware/firmware reports RSSI and data-transfer performance; it does not measure antenna gain or sweep RF frequency. This graph uses synthetic example values to show the full band and must not be treated as experimental results.</p>
      </section>
      <section class="card graph-card">
        <div class="graph-title"><h2><span class="icon">▤</span>Packet Statistics (Live)</h2></div>
        <div class="stats-grid">
          <div class="stat"><span>Blocks Received</span><b class="good" id="statReceived">0</b></div>
          <div class="stat"><span>Failed Requests</span><b class="bad" id="statLost">0</b></div>
          <div class="stat"><span>Failure Rate</span><b class="blue-text" id="statLossRate">0%</b></div>
        </div>
        <div class="packet-strip" id="packetStrip"></div>
        <p class="sub">Red marks indicate failed requests; green marks indicate successful blocks.</p>
      </section>
    </div>
  </div>

  <div class="col right-col">
    <section class="card">
      <h2><span class="icon">⌖</span>Live Distance Estimation</h2>
      <p class="sub">Rough estimate based on Wi-Fi RSSI</p>
      <div class="distance-number" id="estimatedDistance">-- m</div>
      <div class="scale"><div class="scale-marker" id="scaleMarker"></div></div>
      <div class="scale-labels"><span>0</span><span>2</span><span>5</span><span>10</span><span>15+ m</span></div>
      <div class="info"><b>ⓘ Distance is approximate.</b><br>RSSI is affected by antenna direction, obstacles and interference. Enter a tape-measured distance before each recorded trial.</div>
    </section>

    <section class="card">
      <h2><span class="icon">◎</span>Trial Progress</h2>
      <div class="metric-value"><span id="trialCountLarge">0</span> / 100 blocks</div>
      <div class="progress-track" style="margin:12px 0"><div id="barSmall" style="height:100%;width:0;background:#17b965"></div></div>
      <p class="sub">Completed trials are added to the results table automatically. The counter resets when a new trial starts.</p>
      <p>Current trial distance: <b id="currentDistance">1.0 m</b></p>
      <p>Trial throughput: <b id="trialThroughput">-- Mbps</b></p>
    </section>
  </div>

</div>

<div class="bottom-grid">
  <section class="card">
    <h2>Recorded Distance Trials</h2>
    <p class="sub">Automatically recorded after 100 successful data blocks.</p>
    <div class="buttons" style="margin-bottom:10px">
      <button onclick="exportCSV()">Export CSV</button>
      <button class="red" onclick="clearTrials()">Clear Recorded Trials</button>
    </div>
    <div class="table-wrap">
      <table>
        <thead><tr><th>Trial</th><th>Distance (m)</th><th>Throughput (Mbps)</th><th>Avg. Latency (ms)</th><th>Avg. RSSI (dBm)</th><th>Failed</th></tr></thead>
        <tbody id="trials"></tbody>
      </table>
    </div>
  </section>
  <section class="card">
    <h2>Experiment Notes</h2>
    <p class="sub">For comparable results, keep the laptop stationary, keep the ESP32 antenna orientation consistent, and measure distance with a tape measure. Wait for each 100-block trial to finish before moving the board.</p>
    <p class="sub">Throughput is application-level payload throughput. Latency is the time for a complete 16 KiB HTTP download, not pure ping latency. RSSI is measured at the ESP32 from the laptop's Wi-Fi connection.</p>
  </section>
</div>

</main>

<script>
const BLOCKS_PER_TRIAL = 100;
const EXPECTED_BLOCK_BYTES = 16 * 1024;

let running = false;
let runToken = 0;
let trialBlocks = 0;
let trialBytes = 0;
let trialTimeMs = 0;
let trialLatencySum = 0;
let trialFailures = 0;
let trialDistance = 1;
let latestRate = null;
let latestLatency = null;
let latestRSSI = null;
let rates = [], latencies = [], signals = [], trialRssiSamples = [];
let trials = [];
let packetHistory = [];

function sleep(ms){return new Promise(resolve=>setTimeout(resolve,ms));}
function el(id){return document.getElementById(id);}

function chart(id, values, label, color, fixedRange=null) {
  const c=el(id),ctx=c.getContext('2d'),w=c.width,h=c.height;
  const p={l:43,r:10,t:12,b:28};
  ctx.clearRect(0,0,w,h);
  const data=values.slice(-60);
  let lo,hi;
  if(fixedRange){lo=fixedRange[0];hi=fixedRange[1];}
  else if(data.length){
    const min=Math.min(...data),max=Math.max(...data);
    const spread=max-min;
    // Tight padding makes subtle variations visible without hiding values.
    const pad=Math.max(spread*0.22, label==='dBm'?2:label==='ms'?5:0.08);
    lo=min-pad;hi=max+pad;
    if(label==='dBm'){lo=Math.max(-100,Math.floor(lo/2)*2);hi=Math.min(-20,Math.ceil(hi/2)*2);}
    else {lo=Math.max(0,lo);hi=Math.max(hi,lo+0.1);}
  } else {lo=label==='dBm'?-80:0;hi=label==='dBm'?-30:label==='ms'?150:2;}
  if(hi<=lo)hi=lo+1;
  const plotW=w-p.l-p.r,plotH=h-p.t-p.b;
  ctx.font='12px Arial';ctx.lineWidth=1;ctx.strokeStyle='#e4ebf4';ctx.fillStyle='#344b67';
  for(let i=0;i<=4;i++){
    const y=p.t+plotH*i/4,v=hi-(hi-lo)*i/4;
    ctx.beginPath();ctx.moveTo(p.l,y);ctx.lineTo(w-p.r,y);ctx.stroke();
    ctx.fillText(v.toFixed(label==='dBm'?0:1),2,y+4);
  }
  // X-axis shows sample index (recent 60 samples).
  const ticks=[0,15,30,45,60];
  ticks.forEach((v)=>{
    const x=p.l+plotW*v/60;
    ctx.beginPath();ctx.moveTo(x,p.t);ctx.lineTo(x,h-p.b);ctx.stroke();
    ctx.fillStyle='#344b67';ctx.fillText(String(v),x-5,h-8);
  });
  if(data.length<2){
    ctx.fillStyle='#64748b';ctx.fillText('Waiting for live samples...',p.l+12,h/2);return;
  }
  ctx.strokeStyle=color;ctx.lineWidth=2.5;ctx.beginPath();
  data.forEach((v,i)=>{
    const x=p.l+plotW*i/Math.max(59,1);
    const y=p.t+plotH*(hi-v)/(hi-lo);
    if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
  });
  ctx.stroke();
  // Draw points to reveal small changes.
  ctx.fillStyle=color;
  data.forEach((v,i)=>{
    const x=p.l+plotW*i/Math.max(59,1),y=p.t+plotH*(hi-v)/(hi-lo);
    ctx.beginPath();ctx.arc(x,y,2.1,0,Math.PI*2);ctx.fill();
  });
}

function redraw(){
  chart('rateChart',rates,'Mbps','#16b85f');
  chart('latencyChart',latencies,'ms','#7c3aed');
  // A tight, adaptive scale emphasizes RSSI changes while showing actual dBm labels.
  chart('rssiChart',signals,'dBm','#ef3333');
}

/*
 * Antenna-gain curve is intentionally illustrative. The ESP32 firmware does
 * not measure antenna gain or sweep frequency. Replace these example points
 * only with verified datasheet data for the exact module or calibrated RF measurements.
 */
function drawGainFrequencyChart(){
  const canvas=el('gainFrequencyChart');
  if(!canvas)return;
  const rect=canvas.getBoundingClientRect();
  const dpr=window.devicePixelRatio||1;
  const cssW=Math.max(rect.width,320), cssH=250;
  canvas.width=Math.round(cssW*dpr);
  canvas.height=Math.round(cssH*dpr);
  const ctx=canvas.getContext('2d');
  ctx.scale(dpr,dpr);
  const w=cssW,h=cssH;
  const p={l:54,r:18,t:18,b:42};
  const pw=w-p.l-p.r, ph=h-p.t-p.b;
  const f=[2.400,2.415,2.430,2.445,2.460,2.475,2.4835];
  // Synthetic example values only; NOT measured and NOT module-specific.
  const g=[0.8,1.2,1.5,1.6,1.4,1.0,0.7];
  const ymin=0,ymax=2;
  const x=v=>p.l+(v-2.400)/(2.4835-2.400)*pw;
  const y=v=>p.t+(ymax-v)/(ymax-ymin)*ph;
  ctx.clearRect(0,0,w,h);
  ctx.font='11px Segoe UI, Arial, sans-serif';
  ctx.strokeStyle='#e4ebf4';ctx.fillStyle='#455a73';ctx.lineWidth=1;
  [0,0.5,1,1.5,2].forEach(v=>{
    const yy=y(v);ctx.beginPath();ctx.moveTo(p.l,yy);ctx.lineTo(w-p.r,yy);ctx.stroke();
    ctx.fillText(v.toFixed(1),12,yy+4);
  });
  [2.400,2.420,2.440,2.460,2.480].forEach(v=>{
    const xx=x(v);ctx.beginPath();ctx.moveTo(xx,p.t);ctx.lineTo(xx,h-p.b);ctx.stroke();
    ctx.fillText(v.toFixed(3),xx-17,h-22);
  });
  ctx.strokeStyle='#1677ff';ctx.lineWidth=2.5;ctx.beginPath();
  f.forEach((v,i)=>i===0?ctx.moveTo(x(v),y(g[i])):ctx.lineTo(x(v),y(g[i])));
  ctx.stroke();
  ctx.fillStyle='#1677ff';
  f.forEach((v,i)=>{ctx.beginPath();ctx.arc(x(v),y(g[i]),3.5,0,Math.PI*2);ctx.fill();});
  ctx.fillStyle='#344b67';ctx.font='12px Segoe UI, Arial, sans-serif';
  ctx.fillText('Illustrative gain (dBi)',p.l,12);
  ctx.fillText('Frequency (GHz)',Math.max(p.l,w/2-45),h-4);
}

function updateDistanceEstimate(rssi){
  if(rssi===null || rssi===undefined){
    el('estimatedDistance').textContent='-- m';return;
  }
  // Approximate log-distance path-loss model; calibration varies by environment.
  const txPower=-40, n=2.7;
  const metres=Math.pow(10,(txPower-rssi)/(10*n));
  const clamped=Math.max(0,Math.min(15,metres));
  el('estimatedDistance').textContent=clamped>=15?'15+ m':clamped.toFixed(1)+' m';
  el('scaleMarker').style.left=(Math.min(100,clamped/15*100))+'%';
}

async function updateMetrics(){
  try{
    const response=await fetch('/metrics?t='+Date.now(),{cache:'no-store'});
    if(!response.ok)throw Error('Metrics unavailable');
    const m=await response.json();
    el('headerStatus').textContent='Connected';
    el('connectionTitle').textContent='ESP32 Connected';
    el('connectionDot').style.background='#18c66a';
    el('headerDot').style.background='#19d879';
    if(m.rssi!==null && m.rssi!==undefined){
      latestRSSI=m.rssi;
      el('metricRssi').textContent=m.rssi+' dBm';
      el('graphRssi').textContent=m.rssi+' dBm';
      if(running)trialRssiSamples.push(m.rssi);
      signals.push(m.rssi);signals=signals.slice(-60);
      updateDistanceEstimate(m.rssi);
      redraw();
    }else{
      el('metricRssi').textContent='Waiting';
      el('graphRssi').textContent='-- dBm';
      el('estimatedDistance').textContent='Waiting';
    }
  }catch(e){
    el('headerStatus').textContent='Disconnected';
    el('connectionTitle').textContent='ESP32 Not Reachable';
    el('connectionDot').style.background='#ef3333';
    el('headerDot').style.background='#ef3333';
  }
}

async function setLed(state){
  try{
    const r=await fetch('/led/'+state+'?t='+Date.now(),{cache:'no-store'});
    if(!r.ok)throw Error('LED command failed');
    el('ledStatus').textContent='LED '+state.toUpperCase()+' command sent';
  }catch(e){el('ledStatus').textContent='LED command failed';}
}

function updateTrialUI(){
  el('trialCountLarge').textContent=trialBlocks;
  el('progressText').textContent=trialBlocks+' / '+BLOCKS_PER_TRIAL+' blocks';
  el('trialMB').textContent=(trialBytes/1048576).toFixed(2);
  el('bar').style.width=(trialBlocks/BLOCKS_PER_TRIAL*100)+'%';
  el('barSmall').style.width=(trialBlocks/BLOCKS_PER_TRIAL*100)+'%';
  el('metricPackets').textContent=trialBlocks;
  el('statReceived').textContent=trialBlocks;
  el('metricLost').textContent=trialFailures;
  el('statLost').textContent=trialFailures;
  const total=trialBlocks+trialFailures;
  el('statLossRate').textContent=(total?trialFailures/total*100:0).toFixed(1)+'%';
  el('metricDistance').textContent=trialDistance.toFixed(1)+' m';
  el('currentDistance').textContent=trialDistance.toFixed(1)+' m';
  el('metricRate').textContent=latestRate===null?'--':latestRate.toFixed(2);
  el('graphRate').textContent=latestRate===null?'-- Mbps':latestRate.toFixed(2)+' Mbps';
  el('progressRate').textContent=latestRate===null?'-- Mbps':latestRate.toFixed(2)+' Mbps';
  el('metricLatency').textContent=latestLatency===null?'--':latestLatency.toFixed(0);
  el('graphLatency').textContent=latestLatency===null?'-- ms':latestLatency.toFixed(0)+' ms';
  if(trialTimeMs>0){
    const tput=trialBytes*8/(trialTimeMs/1000)/1000000;
    el('trialThroughput').textContent=tput.toFixed(3)+' Mbps';
  }else el('trialThroughput').textContent='-- Mbps';
  packetHistory.push(trialFailures===0?'ok':'lost');
  packetHistory=packetHistory.slice(-36);
  el('packetStrip').innerHTML=packetHistory.map(x=>'<span class="packet '+(x==='lost'?'lost':'')+'"></span>').join('');
}

function resetTrial(){
  trialBlocks=0;trialBytes=0;trialTimeMs=0;trialLatencySum=0;trialFailures=0;
  trialRssiSamples=[];rates=[];latencies=[];
  latestRate=null;latestLatency=null;
  updateTrialUI();redraw();
}

async function startTest(){
  if(running)return;
  const raw=el('distance').value;
  const d=Number(raw);
  if(raw.trim()===''||!Number.isFinite(d)||d<0){alert('Enter a valid measured distance.');return;}
  resetTrial();
  trialDistance=d;
  el('metricDistance').textContent=d.toFixed(1)+' m';
  el('currentDistance').textContent=d.toFixed(1)+' m';
  running=true;
  const token=++runToken;
  el('start').disabled=true;el('stop').disabled=false;el('distance').disabled=true;
  el('transferStatus').textContent='Transferring data...';
  while(running && token===runToken && trialBlocks<BLOCKS_PER_TRIAL){
    const t0=performance.now();
    try{
      const controller=new AbortController();
      const timeout=setTimeout(()=>controller.abort(),6000);
      let data;
      try{
        const r=await fetch('/download?t='+Date.now()+'_'+trialBlocks,{cache:'no-store',signal:controller.signal});
        if(!r.ok)throw Error('HTTP '+r.status);
        data=await r.arrayBuffer();
      }finally{clearTimeout(timeout);}
      const elapsed=Math.max(performance.now()-t0,1);
      if(!running||token!==runToken)break;
      if(data.byteLength!==EXPECTED_BLOCK_BYTES)throw Error('Incomplete data block');
      const blockRate=data.byteLength*8/(elapsed/1000)/1000000;
      trialBlocks++;trialBytes+=data.byteLength;trialTimeMs+=elapsed;trialLatencySum+=elapsed;
      latestLatency=elapsed;
      rates.push(blockRate);rates=rates.slice(-60);
      latencies.push(elapsed);latencies=latencies.slice(-60);
      const last=rates.slice(-5);
      latestRate=last.reduce((a,b)=>a+b,0)/last.length;
      updateTrialUI();redraw();
      el('transferStatus').textContent='Transferring data...';
      if(trialBlocks===BLOCKS_PER_TRIAL){
        saveCompletedTrial();
        running=false;
        el('transferStatus').textContent='Trial complete — result recorded automatically.';
        el('bar').style.width='100%';el('barSmall').style.width='100%';
        break;
      }
    }catch(e){
      if(!running||token!==runToken)break;
      trialFailures++;updateTrialUI();
      el('transferStatus').textContent='Retrying failed request; successful blocks: '+trialBlocks;
      await sleep(250);
    }
  }
  if(token===runToken){
    running=false;
    el('start').disabled=false;el('stop').disabled=true;el('distance').disabled=false;
  }
}

function stopTest(){
  if(!running)return;
  running=false;runToken++;
  el('transferStatus').textContent='Stopped at '+trialBlocks+'/100 blocks. Incomplete trial not saved.';
  el('start').disabled=false;el('stop').disabled=true;el('distance').disabled=false;
}

function saveCompletedTrial(){
  if(trialBlocks!==BLOCKS_PER_TRIAL)return;
  const throughput=trialBytes*8/(trialTimeMs/1000)/1000000;
  const avgLatency=trialLatencySum/trialBlocks;
  const avgRssi=trialRssiSamples.length?trialRssiSamples.reduce((a,b)=>a+b,0)/trialRssiSamples.length:null;
  trials.push({trial:trials.length+1,distance:trialDistance,rate:throughput,latency:avgLatency,rssi:avgRssi,failures:trialFailures});
  renderTrials();
}

function renderTrials(){
  el('trials').innerHTML=trials.map(t=>'<tr><td>'+t.trial+'</td><td>'+t.distance.toFixed(1)+'</td><td>'+t.rate.toFixed(3)+'</td><td>'+t.latency.toFixed(1)+'</td><td>'+(t.rssi===null?'N/A':t.rssi.toFixed(1))+'</td><td>'+t.failures+'</td></tr>').join('');
}

function clearTrials(){
  if(running){alert('Stop the current trial before clearing results.');return;}
  if(!confirm('Clear all recorded trials? Export CSV first if needed.'))return;
  trials=[];renderTrials();resetTrial();
  el('transferStatus').textContent='Trials cleared. Ready for a new test.';
}

function exportCSV(){
  if(!trials.length){alert('Complete a trial first.');return;}
  const csv='Trial,Distance_m,Throughput_Mbps,Average_Request_Latency_ms,Average_RSSI_dBm,Failed_Requests\n'+
    trials.map(t=>[t.trial,t.distance,t.rate.toFixed(6),t.latency.toFixed(3),t.rssi===null?'':t.rssi.toFixed(2),t.failures].join(',')).join('\n');
  const url=URL.createObjectURL(new Blob([csv],{type:'text/csv'}));
  const a=document.createElement('a');a.href=url;a.download='SRWC_Distance_Trials.csv';a.click();
  setTimeout(()=>URL.revokeObjectURL(url),1000);
}

el('distance').addEventListener('input',()=>{
  const d=Number(el('distance').value);
  if(el('distance').value.trim()!==''&&Number.isFinite(d)&&d>=0&&!running){
    el('metricDistance').textContent=d.toFixed(1)+' m';
    el('currentDistance').textContent=d.toFixed(1)+' m';
  }
});

setInterval(updateMetrics,1000);
updateMetrics();redraw();drawGainFrequencyChart();updateTrialUI();
window.addEventListener('resize',drawGainFrequencyChart);
</script>
</body>
</html>
)rawliteral");
}

void handleDownload() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/octet-stream", testPayload);
}

void handleMetrics() {
  wifi_sta_list_t stations;
  bool haveRSSI = false;
  int rssi = 0;

  if (esp_wifi_ap_get_sta_list(&stations) == ESP_OK && stations.num > 0) {
    rssi = stations.sta[0].rssi;
    haveRSSI = true;
  }

  String json = "{\"rssi\":";
  json += haveRSSI ? String(rssi) : "null";
  json += ",\"clients\":";
  json += String(WiFi.softAPgetStationNum());
  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  // Initialize I2C OLED on SDA=GPIO21 and SCL=GPIO22.
  Wire.begin(21, 22);
  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oled.clearDisplay();
    oled.display();
    Serial.println("OLED initialized successfully!");
  } else {
    Serial.println("OLED initialization failed!");
  }

  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);

  testPayload.reserve(BLOCK_SIZE);
  for (size_t i = 0; i < BLOCK_SIZE; i++) {
    testPayload += 'A';
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/metrics", HTTP_GET, handleMetrics);

  server.on("/led/on", HTTP_GET, []() {
    digitalWrite(GREEN_LED, HIGH);
    server.send(200, "text/plain", "LED ON");
  });

  server.on("/led/off", HTTP_GET, []() {
    digitalWrite(GREEN_LED, LOW);
    server.send(200, "text/plain", "LED OFF");
  });

  server.begin();

  Serial.println("SRWC Performance Analyzer ready");
  Serial.print("Wi-Fi network: ");
  Serial.println(ssid);
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
}

// Refresh the OLED once per second. The graph shows the most recent
// RSSI samples from connected Wi-Fi clients; it retains history on disconnect.
void updateOLED() {
  if (millis() - lastOledUpdate < 1000) return;
  lastOledUpdate = millis();

  wifi_sta_list_t stations;
  bool connected =
      esp_wifi_ap_get_sta_list(&stations) == ESP_OK && stations.num > 0;

  if (connected) {
    const int rssi = stations.sta[0].rssi;
    if (rssiCount < 128) {
      rssiHistory[rssiCount++] = rssi;
    } else {
      for (int i = 0; i < 127; ++i) {
        rssiHistory[i] = rssiHistory[i + 1];
      }
      rssiHistory[127] = rssi;
    }
  }

  oled.clearDisplay();

  // Graph occupies the upper 21 pixels. Stronger RSSI is drawn higher.
  oled.drawLine(0, 1, 127, 1, SSD1306_WHITE);
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.drawLine(0, 20, 127, 20, SSD1306_WHITE);

  for (int i = 1; i < rssiCount; ++i) {
    const int previousY = map(constrain(rssiHistory[i - 1], -90, -30),
                              -90, -30, 19, 2);
    const int currentY = map(constrain(rssiHistory[i], -90, -30),
                             -90, -30, 19, 2);
    oled.drawLine(i - 1, previousY, i, currentY, SSD1306_WHITE);
  }

  // A 128x32 display cannot fit the full phrase "Client Status: Disconnected"
  // in the built-in 5x7 font, so use a compact one-line status label.
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextWrap(false);
  oled.setCursor(0, 24);
  oled.print("Client: ");
  oled.print(connected ? "Connected" : "Disconnected");
  oled.display();
}

void loop() {
  server.handleClient();
  updateOLED();
}
