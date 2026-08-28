// =============================================================
//  ESP32_LED_Web — управление лентой WS2812B с телефона по WiFi
// =============================================================
//  Режимы (переключаются с телефона):
//   • eq     — эквалайзер из аудио/видео-файла, загруженного в браузер
//   • sound  — эквалайзер от внешнего звукового датчика (любой звук вокруг)
//   • rainbow/breath/static — анимации
//  Телефон также шлёт яркость/скорость/цвет.
//  Анализ звука (FFT) делает браузер телефона и шлёт спектр по WebSocket.
// =============================================================

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include "config.h"

// ---------- Сетевые объекты ----------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------- Состояние ленты ----------
CRGB leds[NUM_LEDS];
String mode = "eq";                 // eq | sound | rainbow | breath | static
int bands[BANDS] = {0};             // уровни частот для эквалайзера (0..255)
CRGB baseColor = CRGB(0, 255, 0);   // цвет для breath/static
int bright = LED_BRIGHTNESS;         // яркость 0..255
int speed = 5;                      // скорость анимации 1..10

// ---------- Звуковой датчик (режим "sound") ----------
int beatCount = 0;
bool prevSound = LOW;
unsigned long lastSound = 0;
unsigned long soundWindow = 0;
int sensorLevel = 0;                // итоговый уровень 0..255 (сглажен)

// ---------- Тест ленты (проверка связи без звука) ----------
unsigned long testUntil = 0;

// =============================================================
//  ВЕБ-СТРАНИЦА (хранится в прошивке)
// =============================================================
static const char index_html[] PROGMEM = R"INDEX(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>LED лента</title>
<style>
 body{font-family:sans-serif;max-width:480px;margin:20px auto;padding:0 12px;}
 h1{font-size:20px;}
 .row{margin:12px 0;}
 button{padding:10px 14px;margin:4px;font-size:15px;border:1px solid #888;background:#eee;border-radius:8px;}
 button.active{background:#4caf50;color:#fff;border-color:#4caf50;}
 label{display:block;margin:6px 0 2px;}
 input[type=range]{width:100%;}
 #status{color:#666;font-size:13px;margin-top:10px;}
 #viz{width:100%;background:#111;border-radius:6px;margin-top:8px;}
</style>
</head>
<body>
<h1>Управление LED-лентой</h1>

<div class="row">
 <label>Выбери режим:</label>
 <button class="mode active" data-m="eq">Эквалайзер (файл)</button>
  <button class="mode" data-m="sound">Звук (датчик)</button>
  <button class="mode" data-m="rainbow">Радуга</button>
 <button class="mode" data-m="breath">Дыхание</button>
 <button class="mode" data-m="static">Один цвет</button>
</div>

<div class="row" id="musicRow">
 <label>Загрузи аудио или видео (mp3, m4a, wav, aac, mp4, ...):</label>
 <input type="file" id="file" accept="audio/*,video/*,.mp3,.m4a,.wav,.aac,.ogg,.mp4,.mov"><br><br>
 <video id="audio" controls style="width:100%"></video>
 <button id="startBtn">▶ Запустить анализ (обязательно нажми)</button>
 <canvas id="viz" width="300" height="90"></canvas>
 <div>состояние звука: <span id="ctxState">-</span> | max на телефоне: <span id="maxv">0</span></div>
</div>

<div class="row" id="colorRow" style="display:none">
 <label>Цвет (для режима "Один цвет"):</label>
 <input type="color" id="color" value="#00ff00">
</div>

<div class="row">
 <label>Яркость: <span id="brightVal">100</span></label>
 <input type="range" id="bright" min="0" max="255" value="100">
</div>
<div class="row">
 <label>Скорость: <span id="speedVal">5</span></label>
 <input type="range" id="speed" min="1" max="10" value="5">
</div>

<div class="row">
 <button id="testBtn">Тест ленты (проверка связи)</button>
</div>

<div id="status">Подключение...</div>

<script>
const BANDS = 16;
let ws, audioCtx, analyser, source, dataArray, stream;
let currentMode = "eq";
const canvas = document.getElementById('viz');
const ctx = canvas.getContext('2d');

function status(t){ document.getElementById('status').innerText = t; }

function connect(){
  ws = new WebSocket(`ws://${location.host}/ws`);
  ws.onopen = ()=>{ status("WebSocket: подключено ✅"); send({mode:currentMode}); };
  ws.onclose = ()=>{ status("WebSocket: отключено, переподключение..."); setTimeout(connect, 2000); };
  ws.onerror = ()=> status("WebSocket: ошибка");
}
function send(obj){ if(ws && ws.readyState === 1) ws.send(JSON.stringify(obj)); }

function setMode(m){
  currentMode = m;
  send({mode:m});
  document.querySelectorAll('.mode').forEach(b=>b.classList.toggle('active', b.dataset.m===m));
  document.getElementById('colorRow').style.display = (m==='static') ? 'block' : 'none';
  document.getElementById('musicRow').style.display = (m==='eq') ? 'block' : 'none';
}

function drawBars(bands){
  ctx.clearRect(0,0,canvas.width,canvas.height);
  const w = canvas.width / BANDS;
  for(let i=0;i<BANDS;i++){
    const h = bands[i]/255 * canvas.height;
    ctx.fillStyle = `hsl(${i/BANDS*300},100%,50%)`;
    ctx.fillRect(i*w, canvas.height-h, w-1, h);
  }
}

const media = document.getElementById('audio');
let rafStarted = false;

document.getElementById('file').onchange = e => { media.src = URL.createObjectURL(e.target.files[0]); };
media.onplay = () => startAudio();
// ВАЖНО: AudioContext нужно создавать/возобновлять СТРОГО внутри клика (жеста),
// иначе iOS/Android оставляют его "suspended" и анализатор выдаёт нули.
document.getElementById('startBtn').onclick = () => { media.play(); startAudio(); };

function startAudio()
{
  if (!audioCtx)
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  if (audioCtx.state === 'suspended') audioCtx.resume();

  if (source) { setCtxStatus(); return; }  // уже настроено

  // Пробуем взять звук из элемента напрямую, иначе через captureStream
  try { source = audioCtx.createMediaElementSource(media); }
  catch (e) { source = null; }
  if (!source && media.captureStream)
  {
    try { stream = media.captureStream(); source = audioCtx.createMediaStreamSource(stream); }
    catch (e) { source = null; }
  }
  if (!source) { status("Не удалось создать источник звука"); return; }

  analyser = audioCtx.createAnalyser();
  analyser.fftSize = 128;                 // больше частотных корзин -> детальнее
  analyser.smoothingTimeConstant = 0.6;   // поменьше сглаживания -> живее
  analyser.minDecibels = -90;             // делаем тихие части видимыми
  analyser.maxDecibels = 0;               // поднимаем потолок, чтобы спектр не клипповал
  dataArray = new Uint8Array(analyser.frequencyBinCount);
  source.connect(analyser);
  analyser.connect(audioCtx.destination);   // чтобы звук ещё и слышать
  setCtxStatus();
  if (!rafStarted) { rafStarted = true; requestAnimationFrame(loop); }
}

function setCtxStatus()
{
  document.getElementById('ctxState').innerText = audioCtx ? audioCtx.state : "нет";
}

function loop(){
  if(currentMode === 'eq' && analyser){
    analyser.getByteFrequencyData(dataArray);
    const per = Math.floor(dataArray.length / BANDS);
    let bands = [];
    for(let i=0;i<BANDS;i++){
      let s = 0;
      for(let j=0;j<per;j++) s += dataArray[i*per + j];
       let v = Math.round(s / per);
       v = Math.min(255, v);
       bands.push(v);
    }
    drawBars(bands);   // визуализация на самом телефоне (проверка анализа)
    send({bands});     // и шлём на ленту
    document.getElementById('maxv').innerText = Math.max.apply(null, bands);
  }
  requestAnimationFrame(loop);
}

document.querySelectorAll('.mode').forEach(b => b.onclick = ()=> setMode(b.dataset.m));
document.getElementById('testBtn').onclick = ()=> send({test:1});
document.getElementById('bright').oninput = e => { document.getElementById('brightVal').innerText = e.target.value; send({bright:+e.target.value}); };
document.getElementById('speed').oninput  = e => { document.getElementById('speedVal').innerText  = e.target.value; send({speed:+e.target.value}); };
document.getElementById('color').oninput  = e => {
  const h = e.target.value;
  send({color:[parseInt(h.substr(1,2),16), parseInt(h.substr(3,2),16), parseInt(h.substr(5,2),16)]});
};

connect();
</script>
</body>
</html>
)INDEX";

// =============================================================
//  ОБРАБОТКА СООБЩЕНИЙ ОТ ТЕЛЕФОНА
// =============================================================
void handleMessage(String msg)
{
  // Буфер должен вместить объект с массивом из 16 чисел.
  // На ESP32 каждый слот варианта ~16 байт, поэтому 256 мало — ставим 512.
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, msg))
    return;

  if (doc.containsKey("mode"))      mode = doc["mode"].as<String>();
  if (doc.containsKey("test"))      testUntil = millis() + 2000;     // тест на 2 сек

  if (doc.containsKey("bands"))
  {
    JsonArray arr = doc["bands"];
    int n = min((int)arr.size(), BANDS);
    for (int i = 0; i < n; i++) bands[i] = arr[i];
  }
  if (doc.containsKey("color"))
  {
    JsonArray c = doc["color"];
    baseColor = CRGB(c[0], c[1], c[2]);
  }
  if (doc.containsKey("bright")) bright = doc["bright"];
  if (doc.containsKey("speed"))  speed  = doc["speed"];
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.println("WS: клиент подключился");
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.println("WS: клиент отключился");
  }
  else if (type == WS_EVT_DATA)
  {
    // Сообщение может прийти фрагментами — накапливаем, пока не final.
    static String wsBuf;
    AwsFrameInfo *info = (AwsFrameInfo *)arg;

    if (info->index == 0) wsBuf = "";          // начало нового сообщения
    wsBuf.concat(String((char *)data, len));    // добавляем кусок

    if (info->final)                            // сообщение целиком — обрабатываем
    {
      static int dbg = 0;
      if (dbg < 8)                              // первые 8 сообщений — в консоль
      {
        Serial.print("RAW len=");
        Serial.print(wsBuf.length());
        Serial.print(" '");
        Serial.print(wsBuf);
        Serial.println("'");
        dbg++;
      }
      handleMessage(wsBuf);
      wsBuf = "";
    }
  }
}

// =============================================================
//  ОТРИСОВКА
// =============================================================
void renderEq()
{
  // В танцевальной музыке в басах постоянно есть энергия (бас-линия),
  // а УДАР (кик) даёт всплеск поверх неё. Поэтому берём среднюю энергию
  // низких полос и вычитаем медленный "базовый" уровень — остаётся только
  // то, что громче обычного, т.е. сам ритм. Так лента чётко бьётся и
  // затухает между ударами (яркость и растёт, и падает, без "вечного красного").
  int sum = 0, cnt = 0;
  for (int i = 0; i < BANDS / 2; i++) { sum += bands[i]; cnt++; }
  int level = cnt ? sum / cnt : 0;

  static float base = 0;
  base += (level - base) * 0.03;            // медленный базовый уровень (~0.5 с)
  int delta = level - (int)base;            // насколько громче "фона"
  if (delta < 0) delta = 0;
  int drive = constrain(delta * 7, 0, 255); // сильное усиление отклонения (достаём верх)

  // Огибающая: резкая атака на удар + плавное затухание.
  static float env = 0;
  float target = drive / 255.0;
  if (target > env) env += (target - env) * 0.65;  // атака резче
  else              env += (target - env) * 0.22;  // плавное затухание

  // Вся лента ОДНИМ цветом, яркость пульсирует в такт. Тихо -> тускло-зелёный,
  // громко -> ярко-красный. Линейная яркость, чтобы лента реально разгонялась.
  float f = env;
  int b = (int)(bright * f);
  uint8_t hue = map((int)(env * 255), 0, 255, 100, 0);
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, b));
  FastLED.show();
}

void renderRainbow()
{
  static uint8_t hue = 0;
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CHSV(hue + i * (255 / NUM_LEDS), 255, 255);
  hue += speed;
  FastLED.show();
}

void renderBreath()
{
  static float t = 0;
  t += 0.02 * speed;
  int b = ((sin(t) + 1) / 2) * 255;
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CRGB(baseColor.r * b / 255, baseColor.g * b / 255, baseColor.b * b / 255);
  FastLED.show();
}

void renderStatic()
{
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = baseColor;
  FastLED.show();
}

// Бегущий пучок — для кнопки "Тест ленты" (проверка связи ESP↔телефон↔лента)
void renderTest()
{
  static int pos = 0;
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
  for (int k = 0; k < 6; k++)
  {
    int idx = (pos + k) % NUM_LEDS;
    leds[idx] = CHSV(pos * 4, 255, 255);
  }
  pos++;
  FastLED.show();
}

// =============================================================
//  НАСТРОЙКА
// =============================================================
void setup()
{
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Подключаюсь к WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi подключён!");
  Serial.print("Открой в телефоне: http://");
  Serial.println(WiFi.localIP());

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(bright);
  FastLED.clear();
  FastLED.show();

  pinMode(SOUND_PIN, INPUT);
  soundWindow = millis();

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", index_html);
  });
  server.begin();
}

// =============================================================
//  ГЛАВНЫЙ ЦИКЛ
// =============================================================
void loop()
{
  unsigned long now = millis();

  // Тест ленты: короткая анимация без звука (доказывает, что связь работает)
  if (now < testUntil)
  {
    renderTest();
    delay(20);
    return;
  }

  // Опрос звукового датчика (режим "sound")
  bool s = digitalRead(SOUND_PIN);
  if (s == HIGH && prevSound == LOW)
  {
    if (now - lastSound > DEBOUNCE_MS) { beatCount++; lastSound = now; }
  }
  prevSound = s;

  if (now - soundWindow >= WINDOW_MS)
  {
    int target = 255 * beatCount / MAX_BEATS;
    sensorLevel = sensorLevel + 0.4 * (target - sensorLevel);
    beatCount = 0;
    soundWindow = now;
  }

  FastLED.setBrightness(bright);

  if (mode == "eq")
  {
    renderEq();               // bands приходят с телефона
  }
  else if (mode == "sound")
  {
    for (int i = 0; i < BANDS; i++) bands[i] = sensorLevel;
    renderEq();
  }
  else if (mode == "rainbow") renderRainbow();
  else if (mode == "breath")  renderBreath();
  else                        renderStatic();

  // Отладка: режим + сумма уровней (должна быть > 0 в eq при игре звука)
  static unsigned long lastPrint = 0;
  if (now - lastPrint > 500)
  {
    lastPrint = now;
    int sum = 0;
    for (int i = 0; i < BANDS; i++) sum += bands[i];
    Serial.print("mode=");
    Serial.print(mode);
    Serial.print(" bands sum=");
    Serial.println(sum);
  }

  delay(20);
}
