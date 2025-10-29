// Actuator_ESP32_POO_25_26_27_keepalive.ino
#include <WiFi.h>

class ActuatorESP32 {
public:
  ActuatorESP32(const char* ssid, const char* pass,
                const char* host, int port,
                uint8_t led1, uint8_t led2, uint8_t led3)
  : _ssid(ssid), _pass(pass), _host(host), _port(port),
    _l1(led1), _l2(led2), _l3(led3) {}

  void begin() {
    Serial.begin(115200);
    pinMode(_l1, OUTPUT); pinMode(_l2, OUTPUT); pinMode(_l3, OUTPUT);
    allOff();

    // ---- Wi-Fi (solo una vez) ----
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _pass);

    _tWifi = _tSrv = _tPing = millis();
  }

  void loop() {
    const unsigned long now = millis();

    // ---- Reintento Wi-Fi no bloqueante ----
    if (WiFi.status() != WL_CONNECTED && (now - _tWifi >= WIFI_RETRY_MS)) {
      _tWifi = now;
      Serial.println(F("[NET] WiFi reconnect()"));
      WiFi.reconnect();              // NO llamar begin() repetidamente en loop
    }

    // ---- Reintento TCP si hay Wi-Fi ----
    if (WiFi.status() == WL_CONNECTED && !_client.connected() && (now - _tSrv >= SRV_RETRY_MS)) {
      _tSrv = now;
      Serial.println(F("[NET] Conectando a servidor..."));
      if (_client.connect(_host, _port)) {
        _client.setNoDelay(true);    // menor latencia de Nagle
        _client.println(F("ACTUATOR"));
        Serial.println(F("[NET] ACTUATOR registrado"));
      }
    }

    // ---- Heartbeat para evitar cierres por inactividad ----
    if (_client.connected() && (now - _tPing >= PING_MS)) {
      _tPing = now;
      _client.println(F("PING"));
    }

    // ---- Lectura no bloqueante de comandos ----
    readFromServer();

    yield(); // alimenta el WDT
  }

private:
  // Timers (ajusta si quieres)
  static constexpr unsigned long WIFI_RETRY_MS = 5000;
  static constexpr unsigned long SRV_RETRY_MS  = 3000;
  static constexpr unsigned long PING_MS       = 15000;

  // Net cfg
  const char* _ssid; const char* _pass; const char* _host; int _port;

  // Hardware
  const uint8_t _l1, _l2, _l3;

  // Estado
  WiFiClient _client;
  unsigned long _tWifi = 0, _tSrv = 0, _tPing = 0;
  String _buf;

  void allOff() {
    digitalWrite(_l1, LOW);
    digitalWrite(_l2, LOW);
    digitalWrite(_l3, LOW);
  }

  void handleCmd(const String& raw) {
    String cmd = raw; cmd.trim();
    if (!cmd.length() || cmd == F("PING")) return; // ignorar heartbeats

    allOff(); // un solo LED a la vez

    if      (cmd == F("LED1")) { digitalWrite(_l1, HIGH); Serial.println(F("[CMD] LED1")); }
    else if (cmd == F("LED2")) { digitalWrite(_l2, HIGH); Serial.println(F("[CMD] LED2")); }
    else if (cmd == F("LED3")) { digitalWrite(_l3, HIGH); Serial.println(F("[CMD] LED3")); }
    else if (cmd == F("OFF"))  { Serial.println(F("[CMD] OFF")); }
    else {
      Serial.print(F("[WARN] Cmd desconocido: "));
      Serial.println(cmd);
    }
  }

  void readFromServer() {
    while (_client.connected() && _client.available()) {
      char c = (char)_client.read();
      if (c == '\r') continue;
      if (c == '\n') {
        handleCmd(_buf);
        _buf = "";
      } else {
        if (_buf.length() < 128) _buf += c; else _buf = ""; // anti-overflow simple
      }
    }
  }
};

// ======== CONFIG REAL (ajusta HOST_IP con la IP LAN de tu Mac) ========
static constexpr const char* WIFI_SSID = "UCB-IoT";
static constexpr const char* WIFI_PASS = "sistemasyteleco";
static constexpr const char* HOST_IP   = "192.168.50.200"; // ej. "192.168.1.23"
static constexpr int         HOST_PORT = 5050;

// Pines del ACTUADOR en ESP32 (tu pedido): LED1=25, LED2=26, LED3=27
ActuatorESP32 app(WIFI_SSID, WIFI_PASS, HOST_IP, HOST_PORT, 25, 26, 27);

void setup() { app.begin(); }
void loop()  { app.loop();  }
