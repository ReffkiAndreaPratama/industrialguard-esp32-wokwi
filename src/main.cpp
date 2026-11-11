/* IndustrialGuard Professional — ESP32 industrial condition monitoring demo.
   Wokwi simulation only: potentiometers model vibration and current, not physical sensors.
   No certified predictive maintenance or safety functionality. */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <math.h>

namespace Pins {
constexpr int dht=4, vibration=34, current=35, rpm=32;
constexpr int ledGreen=25, ledYellow=26, ledRed=27, buzzer=19;
constexpr int relay=18, mute=23, mode=13, sda=21, scl=22;
}
namespace Config {
constexpr uint32_t sampleMs=1000;
constexpr float warnTemp=38, alarmTemp=48, warnVibration=55, alarmVibration=80;
constexpr float warnCurrent=65, alarmCurrent=85;
constexpr float minRpm=300, maxRpm=3000;
}
WebServer server(80);
DHTesp dht;
Adafruit_SSD1306 oled(128,64,&Wire,-1);
bool oledReady=false, alarmMuted=false, motorRunning=true, automatic=true;
bool previousMute=true, previousMode=true;
uint32_t lastMute=0,lastMode=0,lastSample=0, lastHistory=0;
float tempC=25, humidity=50, vibration=0, currentPct=0, rpm=0, health=100;
float previousVibration=0;
int severity=0;
uint32_t sampleCount=0, alarmCount=0;
String events[12]; int eventPos=0;

const char DASHBOARD[] PROGMEM=R"HTML(<!doctype html><html lang="id"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>IndustrialGuard | Operations Center</title><style>
:root{--bg:#08111e;--panel:#121f30;--line:#26394e;--muted:#9cb0c4;--text:#eaf3fb;--cyan:#45d3e8;--green:#32d4a1;--yellow:#ffc857;--red:#ff6077}*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 12% 0%,#163650,transparent 42%),var(--bg);color:var(--text);font:14px system-ui,Arial}main{max-width:1200px;margin:auto;padding:24px}.top{display:flex;align-items:center;justify-content:space-between;gap:16px;flex-wrap:wrap}h1{margin:0;font-size:28px;letter-spacing:.2px}.sub{color:var(--muted);margin-top:6px}.pill{border:1px solid #31556b;border-radius:30px;padding:9px 14px;color:var(--cyan)}.grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:13px;margin-top:22px}.card{background:linear-gradient(145deg,#15263a,#101b2b);border:1px solid var(--line);border-radius:15px;padding:18px}.label{font-size:12px;color:var(--muted);text-transform:uppercase;letter-spacing:.8px}.value{font-size:32px;font-weight:800;margin:9px 0 4px}.unit{font-size:12px;color:var(--muted)}.bar{height:8px;background:#26384e;border-radius:12px;overflow:hidden;margin-top:13px}.fill{height:100%;width:0;background:var(--cyan);transition:width .3s}.cols{display:grid;grid-template-columns:1.2fr .8fr;gap:13px;margin-top:13px}.cols .card{margin-bottom:13px}h2{font-size:16px;margin:0 0 13px}.row{display:flex;justify-content:space-between;border-bottom:1px solid var(--line);padding:11px 0}.row span{color:var(--muted)}.status{font-size:23px;font-weight:800;margin:14px 0}.dot{display:inline-block;width:12px;height:12px;border-radius:50%;background:var(--green);margin-right:8px}button{padding:11px 14px;border:0;border-radius:9px;background:#167ca9;color:white;font-weight:700;cursor:pointer;margin:5px 5px 0 0}button.secondary{background:#31445b}button.danger{background:#aa2f45}.log{padding:0;margin:0;list-style:none}.log li{border-bottom:1px solid var(--line);padding:9px 0;color:#c8d9e9;font-size:12px}.hint{font-size:12px;color:var(--muted);line-height:1.6;margin-top:14px}.tag{color:var(--cyan);font-size:12px}@media(max-width:900px){.grid{grid-template-columns:repeat(2,1fr)}.cols{grid-template-columns:1fr}}@media(max-width:420px){main{padding:12px}.card{padding:12px}.value{font-size:25px}}</style></head><body><main>
<div class="top"><div><h1>⚙️ IndustrialGuard</h1><div class="sub">PROFESSIONAL · CONDITION MONITORING & MACHINE HEALTH</div></div><div class="pill" id="conn">Connecting…</div></div>
<div class="grid"><div class="card"><div class="label">Temperature</div><div class="value" id="temp">--</div><div class="unit">°C / DHT22</div><div class="bar"><div class="fill" id="tempbar"></div></div></div><div class="card"><div class="label">Vibration</div><div class="value" id="vib">--</div><div class="unit">% simulated vibration</div><div class="bar"><div class="fill" id="vibbar"></div></div></div><div class="card"><div class="label">Motor load</div><div class="value" id="current">--</div><div class="unit">% simulated current</div><div class="bar"><div class="fill" id="currentbar"></div></div></div><div class="card"><div class="label">Machine health</div><div class="value" id="health">--</div><div class="unit">score / 100 (heuristic)</div><div class="bar"><div class="fill" id="healthbar"></div></div></div></div>
<div class="cols"><div><div class="card"><h2>LIVE ASSET CONDITION</h2><div class="status"><span class="dot" id="dot"></span><span id="status">--</span></div><div class="row"><span>Motor status</span><b id="motor">--</b></div><div class="row"><span>Operating mode</span><b id="mode">--</b></div><div class="row"><span>Simulated RPM</span><b id="rpm">--</b></div><div class="row"><span>Humidity</span><b id="hum">--</b></div><div class="row"><span>Alert count</span><b id="alerts">--</b></div><div class="row"><span>Samples processed</span><b id="samples">--</b></div><p class="hint">Health score is rule-based, not a trained machine-learning failure prediction. Alarm thresholds are demonstration values.</p></div><div class="card"><h2>REMOTE OPERATIONS</h2><button onclick="cmd('mode','auto')">AUTO</button><button class="secondary" onclick="cmd('mode','manual')">MANUAL</button><button onclick="cmd('motor','start')">START MOTOR</button><button class="danger" onclick="cmd('motor','stop')">STOP MOTOR</button><button class="secondary" onclick="cmd('mute','toggle')">TOGGLE MUTE</button><button class="secondary" onclick="cmd('test','alarm')">TEST ALARM</button><p class="hint">Motor is represented by a relay and a status indicator; no physical motor load is connected.</p></div></div><div><div class="card"><h2>ANOMALY INSIGHTS</h2><div class="row"><span>Temperature condition</span><b id="tc">--</b></div><div class="row"><span>Vibration condition</span><b id="vc">--</b></div><div class="row"><span>Load condition</span><b id="cc">--</b></div><div class="row"><span>Alarm</span><b id="mute">--</b></div><p class="hint">Severity: NORMAL → WARNING → CRITICAL. Critical condition in AUTO mode stops the simulated motor.</p></div><div class="card"><h2>EVENT TIMELINE</h2><ul class="log" id="events"><li>Waiting for events</li></ul></div></div></div><div class="hint">IndustrialGuard demonstration only. Not an industrial safety controller or validated predictive maintenance solution.</div></main><script>
const $=x=>document.getElementById(x);async function cmd(k,v){try{await fetch('/api/control?'+k+'='+v);await refresh()}catch(e){}}const colors=['#32d4a1','#ffc857','#ff6077'];function setBar(id,v){$(id).style.width=Math.max(0,Math.min(100,v))+'%'}async function refresh(){try{const r=await fetch('/api/status'),d=await r.json();$('conn').textContent='● ONLINE';$('temp').textContent=d.temp.toFixed(1);$('vib').textContent=d.vibration.toFixed(0);$('current').textContent=d.load.toFixed(0);$('health').textContent=d.health.toFixed(0);setBar('tempbar',d.temp/70*100);setBar('vibbar',d.vibration);setBar('currentbar',d.load);setBar('healthbar',d.health);$('healthbar').style.background=colors[d.severity];$('status').textContent=d.status;$('dot').style.background=colors[d.severity];$('motor').textContent=d.motor?'RUNNING':'STOPPED';$('mode').textContent=d.auto?'AUTO':'MANUAL';$('rpm').textContent=d.rpm.toFixed(0)+' rpm';$('hum').textContent=d.humidity.toFixed(0)+'%';$('alerts').textContent=d.alerts;$('samples').textContent=d.samples;$('tc').textContent=d.temp>=48?'CRITICAL':d.temp>=38?'WARNING':'NORMAL';$('vc').textContent=d.vibration>=80?'CRITICAL':d.vibration>=55?'WARNING':'NORMAL';$('cc').textContent=d.load>=85?'CRITICAL':d.load>=65?'WARNING':'NORMAL';$('mute').textContent=d.muted?'MUTED':'ACTIVE';$('events').innerHTML=d.events.map(x=>'<li>'+x+'</li>').join('')||'<li>No events</li>'}catch(e){$('conn').textContent='● OFFLINE'}}setInterval(refresh,1000);refresh();</script></body></html>)HTML";

void eventLog(const String &s) { events[eventPos]=String(millis()/1000)+"s · "+s; eventPos=(eventPos+1)%12; Serial.println("[EVENT] "+s); }
String escapeJson(const String &s){String r;for(size_t i=0;i<s.length();i++){char c=s[i];if(c=='"'||c=='\\')r+='\\';if(c>=32)r+=c;}return r;}
float pct(int pin){return constrain(analogRead(pin)*100.0f/4095.0f,0.0f,100.0f);}
void sampleSensors(){
  TempAndHumidity th=dht.getTempAndHumidity();
  if(!isnan(th.temperature) && th.temperature>-30 && th.temperature<100) tempC=th.temperature;
  if(!isnan(th.humidity) && th.humidity>=0 && th.humidity<=100) humidity=th.humidity;
  previousVibration=vibration; vibration=pct(Pins::vibration); currentPct=pct(Pins::current);
  rpm=motorRunning?Config::minRpm+(pct(Pins::rpm)/100.0f)*(Config::maxRpm-Config::minRpm):0;
  severity=0;
  if(tempC>=Config::warnTemp||vibration>=Config::warnVibration||currentPct>=Config::warnCurrent)severity=1;
  if(tempC>=Config::alarmTemp||vibration>=Config::alarmVibration||currentPct>=Config::alarmCurrent)severity=2;
  health=constrain(100.0f-max(0.0f,tempC-30.0f)*1.0f-vibration*0.35f-currentPct*0.30f,0.0f,100.0f);
  static int oldSeverity=-1;
  if(severity!=oldSeverity){if(severity==2)alarmCount++;eventLog(String("Condition: ")+(severity==2?"CRITICAL":severity==1?"WARNING":"NORMAL"));oldSeverity=severity;}
  if(automatic && severity==2 && motorRunning){motorRunning=false;eventLog("AUTO shutdown: critical anomaly");}
  sampleCount++;
}
void outputs(){
  digitalWrite(Pins::relay,motorRunning?HIGH:LOW);
  digitalWrite(Pins::ledGreen,severity==0?HIGH:LOW);
  digitalWrite(Pins::ledYellow,severity==1?HIGH:LOW);
  digitalWrite(Pins::ledRed,severity==2?HIGH:LOW);
  if(alarmMuted||severity==0){noTone(Pins::buzzer);return;}
  unsigned long phase=millis()%1500;
  if(severity==1){if(phase<180)tone(Pins::buzzer,1200);else noTone(Pins::buzzer);}
  else {if(phase<300||(phase>450&&phase<800)||(phase>950&&phase<1300))tone(Pins::buzzer,2000);else noTone(Pins::buzzer);}
}
void updateOled(){if(!oledReady)return;oled.clearDisplay();oled.setTextColor(SSD1306_WHITE);oled.setTextSize(1);oled.setCursor(0,0);oled.println("INDUSTRIALGUARD");oled.drawLine(0,11,127,11,SSD1306_WHITE);oled.setCursor(0,17);oled.print("TEMP ");oled.print(tempC,1);oled.print(" C");oled.setCursor(0,28);oled.print("VIB  ");oled.print(vibration,0);oled.print(" %");oled.setCursor(0,39);oled.print("LOAD ");oled.print(currentPct,0);oled.print(" %");oled.setCursor(0,50);oled.print(severity==2?"CRITICAL":severity==1?"WARNING":"NORMAL");oled.print("  ");oled.print(motorRunning?"RUN":"STOP");oled.display();}
String statusJson(){String e="[";bool first=true;for(int i=0;i<12;i++){int idx=(eventPos+11-i)%12;if(events[idx].length()){if(!first)e+=",";e+="\""+escapeJson(events[idx])+"\"";first=false;}}e+="]";
 String j="{";j+="\"temp\":"+String(tempC,1)+",\"humidity\":"+String(humidity,1)+",";
 j+="\"vibration\":"+String(vibration,1)+",\"load\":"+String(currentPct,1)+",";
 j+="\"rpm\":"+String(rpm,1)+",\"health\":"+String(health,1)+",";
 j+="\"severity\":"+String(severity)+",\"status\":\""+String(severity==2?"CRITICAL":severity==1?"WARNING":"NORMAL")+"\",";
 j+="\"motor\":"+String(motorRunning?"true":"false")+",\"auto\":"+String(automatic?"true":"false")+",";
 j+="\"muted\":"+String(alarmMuted?"true":"false")+",\"samples\":"+String(sampleCount)+",\"alerts\":"+String(alarmCount)+",\"events\":"+e+"}";return j;}
void handleControl(){
 if(server.hasArg("mode")){String m=server.arg("mode");if(m=="auto")automatic=true;else if(m=="manual")automatic=false;eventLog(String("Mode: ")+(automatic?"AUTO":"MANUAL"));}
 if(server.hasArg("motor")){String m=server.arg("motor");if(m=="start"){if(automatic && severity==2){server.send(409,"application/json","{\"ok\":false,\"error\":\"Critical condition in AUTO mode\"}");return;}motorRunning=true;eventLog("Motor START command");}else if(m=="stop"){motorRunning=false;eventLog("Motor STOP command");}}
 if(server.hasArg("mute")){alarmMuted=!alarmMuted;eventLog(alarmMuted?"Alarm muted":"Alarm unmuted");}
 if(server.hasArg("test")){tone(Pins::buzzer,1900,300);eventLog("Buzzer test");}
 outputs();server.send(200,"application/json","{\"ok\":true}");}
void buttons(){unsigned long now=millis();bool m=digitalRead(Pins::mute),a=digitalRead(Pins::mode);
 if(previousMute && !m && now-lastMute>220){alarmMuted=!alarmMuted;eventLog(alarmMuted?"Physical MUTE ON":"Physical MUTE OFF");lastMute=now;}
 if(previousMode && !a && now-lastMode>220){automatic=!automatic;eventLog(String("Physical mode: ")+(automatic?"AUTO":"MANUAL"));lastMode=now;}
 previousMute=m;previousMode=a;}
void setup(){Serial.begin(115200);pinMode(Pins::ledGreen,OUTPUT);pinMode(Pins::ledYellow,OUTPUT);pinMode(Pins::ledRed,OUTPUT);pinMode(Pins::relay,OUTPUT);pinMode(Pins::buzzer,OUTPUT);pinMode(Pins::mute,INPUT_PULLUP);pinMode(Pins::mode,INPUT_PULLUP);pinMode(Pins::vibration,INPUT);pinMode(Pins::current,INPUT);pinMode(Pins::rpm,INPUT);
 analogReadResolution(12);analogSetPinAttenuation(Pins::vibration,ADC_11db);analogSetPinAttenuation(Pins::current,ADC_11db);analogSetPinAttenuation(Pins::rpm,ADC_11db);
 dht.setup(Pins::dht,DHTesp::DHT22);Wire.begin(Pins::sda,Pins::scl);oledReady=oled.begin(SSD1306_SWITCHCAPVCC,0x3C);
 WiFi.mode(WIFI_STA);WiFi.begin("Wokwi-GUEST","");unsigned long started=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-started<12000){delay(250);}
 if(WiFi.status()==WL_CONNECTED){Serial.print("WiFi IP: ");Serial.println(WiFi.localIP());}else Serial.println("WiFi offline; local controls remain available");
 server.on("/",HTTP_GET,[](){server.send_P(200,"text/html",DASHBOARD);});server.on("/api/status",HTTP_GET,[](){server.send(200,"application/json",statusJson());});server.on("/api/control",HTTP_GET,handleControl);server.onNotFound([](){server.send(404,"text/plain","Not found");});server.begin();eventLog("IndustrialGuard boot complete");sampleSensors();outputs();updateOled();}
void loop(){server.handleClient();buttons();unsigned long now=millis();if(now-lastSample>=Config::sampleMs){lastSample=now;sampleSensors();updateOled();}outputs();}
