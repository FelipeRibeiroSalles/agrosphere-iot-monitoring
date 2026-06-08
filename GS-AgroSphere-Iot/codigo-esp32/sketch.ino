
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHTesp.h"

// =====================================
// WIFI
// =====================================

const char* ssid = "Wokwi-GUEST";
const char* password = "";

// =====================================
// MQTT
// =====================================

const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// =====================================
// OLED
// =====================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =====================================
// DHT22
// =====================================

const int DHT_PIN = 15;
DHTesp dhtSensor;

// =====================================
// LEDS
// =====================================

const int LED_VERDE = 18;
const int LED_VERMELHO = 19;

// =====================================
// SOLO
// =====================================

const int SOLO_PIN = 34;

// =====================================
// VARIÁVEIS
// =====================================

float temperatura;
float umidade;

int solo;

String statusClimatico;

// =====================================
// WIFI
// =====================================

void setup_wifi() {

  delay(10);

  Serial.println();
  Serial.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.println(WiFi.localIP());
}

// =====================================
// MQTT
// =====================================

void reconnectMQTT() {

  while (!client.connected()) {

    Serial.print("Conectando MQTT...");

    if (client.connect("AgroSphereESP32")) {

      Serial.println("conectado!");

    } else {

      Serial.print("erro=");
      Serial.print(client.state());
      Serial.println(" tentando novamente...");
      delay(2000);
    }
  }
}

// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);

  // WIFI
  setup_wifi();

  // MQTT
  client.setServer(mqtt_server, 1883);

  // DHT22
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  // LEDS
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("Erro OLED");
    while(true);
  }

  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(2);

  display.setCursor(10,20);
  display.println("AgroSphere");

  display.display();

  delay(2000);
}

// =====================================
// LOOP
// =====================================

void loop() {

  // MQTT
  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  // =========================
  // LEITURA SENSOR
  // =========================

  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  temperatura = data.temperature;
  umidade = data.humidity;

  solo = analogRead(SOLO_PIN);

  // =========================
  // LÓGICA CLIMÁTICA
  // =========================

  bool alertaTemp = temperatura > 35;
  bool alertaSolo = solo < 1500;

  if(alertaTemp || alertaSolo) {

    statusClimatico = "RISCO";

    digitalWrite(LED_VERMELHO, HIGH);
    digitalWrite(LED_VERDE, LOW);

  } else {

    statusClimatico = "ESTAVEL";

    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
  }

  // =========================
  // OLED
  // =========================

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0,0);
  display.print("Temp: ");
  display.print(temperatura);
  display.println(" C");

  display.setCursor(0,15);
  display.print("Umidade: ");
  display.print(umidade);
  display.println("%");

  display.setCursor(0,30);
  display.print("Solo: ");
  display.println(solo);

  display.setCursor(0,50);

  if(statusClimatico == "RISCO") {

    display.print("ALERTA CLIMATICO");

  } else {

    display.print("AMBIENTE ESTAVEL");
  }

  display.display();

  // =========================
  // JSON MQTT
  // =========================

  String payload = "{";
  payload += "\"temperatura\":" + String(temperatura) + ",";
  payload += "\"umidade\":" + String(umidade) + ",";
  payload += "\"solo\":" + String(solo) + ",";
  payload += "\"status\":\"" + statusClimatico + "\"";
  payload += "}";

  // =========================
  // PUBLICAÇÃO MQTT
  // =========================

  client.publish("agrosphere/dados", payload.c_str());

  Serial.println(payload);

  delay(2000);
}
