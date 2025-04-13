#define HC_ECHO 5  // пин Echo
#define HC_TRIG 4  // пин Trig

#define LED_MAX_MA 1500  // ограничение тока ленты, ма 800 от USB, 1500 от лампы
#define LED_PIN 19       // пин ленты
#define LED_NUM 58       // к-во светодиодов

#define VB_DEB 0      // отключаем антидребезг (он есть у фильтра)
#define VB_CLICK 900  // таймаут клика

#include <EncButton.h>
VirtButton gest;

#include <GRGB.h>
GRGB led;

#include <FastLED.h>
CRGB leds[LED_NUM];

#undef nullptr
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID "wifi-name"
#define STAPSK "password"
#endif
#define API_KEY "secret-key"

const char* ssid = STASSID;
const char* password = STAPSK;
const char* apiKey = API_KEY;

WebServer server(80);

#include "Data.h"
#include <Preferences.h>

Data data;

String successResponse = "{\"status\":\"success\"}";
String invalidResponse = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
String invalidApiKey = "{\"status\":\"error\",\"message\":\"Invalid API Key\"}";

void checkApiKey(std::function<void()> handler) {
  if (!server.hasHeader("Authorization")) {
    server.send(401, "application/json", invalidApiKey);
    return;
  }

  String auth = server.header("Authorization");
  if (auth != apiKey) {
    server.send(401, "application/json", invalidApiKey);
    return;
  }

  handler();
}

void handleConfig() {
  JsonDocument doc;

  doc["lock"] = data.lock;
  doc["state"] = data.state;
  doc["mode"] = data.mode;

  JsonArray brightArray = doc.createNestedArray("bright");
  for (int i = 0; i < 3; i++) {
    brightArray.add(data.bright[i]);
  }
  JsonArray valueArray = doc.createNestedArray("value");
  for (int i = 0; i < 3; i++) {
    valueArray.add(data.value[i]);
  }

  String jsonResponse;
  serializeJson(doc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
}

void handleLockToggle() {
  Preferences prefs;
  data.lock = !data.lock;
  prefs.begin("mega-lamp", false);
  prefs.putBool("lock", data.lock);
  prefs.end();

  server.send(200, "application/json", successResponse);
}

void handleStateToggle() {
  data.state = !data.state;
  applyMode();

  server.send(200, "application/json", successResponse);
}

void handleByteArray(String type) {
  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", invalidResponse);
    return;
  }

  JsonArray newArray = doc[type];
  if (newArray.size() != 3) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid array size\"}");
    return;
  }

  for (int i = 0; i < 3; i++) {
    if (type == "color") {
      data.value[i] = newArray[i];
    } else {
      data.bright[i] = newArray[i];
    }
  }
  applyMode();

  server.send(200, "application/json", successResponse);
}

void handleBrightness() {
  handleByteArray("bright");
}

void handleColor() {
  handleByteArray("color");
}

void handleMode() {
  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", invalidResponse);
    return;
  }

  int mode = doc["mode"];
  data.mode = mode;
  applyMode();

  server.send(200, "application/json", successResponse);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

int prev_br;

void setup1(void) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/settings/config", HTTP_GET, []() {
    checkApiKey(handleConfig);
  });

  server.on("/settings/lock", HTTP_POST, []() {
    checkApiKey(handleLockToggle);
  });

  server.on("/settings/state", HTTP_POST, []() {
    checkApiKey(handleStateToggle);
  });

  server.on("/settings/bright", HTTP_POST, []() {
    checkApiKey(handleBrightness);
  });

  server.on("/settings/color", HTTP_POST, []() {
    checkApiKey(handleColor);
  });

  server.on("/settings/mode", HTTP_POST, []() {
    checkApiKey(handleMode);
  });

  server.onNotFound(handleNotFound);

  server.begin();
}

void setup(void) {
  Preferences prefs;
  delay(3000);
  Serial.begin(115200);

  // LED далее
  pinMode(HC_TRIG, OUTPUT);  // trig выход
  pinMode(HC_ECHO, INPUT);   // echo вход

  // FastLED
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_NUM);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MA);
  FastLED.setBrightness(255);

  led.setBrightness(0);
  led.attach(setLED);
  led.setCRT(1);

  prefs.begin("mega-lamp", true);
  data.state = prefs.getBool("state", data.state);
  data.lock = prefs.getBool("lock", data.lock);
  data.mode = prefs.getUChar("mode", data.mode);
  prefs.getBytes("bright", data.bright, sizeof(data.bright));
  prefs.getBytes("value", data.value, sizeof(data.value));
  prefs.end();

  applyMode();  // применить режим
}

void loop1() {
  server.handleClient();  // обработка запросов
}

void loop() {
  if (data.state && data.mode == 2 && !gest.hasClicks()) fireTick();  // анимация огня

  // таймер 50мс, опрос датчика и вся основная логика
  static uint32_t tmr;
  if (millis() - tmr >= 50 && !data.lock) {
    tmr = millis();
    // Serial.println(tmr);

    static uint32_t tout;  // таймаут настройки (удержание)
    static int offset_d;   // оффсеты для настроек
    static byte offset_v;

    int dist = getDist(HC_TRIG, HC_ECHO);  // получаем расстояние
    dist = getFilterMedian(dist);          // медиана
    dist = getFilterSkip(dist);            // пропускающий фильтр
    int dist_f = getFilterExp(dist);       // усреднение

    gest.tick(dist > 0 ? 1 : 0);  // расстояние > 0 - это клик

    // есть клики и прошло 2 секунды после настройки (удержание)
    if (gest.hasClicks() && millis() - tout > 2000) {
      switch (gest.getClicks()) {
        case 1:
          data.state = !data.state;  // вкл/выкл
          break;
        case 2:
          // если включена И меняем режим (0.. 2)
          if (data.state && ++data.mode >= 3) data.mode = 0;
          break;
      }
      applyMode();
    }

    // клик
    if (gest.hasClicks() && data.state) {
      pulse();  // мигнуть яркостью
    }

    // удержание (выполнится однократно)
    if (gest.hold() && data.state) {
      pulse();            // мигнуть яркостью
      offset_d = dist_f;  // оффсет расстояния для дальнейшей настройки
      switch (gest.getClicks()) {
        case 0: offset_v = data.bright[data.mode]; break;  // оффсет яркости
        case 1: offset_v = data.value[data.mode]; break;   // оффсет значения
      }
    }

    // удержание (выполнится пока удерживается)
    if (gest.holding() && data.state) {
      tout = millis();
      // смещение текущей настройки как оффсет + (текущее расстояние - расстояние начала)
      int shift = constrain(offset_v + (dist_f - offset_d), 0, 255);

      // применяем
      switch (gest.getClicks()) {
        case 0: data.bright[data.mode] = shift; break;
        case 1: data.value[data.mode] = shift; break;
      }
      applyMode();
    }
  }
}

// получение расстояния с дальномера
#define HC_MAX_LEN 1000L  // макс. расстояние измерения, мм
int getDist(byte trig, byte echo) {
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // измеряем время ответного импульса
  uint32_t us = pulseIn(echo, HIGH, (HC_MAX_LEN * 2 * 1000 / 343));

  // считаем расстояние и возвращаем
  return (us * 343L / 2000);
}

// медианный фильтр
int getFilterMedian(int newVal) {
  static int buf[3];
  static byte count = 0;
  buf[count] = newVal;
  if (++count >= 3) count = 0;
  return (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
}

// пропускающий фильтр
#define FS_WINDOW 7  // количество измерений, в течение которого значение не будет меняться
#define FS_DIFF 80   // разница измерений, с которой начинается пропуск
int getFilterSkip(int val) {
  static int prev;
  static byte count;

  if (!prev && val) prev = val;  // предыдущее значение 0, а текущее нет. Обновляем предыдущее
  // позволит фильтру резко срабатывать на появление руки

  // разница больше указанной ИЛИ значение равно 0 (цель пропала)
  if (abs(prev - val) > FS_DIFF || !val) {
    count++;
    // счётчик потенциально неправильных измерений
    if (count > FS_WINDOW) {
      prev = val;
      count = 0;
    } else val = prev;
  } else count = 0;  // сброс счётчика
  prev = val;

  return val;
}

// экспоненциальный фильтр со сбросом снизу
#define ES_EXP 3L    // коэффициент плавности (больше - плавнее)
#define ES_MULT 16L  // мультипликатор повышения разрешения фильтра
int getFilterExp(int val) {
  static long filt;
  if (val) filt += (val * ES_MULT - filt) / ES_EXP;
  else filt = 0;  // если значение 0 - фильтр резко сбрасывается в 0
  // в нашем случае - чтобы применить заданную установку и не менять её вниз к нулю
  return filt / ES_MULT;
}

#define BR_REVERSE_LIMIT 180
#define BR_STEP 4
void applyMode() {
  Preferences prefs;
  if (data.state) {
    switch (data.mode) {
      case 0: led.setWheel8(data.value[0]); break;
      case 1: led.setKelvin(data.value[1] * 28); break;
    }

    // плавная смена яркости при ВКЛЮЧЕНИИ и СМЕНЕ РЕЖИМА
    if (prev_br != data.bright[data.mode]) {
      int shift = prev_br > data.bright[data.mode] ? -BR_STEP : BR_STEP;
      while (abs(prev_br - data.bright[data.mode]) > BR_STEP) {
        prev_br += shift;
        led.setBrightness(prev_br);
        delay(10);
      }
      prev_br = data.bright[data.mode];
    }
  } else {
    // плавная смена яркости при ВЫКЛЮЧЕНИИ
    while (prev_br > 0) {
      prev_br -= BR_STEP;
      if (prev_br < 0) prev_br = 0;
      led.setBrightness(prev_br);
      delay(10);
    }
  }

  prefs.begin("mega-lamp", false);
  prefs.putBool("state", data.state);
  prefs.putUChar("mode", data.mode);
  prefs.putBytes("bright", data.bright, sizeof(data.bright));
  prefs.putBytes("value", data.value, sizeof(data.value));
  prefs.end();
}

void setLED() {
  FastLED.showColor(CRGB(led.R, led.G, led.B));
}

// огненный эффект
void fireTick() {
  static uint32_t rnd_tmr, move_tmr;
  static int rnd_val, fil_val;

  // таймер 100мс, генерирует случайные значения
  if (millis() - rnd_tmr > 100) {
    rnd_tmr = millis();
    rnd_val = random(0, 13);
  }

  // таймер 20мс, плавно движется к rnd_val
  if (millis() - move_tmr > 20) {
    move_tmr = millis();
    // эксп фильтр, на выходе получится число 0..120
    fil_val += (rnd_val * 10 - fil_val) / 5;

    // преобразуем в яркость от 100 до 255
    int br = map(fil_val, 0, 120, 100, 255);

    // преобразуем в цвет как текущий цвет + (0.. 24)
    int hue = data.value[2] + fil_val / 5;
    led.setWheel8(hue, br);
  }
}

// Функция для плавного изменения яркости от start до end с заданным шагом
void changeBrightness(int start, int end, int step) {
  for (int i = start; (step > 0) ? i <= end : i >= end; i += step) {
    int brightness = constrain(i, 0, 255);  // Ограничиваем яркость в пределах 0–255
    led.setBrightness(brightness);          // Устанавливаем яркость
    delay(5);                               // Задержка для плавности
  }
}

// подмигнуть яркостью
void pulse() {
  const int delta = 45;
  const int step = 3;

  if (prev_br > BR_REVERSE_LIMIT) {
    // Уменьшаем яркость на delta, но не ниже 0
    int target = max(prev_br - 2 * delta, 0);
    changeBrightness(prev_br, target, -2 * step);
    changeBrightness(target, prev_br, 2 * step);  // Возврат к исходной
  } else {
    // Увеличиваем яркость на delta, но не выше 255
    int target = min(prev_br + delta, 255);
    changeBrightness(prev_br, target, step);
    changeBrightness(target, prev_br, -step);  // Возврат к исходной
  }
}
