#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include "DHT.h"

// Configuração do serviço de provisionamento
const char *SERVICE_NAME = "BPK-ALUNOS";
const char *POP = "2020alunos";

// Pinos dos relés
#define RELAY_1 23  // D23
#define RELAY_2 22  // D22
#define RELAY_3 21  // D21
#define RELAY_4 19  // D19

// LED Wi-Fi
#define WIFI_LED 2  // D2

// Pinos dos sensores
#define LDR_PIN 33             // D33
#define GAS_DIGITAL_PIN 32     // D32
#define GAS_ANALOG_PIN 35      // D35
#define DHT_PIN 34             // D34
#define DHT_TYPE DHT11         // Tipo do sensor DHT11

// Estados dos relés
bool stateRelay1 = false;
bool stateRelay2 = false;
bool stateRelay3 = false;
bool stateRelay4 = false;

// Instância do sensor DHT
DHT dht(DHT_PIN, DHT_TYPE);

// Função para controle de relés
void controlRelay(int relayPin, bool &state, const char *name) {
    state = !state;
    digitalWrite(relayPin, state ? HIGH : LOW);
    Serial.printf("%s está %s\n", name, state ? "LIGADO" : "DESLIGADO");
}

// Callback para gerenciar mudanças nos dispositivos
void writeCallback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
    const char *deviceName = device->getDeviceName();
    if (strcmp(deviceName, "Relay1") == 0) {
        controlRelay(RELAY_1, stateRelay1, "Relay1");
    } else if (strcmp(deviceName, "Relay2") == 0) {
        controlRelay(RELAY_2, stateRelay2, "Relay2");
    } else if (strcmp(deviceName, "Relay3") == 0) {
        controlRelay(RELAY_3, stateRelay3, "Relay3");
    } else if (strcmp(deviceName, "Relay4") == 0) {
        controlRelay(RELAY_4, stateRelay4, "Relay4");
    }
}

// Evento de provisionamento
void sysProvEvent(arduino_event_t *sys_event) {
    if (sys_event->event_id == ARDUINO_EVENT_PROV_START) {
        Serial.printf("\nProvisionando com nome \"%s\" e PoP \"%s\"...\n", SERVICE_NAME, POP);
    } else if (sys_event->event_id == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
        Serial.println("\nConectado ao Wi-Fi!");
        digitalWrite(WIFI_LED, HIGH);
    }
}

void setup() {
    // Iniciar serial
    Serial.begin(115200);

    // Configurar pinos como saída
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(RELAY_3, OUTPUT);
    pinMode(RELAY_4, OUTPUT);
    pinMode(WIFI_LED, OUTPUT);

    // Configurar pinos dos sensores
    pinMode(GAS_DIGITAL_PIN, INPUT);

    // Inicializar relés em estado desligado
    digitalWrite(RELAY_1, LOW);
    digitalWrite(RELAY_2, LOW);
    digitalWrite(RELAY_3, LOW);
    digitalWrite(RELAY_4, LOW);
    digitalWrite(WIFI_LED, LOW);

    // Iniciar sensor DHT
    dht.begin();

    // Configurar dispositivos ESP RainMaker
    Node myNode = RMaker.initNode("ESP32 Node");
    Switch relay1("Relay1", &RELAY_1);
    Switch relay2("Relay2", &RELAY_2);
    Switch relay3("Relay3", &RELAY_3);
    Switch relay4("Relay4", &RELAY_4);

    relay1.addCb(writeCallback);
    relay2.addCb(writeCallback);
    relay3.addCb(writeCallback);
    relay4.addCb(writeCallback);

    myNode.addDevice(relay1);
    myNode.addDevice(relay2);
    myNode.addDevice(relay3);
    myNode.addDevice(relay4);

    // Habilitar OTA e serviços adicionais
    RMaker.enableOTA(OTA_USING_PARAMS);
    RMaker.enableTZService();
    RMaker.enableSchedule();

    // Iniciar ESP RainMaker
    RMaker.start();

    // Configurar provisão Wi-Fi
    WiFi.onEvent(sysProvEvent);
#if CONFIG_IDF_TARGET_ESP32
    WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM, NETWORK_PROV_SECURITY_1, POP, SERVICE_NAME);
#else
    WiFiProv.beginProvision(NETWORK_PROV_SCHEME_SOFTAP, NETWORK_PROV_SCHEME_HANDLER_NONE, NETWORK_PROV_SECURITY_1, POP, SERVICE_NAME);
#endif
}

void loop() {
    // Atualizar estado do LED Wi-Fi
    digitalWrite(WIFI_LED, WiFi.status() == WL_CONNECTED ? HIGH : LOW);

    // Leitura do sensor LDR
    int ldrValue = analogRead(LDR_PIN);
    Serial.printf("LDR Value: %d\n", ldrValue);

    // Leitura do sensor de gás
    int gasDigital = digitalRead(GAS_DIGITAL_PIN);
    int gasAnalog = analogRead(GAS_ANALOG_PIN);
    Serial.printf("Gas (Digital): %d, Gas (Analog): %d\n", gasDigital, gasAnalog);

    // Leitura do sensor de umidade e temperatura
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Falha ao ler o sensor DHT!");
    } else {
        Serial.printf("Umidade: %.2f%%, Temperatura: %.2f°C\n", humidity, temperature);
    }

    delay(2000);
}
