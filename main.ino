
#include <DHT.h>
#include <math.h>
#include <WiFi.h>
#include <DHT_U.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>                       

#define DHTTYPE    DHT11      
#define DHTPIN     32                // Sensor DHT11
#define MQ2pin     33

uint32_t delayMS;     

const char* ssid     = "";
const char* password = "";

const char* mqtt_server = "";   
const int   mqtt_port   = 1883;             // porta do plugin MQTT

const char* mqtt_user   = ""; 
const char* mqtt_pass   = ""; 

// tópicos/filas
const char* topic_temp   = "sensor.temperatura";
const char* topic_umid   = "sensor.umidade";
const char* topic_fuma   = "sensor.fumaca";

WiFiClient espClient;
PubSubClient client(espClient);
DHT_Unified dht(DHTPIN, DHTTYPE);

void connectWiFi() {
  Serial.print("Conectando ao WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT/RabbitMQ... ");
    String clientId = "esp32-publisher-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("conectado!");

    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando de novo em 5s...");
      delay(5000);
    }
  }
}

float ler_temperatura(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Erro na leitura da Temperatura!");
    return -1;
  }
  return event.temperature;
}

float ler_umidade(){
  sensors_event_t event;                        
  dht.humidity().getEvent(&event);              
  if (isnan(event.relative_humidity))           
  {
    Serial.println("Erro na leitura da Umidade!");
    return -1;
  }
  
  return event.relative_humidity;
}

float ler_fumaca(){
  return analogRead(MQ2pin);
}

void setup() {
  Serial.begin(115200);
  sensor_t sensor;
  dht.begin();  
  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  delayMS = sensor.min_delay / 1000;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi caiu, reconectando...");
    connectWiFi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(delayMS);                             
  
  char buffer[32];

  float umidade, temperatura, fumaca;

  umidade = ler_umidade();
  temperatura = ler_temperatura();
  fumaca = ler_fumaca();

  // temperatura  
  dtostrf(temperatura, 0, 2, buffer);   
  client.publish(topic_temp, buffer, true); 

  // umidade
  dtostrf(umidade, 0, 2, buffer);
  client.publish(topic_umid, buffer, true);
  
  // fumaca
  dtostrf(fumaca, 0, 2, buffer);
  client.publish(topic_fuma, buffer, true);

  Serial.print("[PUB] temp="); Serial.print(temperatura);
  Serial.print(" umid="); Serial.print(umidade);
  Serial.print(" fuma="); Serial.println(fumaca);
  delay(5000);
}
