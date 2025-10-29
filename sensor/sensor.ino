// Sensor_ESP32_EnvioDistancia_POO.ino
#include <WiFi.h>

class DistanceSensorClient {
public:
  DistanceSensorClient(const char* ssid, const char* pass,
                       const char* host, int port,
                       int trigPin, int echoPin,
                       unsigned long measureIntervalMs = 1000)
  : _ssid(ssid), _pass(pass), _host(host), _port(port),
    _trig(trigPin), _echo(echoPin),
    _measIv(measureIntervalMs) {}

  void begin() {
    Serial.begin(115200);
    pinMode(_trig, OUTPUT);
    pinMode(_echo, INPUT);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _pass);

    _tWifi = _tSrv = _tMeas = millis();
  }

  void loop() {
    const unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED && (now - _tWifi >= WIFI_RETRY_MS)) {
      _tWifi = now;
      Serial.println(F("[NET] WiFi reconnect()"));
      WiFi.reconnect();
    }

    if (WiFi.status() == WL_CONNECTED && !_client.connected() && (now - _tSrv >= SRV_RETRY_MS)) {
      _tSrv = now;
      Serial.println(F("[NET] Conectando a servidor..."));
      if (_client.connect(_host, _port)) {
        _client.println(F("SENSOR"));
        Serial.println(F("[NET] SENSOR registrado"));
      }
    }

    if (now - _tMeas >= _measIv) {
      _tMeas = now;

      float d = measureCM();
      Serial.print(F("Distancia: ")); Serial.print(d, 2); Serial.println(F(" cm"));

      if (isnan(d) || d < 0) d = 1000.0f;

      if (_client.connected()) {
        _client.print(F("DIST:"));
        _client.println(String(d, 2));
      }
    }

    yield();
  }

private:
  static constexpr unsigned long WIFI_RETRY_MS = 5000;
  static constexpr unsigned long SRV_RETRY_MS  = 3000;

  const char* _ssid; const char* _pass; const char* _host; int _port;
  const int _trig, _echo;
  WiFiClient _client;
  unsigned long _tWifi = 0, _tSrv = 0, _tMeas = 0;
  const unsigned long _measIv;

  float measureCM() {
    digitalWrite(_trig, LOW);  delayMicroseconds(2);
    digitalWrite(_trig, HIGH); delayMicroseconds(10);
    digitalWrite(_trig, LOW);

    long us = pulseIn(_echo, HIGH, 40000UL);
    if (us <= 0) return NAN;

    return (us * 0.034f) * 0.5f;
  }
};

static constexpr const char* WIFI_SSID = "UCB-IoT";
static constexpr const char* WIFI_PASS = "sistemasyteleco";
static constexpr const char* HOST_IP   = "192.168.50.200";
static constexpr int         HOST_PORT = 5050;

DistanceSensorClient sensor(WIFI_SSID, WIFI_PASS, HOST_IP, HOST_PORT,
                            /*TRIG*/ 25, /*ECHO*/ 26,
                            /*cada*/ 1000);

void setup() { sensor.begin(); }
void loop()  { sensor.loop();  }
