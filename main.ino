#include <DHT.h>
#include <math.h>
#include <WiFi.h>
#include <DHT_U.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>

#define DHTTYPE    DHT11
#define DHTPIN     32
#define MQ2PIN     33
#define Dsm501aPIN 23 // PM2.5 via divisor de tensão

// ==== TEMPOS ====
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms      = 10000;   // 
unsigned long lowpulseoccupancy  = 0;

float ratio         = 0.0;
float concentration = 0.0;

const char* ssid     = "Fred_2G";
const char* password = "8532533637";

const char* mqtt_server = "82.25.77.19";
const int   mqtt_port   = 1883;

const char* mqtt_user   = "admin";
const char* mqtt_pass   = "647431b5ca55b04fdf3c2fce31ef1915";

// tópicos
const char* topic_temp   = "sensor.temperatura";
const char* topic_umid   = "sensor.umidade";
const char* topic_fuma   = "sensor.fumaca";
const char* topic_gas    = "sensor.gas";

WiFiClient espClient;
PubSubClient client(espClient);
DHT_Unified dht(DHTPIN, DHTTYPE);

// ======== FUNÇÕES AUXILIARES ========

void connectWiFi() {
  Serial.print("Conectando ao WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT... ");

    String clientId = "esp32-client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("conectado!");
    } else {
      Serial.print("falhou rc=");
      Serial.print(client.state());
      Serial.println(" tentando de novo em 5s...");
      delay(5000);
    }
  }
}

float ler_temperatura(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) return -1;
  return event.temperature;
}

float ler_umidade(){
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) return -1;
  return event.relative_humidity;
}

float ler_gas(){
  return analogRead(MQ2PIN);
}

void setup() {
  Serial.begin(115200);
  pinMode(Dsm501aPIN, INPUT);
  dht.begin();

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);

  delay(30000); // Warm-up DSM501A
}

void loop() {

  // ===== 1) INICIA JANELA DE 15s =====
  lowpulseoccupancy = 0;
  starttime = millis();

  // ===== 2) FICA 15s MEDINDO SOMENTE O DSM501A =====
  while ((millis() - starttime) < sampletime_ms) {

    duration = pulseIn(Dsm501aPIN, LOW, sampletime_ms * 1000UL);
    lowpulseoccupancy += duration;
  }

  // ===== 3) APÓS OS 15s: CALCULA O RESULTADO =====
  ratio = lowpulseoccupancy / (sampletime_ms * 10.0);

  concentration =
        1.1 * pow(ratio, 3)
      - 3.8 * pow(ratio, 2)
      + 520 * ratio
      + 0.62;

  Serial.println("========== DSM501A ==========");
  Serial.print("LowPulseOccupancy(us): ");
  Serial.println(lowpulseoccupancy);
  Serial.print("Ratio(%): ");
  Serial.println(ratio);
  Serial.print("Concentracao: ");
  Serial.println(concentration);
  Serial.println("============================\n");

  // ===== 4) DEPOIS DOS 15s → AÍ SIM FAZ O RESTO =====
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!client.connected()) reconnect();
  client.loop();

  char buffer[32];

  float umidade     = ler_umidade();
  float temperatura = ler_temperatura();
  float gas         = ler_gas();

  // Publicação

  dtostrf(temperatura, 0, 2, buffer);
  client.publish(topic_temp, buffer, true);

  dtostrf(umidade, 0, 2, buffer);
  client.publish(topic_umid, buffer, true);

  dtostrf(concentration, 0, 2, buffer);
  client.publish(topic_fuma, buffer, true);

  dtostrf(gas, 0, 2, buffer);
  client.publish(topic_gas, buffer, true);

  Serial.print("[PUB] temp="); Serial.print(temperatura);
  Serial.print(" umid=");      Serial.print(umidade);
  Serial.print(" fuma=");      Serial.print(concentration);
  Serial.print(" gas=");       Serial.println(gas);

  // volta pro loop e recomeça tudo
}
