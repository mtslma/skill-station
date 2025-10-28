#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>

// Definições de Avaliação
#define boardLED 2 
const int LED_PINS[] = {16, 17, 18, 19, 21}; 
const int BTN_PINS[] = {22, 23, 25, 26, 27};
const char* LABELS[] = {"pessimo", "ruim", "regular", "bom", "otimo"};
const char* DISPLAY_LABELS[] = {"Péssimo", "Ruim", "Regular", "Bom", "Ótimo"}; 
const int NUM_RATES = 5;

// Identificadores padrão
String setorAvaliado = "Setor_01"; 
String localizacao = "Totem_01";

// Wi-Fi
const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";

// MQTT Broker (Local)
const char* BROKER_MQTT = "host.wokwi.internal"; 
const int BROKER_PORT = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
#define TOPICO_PUBLISH "2TDS/esp32/avaliacoes"

// Variáveis Globais
WiFiClient espClient;
PubSubClient MQTT(espClient);
JsonDocument doc; 
char buffer[256]; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000); 

WebServer server(80); // Instância do Servidor Web

unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200; 

// Conexão Wi-Fi (Sem mudanças)
void initWiFi() {
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Conectando ao Wi-Fi (Wokwi-GUEST)");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// Conexão MQTT
void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    while (!MQTT.connected()) {
        Serial.println("Conectando ao Broker MQTT Local (host.wokwi.internal)...");
        if (MQTT.connect(localizacao.c_str())) { 
            Serial.println("Conectado ao Broker Local (Mosquitto)!");
        } else {
            Serial.print("Falha na conexão. Estado: ");
            Serial.println(MQTT.state());
            delay(2000);
        }
    }
}

void verificaConexoesWiFiEMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconectando Wi-Fi...");
        initWiFi();
    }
    if (!MQTT.connected()) {
        initMQTT();
    }
    MQTT.loop();
}

// Lógica de Envio MQTT
void piscaLed(int ledPin) {
    digitalWrite(ledPin, HIGH);
    digitalWrite(boardLED, HIGH); 
    delay(500); 
    digitalWrite(ledPin, LOW);
    digitalWrite(boardLED, LOW);
}

void sendMqttEvaluation(int index) {
    timeClient.update();
    String formattedTime = timeClient.getFormattedTime();
    const char* label = LABELS[index];
    const char* displayLabel = DISPLAY_LABELS[index];

    // Construindo o corpo que será enviado <- na hora de integrar com a API Java isso aqui vai ser importante
    doc.clear();
    doc["setor"] = setorAvaliado;
    doc["local"] = localizacao;
    doc["horario"] = formattedTime;
    doc["avaliacao"] = label;

    serializeJson(doc, buffer);

    Serial.printf("\n*** AVALIAÇÃO RECEBIDA: %s ***\n", displayLabel);
    Serial.print("Enviando MQTT: ");
    Serial.println(buffer);

    MQTT.publish(TOPICO_PUBLISH, buffer);
    
    piscaLed(LED_PINS[index]);
}

// Lógica de Botões
void handleButtons() {
    unsigned long currentMillis = millis();
    for (int i = 0; i < NUM_RATES; i++) {
        if (digitalRead(BTN_PINS[i]) == LOW) { 
            if (currentMillis - lastButtonPress > debounceDelay) {
                sendMqttEvaluation(i);
                lastButtonPress = currentMillis;
            }
        }
    }
}

// FUNÇÕES DO WEB SERVER 

// Página principal
void handleRoot() {
  String html = R"(
    <!DOCTYPE html><html><head><title>Configurar Totem</title>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <meta charset='UTF-8'>
    <style>
      * { box-sizing: border-box; }
      html, body {
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
        background-color: #f0f2f5;
        margin: 0; padding: 0;
        display: flex;
        align-items: center;
        justify-content: center;
        min-height: 100vh;
        color: #333;
      }
      .container {
        background: #ffffff;
        border-radius: 12px;
        box-shadow: 0 6px 16px rgba(0,0,0,0.1);
        padding: 2.0em;
        width: 100%;
        max-width: 500px;
        margin: 1em;
      }
      h1 {
        color: #1a1a1a;
        margin-top: 0;
        margin-bottom: 1.0em;
        font-size: 1.8em;
        text-align: center;
      }
      .status-box {
        background: #e6f7ff;
        border: 1px solid #b3e0ff;
        padding: 15px;
        border-radius: 8px;
        margin-bottom: 1.5em;
        line-height: 1.6;
      }
      form label {
        display: block;
        margin-top: 1.2em;
        margin-bottom: 0.5em;
        font-weight: 600;
        color: #555;
      }
      form input[type='text'] {
        width: 100%;
        padding: 12px;
        border: 1px solid #ccd0d5;
        border-radius: 6px;
        font-size: 1em;
      }
      form input[type='text']:focus {
         border-color: #007bff;
         outline: none;
         box-shadow: 0 0 0 2px rgba(0,123,255,0.25);
      }
      form input[type='submit'] {
        background: #007bff;
        color: white;
        padding: 12px 20px;
        border: none;
        border-radius: 6px;
        cursor: pointer;
        margin-top: 1.5em;
        font-size: 1.1em;
        font-weight: 600;
        width: 100%;
        transition: background-color 0.2s;
      }
      form input[type='submit']:hover { background: #0056b3; }
    </style>
    </head><body>
    <div class='container'>
      <h1>Configurar Totem</h1>
      <div class='status-box'>
        Setor Atual: <strong>)";
  html += setorAvaliado;
  html += R"(</strong><br>Local Atual: <strong>)";
  html += localizacao;
  html += R"(</strong>
      </div>
      <form action='/salvar' method='POST'>
        <label for='setor'>Nome do Setor:</label>
        <input type='text' id='setor' name='setor' value=')";
  html += setorAvaliado;
  html += R"('>
        <label for='local'>ID/Local do Totem:</label>
        <input type='text' id='local' name='local' value=')";
  html += localizacao;
  html += R"('><br>
        <input type='submit' value='Salvar Configurações'>
      </form>
    </div>
    </body></html>)";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

// Função chamada pelo formulário para salvar os dados
void handleSave() {
  setorAvaliado = server.arg("setor"); // Pega o valor do campo 'setor'
  localizacao = server.arg("local");   // Pega o valor do campo 'local'

  Serial.println("CONFIGURAÇÃO ATUALIZADA VIA WEB");
  Serial.println("Novo Setor: " + setorAvaliado);
  Serial.println("Novo Local: " + localizacao);
  
  // Resposta de sucesso
  String html = R"(
    <!DOCTYPE html><html><head><title>Salvo!</title>
    <meta http-equiv='refresh' content='3;url=/' />
    <meta charset='UTF-8'>
    <style>
      * { box-sizing: border-box; }
      html, body {
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
        background-color: #f0f2f5;
        margin: 0; padding: 0;
        display: flex;
        align-items: center;
        justify-content: center;
        min-height: 100vh;
        color: #333;
        text-align: center;
      }
      .container {
        background: #ffffff;
        border-radius: 12px;
        box-shadow: 0 6px 16px rgba(0,0,0,0.1);
        padding: 2.5em;
        width: 100%;
        max-width: 500px;
        margin: 1em;
      }
      h1 { color: #28a745; } /* Verde para sucesso */
      p { font-size: 1.1em; line-height: 1.6; }
      a { color: #007bff; text-decoration: none; }
      a:hover { text-decoration: underline; }
    </style>
    </head><body>
    <div class='container'>
      <h1>&#10004; Salvo com Sucesso!</h1>
      <p>As novas configurações foram aplicadas.</p>
      <p>Redirecionando de volta em 3 segundos...</p>
      <p><a href='/'>Voltar Agora</a></p>
    </div>
    </body></html>)";
  server.send(200, "text/html; charset=UTF-8", html);
}

// Setup e Loop
void setup() {
    Serial.begin(115200);
    pinMode(boardLED, OUTPUT);
    digitalWrite(boardLED, LOW);

    for (int i = 0; i < NUM_RATES; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
        pinMode(BTN_PINS[i], INPUT_PULLUP);
    }

    initWiFi();
    timeClient.begin();
    initMQTT();

    // Inicializa o Servidor Web
    server.on("/", HTTP_GET, handleRoot);       // Página principal
    server.on("/salvar", HTTP_POST, handleSave); // Endpoint para salvar
    server.onNotFound([](){
        server.send(404, "text/plain", "Nao encontrado.");
    });

    // Inicia o servidor web
    server.begin();
    Serial.println("Servidor HTTP iniciado! Acesse http://localhost:9080");
}

void loop() {
    // Verifica requisições web
    server.handleClient(); 
    
    // Mantém MQTT
    verificaConexoesWiFiEMQTT();
    
    // Verifica botões
    handleButtons();

    // Delay para estabilidade
    delay(2);
}