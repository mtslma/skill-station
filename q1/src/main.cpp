#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <U8g2lib.h> // OLED

// Hardware & network pins
#define boardLED 2          // LED interno (feedback rápido)
#define STATUS_LED_PIN 25   // LED externo para indicar ligado
#define RESET_BTN_PIN 32    // Botão de reset

// OLED (I2C: SDA=21, SCL=22)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Endpoints e configurações
const char* API_URL = "http://host.wokwi.internal:8080/api";
const char* LOGIN_ENDPOINT = "/auth/login";
const char* MINHAS_CANDIDATURAS_ENDPOINT = "/candidato/me/candidaturas";
const char* DETALHES_CANDIDATURA_ENDPOINT = "/candidato/me/candidaturas/";
const char* QUESTOES_ENDPOINT = "/testes/candidatura/";
const char* SUBMIT_ENDPOINT = "/testes/submit-score";

const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";

const char* BROKER_MQTT = "host.wokwi.internal";
const int BROKER_PORT = 1883;
#define TOPICO_PUBLISH "2TDS/esp32/skillstation/submission"

// Estado global
String currentAccessToken = "";
WiFiClient espClient;
PubSubClient MQTT(espClient);
WebServer server(80);

StaticJsonDocument<4096> doc;
char buffer[4096];

unsigned long lastLedToggle = 0;
bool ledState = false;
unsigned long lastDisplayUpdate = 0;

// CSS embutido (mantido)
const char* EMBEDDED_CSS = R"raw(
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
  <style>
    :root{
      --white:#FFFFFF; --black:#000000;
      --om-100:#fff7ed; --om-200:#ffedd5; --om-300:#fed7aa; --om-400:#fdba74;
      --om-500:#fb923c; --om-600:#f97316; --om-700:#ea580c; --om-800:#c2410c; --om-900:#9a3412;
      --n-100:#f4f4f5; --n-200:#e4e4e7; --n-300:#d4d4d8; --n-400:#a1a1aa; --n-500:#71717a;
      --n-600:#52525b; --n-700:#3f3f46; --n-800:#27272a; --n-900:#18181b;
      --radius:12px; --container-width:920px;
      --ease: cubic-bezier(.2,.9,.2,1);
    }
    html,body{height:100%;margin:0;padding:0;background:linear-gradient(180deg,var(--om-100),#fff);font-family:Inter,system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;color:var(--n-900);-webkit-font-smoothing:antialiased}
    a{color:inherit;text-decoration:none}
    .page{min-height:100vh;display:flex;align-items:center;justify-content:center;padding:28px}
    .shell{width:100%;max-width:var(--container-width);display:flex;flex-direction:column;gap:20px}
    .topbar{display:flex;justify-content:space-between;align-items:center;padding:14px 18px;border-radius:10px;background:transparent}
    .brand{display:flex;gap:14px;align-items:center}
    .logo{width:48px;height:48px;border-radius:10px;background:linear-gradient(135deg,var(--om-700),var(--om-500));display:flex;align-items:center;justify-content:center;color:var(--white);font-weight:800;box-shadow:0 6px 18px rgba(234,88,12,0.12)}
    .brand h1{font-size:1rem;margin:0;color:var(--n-800);letter-spacing:0.2px}
    .brand p{margin:0;font-size:0.85rem;color:var(--n-500)}
    .card{background:var(--white);border-radius:14px;box-shadow:0 18px 60px rgba(16,24,40,0.08);border:1px solid rgba(0,0,0,0.04);overflow:hidden;display:grid;grid-template-columns:1fr 420px}
    @media (max-width:940px){ .card{grid-template-columns:1fr} }
    .left {padding:36px 28px;display:flex;flex-direction:column;gap:18px}
    .left h2{margin:0;font-size:1.35rem;color:var(--n-900);font-weight:700}
    .left p{margin:0;color:var(--n-600);line-height:1.5}
    .left .feature-list{display:grid;gap:10px;margin-top:10px}
    .feature{display:flex;gap:12px;align-items:flex-start}
    .feature .ico{width:40px;height:40px;border-radius:8px;background:var(--om-100);display:flex;align-items:center;justify-content:center;color:var(--om-800);font-weight:700}
    .feature strong{display:block}
    .right{padding:26px;display:flex;align-items:center;justify-content:center;background:linear-gradient(180deg,#fff, #fff)}
    .panel{width:100%;max-width:360px;border-radius:12px;padding:18px;border:1px solid var(--n-100);box-shadow:0 10px 30px rgba(16,24,40,0.04);background:linear-gradient(180deg,var(--white),#fff)}
    .panel h3{margin:0 0 6px 0;font-size:1.05rem;color:var(--n-900)}
    .panel p.note{margin:0;color:var(--n-600);font-size:0.92rem}
    .field{display:flex;align-items:center;gap:10px;padding:10px;border-radius:10px;background:var(--n-100);border:1px solid transparent;transition:box-shadow 180ms var(--ease),transform 180ms var(--ease)}
    .field:focus-within{box-shadow:0 10px 30px rgba(16,24,40,0.06);transform:translateY(-1px);border-color:var(--om-300)}
    .field i{width:28px;text-align:center;color:var(--om-700)}
    .field input{border:0;background:transparent;outline:0;font-size:0.95rem;color:var(--n-900);width:100%;padding:6px 0}
    .actions{display:flex;gap:10px;align-items:center;justify-content:flex-end;margin-top:14px}
    .btn{display:inline-flex;align-items:center;gap:10px;padding:10px 14px;border-radius:10px;border:0;font-weight:700;cursor:pointer;transition:transform 160ms var(--ease)}
    .btn:active{transform:translateY(1px)}
    .btn-primary{background:linear-gradient(90deg,var(--om-700),var(--om-600));color:var(--white)}
    .btn-primary:hover{filter:brightness(.98)}
    .btn-ghost{background:transparent;border:1px solid var(--n-200);color:var(--n-700);padding:9px 12px}
    .btn-cta{background:var(--om-900);color:var(--white)}
    .row{display:flex;justify-content:space-between;align-items:center;padding:12px;border-radius:10px;background:var(--om-100);border:1px solid var(--om-200);gap:12px}
    .meta{color:var(--n-600);font-size:0.9rem}
    .question{display:none}
    .question.active{display:block;animation:fadeIn .18s ease-in}
    @keyframes fadeIn{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:translateY(0)}}
    .progress{height:8px;background:var(--n-100);border-radius:999px;overflow:hidden}
    .progress>i{display:block;height:100%;width:0;background:linear-gradient(90deg,var(--om-600),var(--om-700));transition:width 240ms ease}
    .review-item{padding:12px;border-radius:10px;border:1px solid var(--n-100);margin-bottom:10px;background:#fff}
    .footer{display:flex;justify-content:space-between;align-items:center;padding:10px 6px;color:var(--n-500);font-size:0.85rem}
    .muted{color:var(--n-600);font-size:0.9rem}
    .center{text-align:center}
  </style>
)raw";

// Forward declarations
void initWiFi();
void initMQTT();
void verificaConexoes();
bool authenticateAndGetToken(String email, String senha);
void sendMqttScoreWithToken(String idCandidatura, double score);
String formatDateISO_hhmmss(const String &iso);
String humanizeStatus(const String &s);
void updateOLED();

void handleRoot();
void handleDoLogin();
void handleCandidaturasList();
void handleDetalhes();
void handleQuiz();
void handleSubmit();

// Network / auth implementations

void initWiFi() {
  Serial.println("WiFi: iniciando conexão...");
  WiFi.begin(SSID, PASSWORD);
  unsigned long start = millis();
  // pisca LED enquanto tenta conectar (feedback)
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
      delay(100);
  }
  if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi: conectado, IP ");
      Serial.println(WiFi.localIP().toString());
      digitalWrite(STATUS_LED_PIN, HIGH); // indica ligado
  } else {
      Serial.println("WiFi: falha ao conectar");
      digitalWrite(STATUS_LED_PIN, LOW);
  }
}

void initMQTT() {
  Serial.println("MQTT: configurando cliente...");
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  String clientId = "Skill Station-" + WiFi.macAddress();
  Serial.print("MQTT: clientId=");
  Serial.println(clientId);
  // Tenta conectar (não checa retorno aqui para não bloquear)
  if (MQTT.connect(clientId.c_str())) {
    Serial.println("MQTT: conectado");
  } else {
    Serial.println("MQTT: conexão pendente/falhou");
  }
}

void verificaConexoes() {
  if (WiFi.status() != WL_CONNECTED) initWiFi();
  if (!MQTT.connected()) initMQTT();
  MQTT.loop();
}

bool authenticateAndGetToken(String email, String senha) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Auth: sem WiFi");
    return false;
  }
  Serial.println("Auth: solicitando token...");
  HTTPClient http;
  http.begin(String(API_URL) + LOGIN_ENDPOINT);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"email\":\"" + email + "\",\"senha\":\"" + senha + "\"}";
  int httpCode = http.POST(payload);
  if (httpCode == 200 || httpCode == 201) {
    String response = http.getString();
    doc.clear();
    deserializeJson(doc, response);
    currentAccessToken = doc["token"].as<String>();
    Serial.println("Auth: token recebido");
    http.end();
    return true;
  }
  Serial.print("Auth: falha HTTP ");
  Serial.println(httpCode);
  http.end();
  return false;
}

void sendMqttScoreWithToken(String idCandidatura, double score) {
  // monta payload e publica no tópico
  doc.clear();
  doc["idCandidatura"] = idCandidatura.toInt();
  doc["score"] = score;
  doc["token"] = currentAccessToken;
  serializeJson(doc, buffer);
  bool ok = MQTT.publish(TOPICO_PUBLISH, buffer);
  Serial.print("MQTT: publish -> ");
  Serial.println(ok ? "ok" : "falha");
  // feedback visual breve
  for(int i=0; i<5; i++){
      digitalWrite(boardLED, HIGH); delay(50);
      digitalWrite(boardLED, LOW); delay(50);
  }
}

// Helpers

String formatDateISO_hhmmss(const String &iso) {
  if (iso.length() < 19) return iso;
  String year = iso.substring(0,4);
  String month = iso.substring(5,7);
  String day = iso.substring(8,10);
  String hour = iso.substring(11,13);
  String minute = iso.substring(14,16);
  String second = iso.substring(17,19);
  String shortYear = year.substring(2,4);
  return hour + ":" + minute + ":" + second + " - " + day + "/" + month + "/" + shortYear;
}

String humanizeStatus(const String &s) {
  String tmp = s;
  for (int i = 0; i < tmp.length(); i++) if (tmp.charAt(i) == '_') tmp.setCharAt(i, ' ');
  tmp.toLowerCase();
  bool cap = true;
  for (int i = 0; i < tmp.length(); i++) {
    char c = tmp.charAt(i);
    if (cap && c >= 'a' && c <= 'z') { tmp.setCharAt(i, c - ('a' - 'A')); cap = false; }
    else if (c == ' ') cap = true;
    else cap = false;
  }
  return tmp;
}

// Atualiza OLED com informações essenciais (não bloqueante)
void updateOLED() {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_helvB08_tr);
        u8g2.drawStr(0, 10, "Skill Station"); // título

        u8g2.setFont(u8g2_font_crox1c_tf);
        u8g2.drawStr(0, 24, "One Matter GS");

        u8g2.drawLine(0, 28, 128, 28);

        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.setCursor(0, 40);
        if (WiFi.status() == WL_CONNECTED) {
            u8g2.print("WiFi: ON");
            u8g2.setCursor(0, 50);
            u8g2.print(WiFi.localIP());
        } else {
            u8g2.print("WiFi: OFF");
        }

        u8g2.setCursor(0, 62);
        if (MQTT.connected()) {
            u8g2.print("MQTT: Online");
        } else {
            u8g2.print("MQTT: ...Conectando");
        }

    } while (u8g2.nextPage());
}

// Web handlers

void handleRoot() {
  Serial.println("HTTP: / (root) requisitado");
  String html = "<!doctype html><html lang=\"pt-BR\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Skill Station</title>";
  html += EMBEDDED_CSS;
  html += "</head><body><div class=\"page\"><div class=\"shell\">";
  html += "<div class=\"topbar\"><div class=\"brand\"><div class=\"logo\">SS</div><div><h1>Skill Station</h1><p>Onde o talento encontra propósito</p></div></div><div class=\"muted small\">One Matter</div></div>";

  html += "<div class=\"card\">";
  html += "<div class=\"left\"><h2>Avaliações simples e seguras</h2><p>Avaliações em ambientes justos e seguros.</p>";
  html += "<div class=\"feature-list\">";
  html += "<div class=\"feature\"><div class=\"ico\"><i class=\"fa-solid fa-check\"></i></div><div><strong>Progressivo</strong><div class=\"muted\">Avança por questão para foco máximo.</div></div></div>";
  html += "<div class=\"feature\"><div class=\"ico\"><i class=\"fa-solid fa-shield-halved\"></i></div><div><strong>Seguro</strong><div class=\"muted\">Proteção completa das avaliações e respostas.</div></div></div>";
  html += "<div class=\"feature\"><div class=\"ico\"><i class=\"fa-solid fa-chart-line\"></i></div><div><strong>Feedback</strong><div class=\"muted\">Nota preliminar enviada ao RH automaticamente.</div></div></div>";
  html += "</div></div>";

  html += "<div class=\"right\"><div class=\"panel\" role=\"region\" aria-labelledby=\"login-title\"><h3 id=\"login-title\">Entrar</h3><p class=\"note\">Use suas credenciais corporativas</p>";
  html += "<form action=\"/do-login\" method=\"POST\" style=\"margin-top:14px\">";
  html += "<div class=\"field\"><i class=\"fa-solid fa-envelope\"></i><input type=\"text\" name=\"email\" placeholder=\"E-mail\" required autocomplete=\"username\"></div>";
  html += "<div class=\"field\"><i class=\"fa-solid fa-lock\"></i><input type=\"password\" name=\"senha\" placeholder=\"Senha\" required autocomplete=\"current-password\"></div>";
  html += "<div class=\"actions\"><a class=\"btn btn-ghost\" href=\"/\" title=\"Ajuda\"><i class=\"fa-solid fa-circle-info\"></i></a><button class=\"btn btn-primary\" type=\"submit\"><i class=\"fa-solid fa-right-to-bracket\"></i> Entrar</button></div>";
  html += "</form></div></div>";
  html += "</div>";
  html += "<div class=\"footer\"><div>© Skill Station</div><div class=\"muted\">Versão interna</div></div>";

  html += "</div></div></body></html>";
  server.send(200, "text/html", html);
}

void handleDoLogin() {
  Serial.println("HTTP: /do-login requisitado");
  if (!server.hasArg("email") || !server.hasArg("senha")) { server.send(400, "text/plain", "Dados incompletos"); return; }
  if (authenticateAndGetToken(server.arg("email"), server.arg("senha"))) {
    server.sendHeader("Location", "/candidaturas");
    server.send(302, "text/plain", "");
  } else {
    Serial.println("Auth: credenciais inválidas");
    String html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Falha</title>";
    html += EMBEDDED_CSS;
    html += "</head><body><div class=\"page\"><div class=\"panel\" style=\"margin:0 auto;max-width:540px;text-align:center;padding:26px;border-radius:12px\"><h3 style=\"color:var(--om-700)\">Autenticação falhou</h3><p class=\"muted\">Verifique suas credenciais e tente novamente.</p><div style=\"margin-top:16px\"><a class=\"btn btn-primary\" href=\"/\">Voltar</a></div></div></div></body></html>";
    server.send(200, "text/html", html);
  }
}

void handleCandidaturasList() {
  Serial.println("HTTP: /candidaturas requisitado");
  if (currentAccessToken == "") { server.sendHeader("Location", "/"); server.send(302); return; }
  HTTPClient http;
  String url = String(API_URL) + MINHAS_CANDIDATURAS_ENDPOINT + "?size=20";
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + currentAccessToken);
  int httpCode = http.GET();
  String rows = "";

  if (httpCode == 200) {
    doc.clear();
    deserializeJson(doc, http.getString());
    JsonArray candidaturas = doc["content"].as<JsonArray>();
    if (candidaturas.size() == 0) {
      rows = "<div class='row'><div><strong>Nenhuma candidatura</strong><div class='muted'>Sem registros</div></div></div>";
    }
    for (JsonObject c : candidaturas) {
      String id = c["idCandidatura"].as<String>();
      String vaga = c["descVaga"].as<String>();
      String empresa = c["nomeEmpresa"].as<String>();
      String statusRaw = c["ultimoStatus"].as<String>();
      String dataRaw = c["dataUltimaAtualizacao"].as<String>();
      String dataFmt = formatDateISO_hhmmss(dataRaw);
      bool provaConcluida = (statusRaw == "TESTE_SUBMETIDO");
      bool emAndamento = (statusRaw == "EM_ANDAMENTO");
      String badgeText = humanizeStatus(statusRaw);

      rows += "<div style='margin-bottom:12px'><div class='row'><div>";
      rows += "<div style='font-weight:700;color:var(--n-900)'>" + vaga + "</div>";
      rows += "<div class='meta'>" + empresa + " • " + dataFmt + "</div></div>";
      rows += "<div style='display:flex;gap:10px;align-items:center'><div class='muted' style='font-weight:600'>" + badgeText + "</div>";
      if (provaConcluida) rows += "<a class='btn btn-primary' href='/detalhes?id=" + id + "' title='Ver'><i class='fa-solid fa-eye'></i></a>";
      else rows += "<a class='btn btn-primary' href='/quiz?id=" + id + "' title='Iniciar/Continuar'><i class='fa-solid fa-play'></i></a>";
      rows += "</div></div></div>";
    }
  } else {
    rows = "<div class='row'><div><strong>Erro</strong><div class='muted'>HTTP " + String(httpCode) + "</div></div></div>";
  }
  http.end();

  String html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Minhas Candidaturas</title>";
  html += EMBEDDED_CSS;
  html += "</head><body><div class=\"page\"><div class=\"shell\"><div class=\"topbar\"><div class=\"brand\"><div class=\"logo\">SS</div><div><h1>Minhas Candidaturas</h1><p>Visualize suas avaliações</p></div></div><div class=\"muted\">Logado</div></div>";
  html += "<div class=\"card\" style=\"grid-template-columns:1fr\"><div class=\"left\" style=\"padding:18px\"><div style=\"display:flex;justify-content:space-between;align-items:center;margin-bottom:10px\"><div><strong style='font-size:1.05rem'>Suas vagas</strong><div class='muted'>Acesse a avaliação</div></div><div><a class='btn btn-ghost' href='/'><i class='fa-solid fa-right-from-bracket'></i></a></div></div>";
  html += rows;
  html += "</div></div></div></body></html>";

  server.send(200, "text/html", html);
}

void handleDetalhes() {
  Serial.println("HTTP: /detalhes requisitado");
  if (currentAccessToken == "") { server.sendHeader("Location", "/"); server.send(302); return; }
  if (!server.hasArg("id")) { server.sendHeader("Location", "/candidaturas"); server.send(302); return; }

  String id = server.arg("id");
  HTTPClient http;
  String url = String(API_URL) + DETALHES_CANDIDATURA_ENDPOINT + id;
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + currentAccessToken);
  int httpCode = http.GET();
  String details = "";
  String history = "";

  if (httpCode == 200) {
    doc.clear();
    deserializeJson(doc, http.getString());
    String vaga = doc["descricaoVaga"].as<String>();
    String empresa = doc["nomeEmpresa"].as<String>();
    String dataRaw = doc["dataCandidatura"].as<String>();
    String dataFmt = formatDateISO_hhmmss(dataRaw);
    double notaD = doc["notaFinal"].isNull() ? -1.0 : doc["notaFinal"].as<double>();
    String nota = (notaD < 0) ? "N/A" : String(notaD, 1);
    String situacao = "PENDENTE"; if (notaD >= 0) situacao = (notaD >= 70.0) ? "APROVADO" : "REPROVADO";

    details += "<div style='padding:8px'><div style='font-weight:700;color:var(--n-900)'>" + vaga + "</div>";
    details += "<div class='muted' style='margin-top:6px'>" + empresa + "</div>";
    details += "<div class='muted' style='margin-top:8px'>Aplicado: " + dataFmt + "</div>";
    details += "<div style='margin-top:8px'>Nota final: <strong style='color:var(--om-700)'>" + nota + "</strong></div>";
    details += "<div style='margin-top:8px'>Situação: <strong>" + situacao + "</strong></div></div>";

    JsonArray hist = doc["statusHistorico"].as<JsonArray>();
    if (hist) {
      for (JsonObject s : hist) {
        String desc = s["statusDescricao"].as<String>();
        String d = s["dataAtualizacao"].as<String>();
        history += "<div style='padding:10px;border-radius:8px;border:1px solid var(--n-100);margin-bottom:8px'><div style='font-weight:600'>" + humanizeStatus(desc) + "</div><div class='muted' style='margin-top:6px'>" + formatDateISO_hhmmss(d) + "</div></div>";
      }
    } else history = "<div class='muted'>Sem histórico</div>";
  } else {
    details = "<div class='row'><div><strong>Erro</strong><div class='muted'>HTTP " + String(httpCode) + "</div></div></div>";
  }
  http.end();

  String html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Detalhes</title>";
  html += EMBEDDED_CSS;
  html += "</head><body><div class=\"page\"><div class=\"shell\"><div class=\"topbar\"><div class=\"brand\"><div class=\"logo\">SS</div><div><h1>Detalhes</h1><p>Visão da candidatura</p></div></div><div></div></div>";
  html += "<div class=\"card\" style=\"grid-template-columns:1fr 360px\"><div class=\"left\" style=\"padding:18px\">";
  html += details;
  html += "</div><div class=\"right\" style=\"padding:18px\"><div class=\"panel\"><h3>Histórico</h3><div style=\"margin-top:10px\">";
  html += history;
  html += "</div><div style=\"margin-top:12px;text-align:right\"><a class='btn btn-primary' href='/candidaturas'>Voltar</a></div></div></div></div></div></body></html>";

  server.send(200, "text/html", html);
}

void handleQuiz() {
  Serial.println("HTTP: /quiz requisitado");
  if (currentAccessToken == "") { server.sendHeader("Location", "/"); server.send(302); return; }
  if (!server.hasArg("id")) { server.sendHeader("Location", "/candidaturas"); server.send(302); return; }

  String id = server.arg("id");
  HTTPClient http;
  String url = String(API_URL) + QUESTOES_ENDPOINT + id + "/questoes";
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + currentAccessToken);
  int httpCode = http.GET();

  if (httpCode == 403) {
    String html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
    html += EMBEDDED_CSS;
    html += "</head><body><div class=\"page\"><div class=\"panel\" style=\"margin:0 auto;padding:28px;text-align:center\"><div class=\"muted\">Avaliação já finalizada</div><div style=\"margin-top:12px\"><a class=\"btn btn-primary\" href=\"/candidaturas\">Voltar</a></div></div></div></body></html>";
    server.send(403, "text/html", html); http.end(); return;
  }

  String formHtml = "";
  int total = 0;

  if (httpCode == 200) {
    doc.clear();
    deserializeJson(doc, http.getString());
    JsonArray questoes = doc.as<JsonArray>();
    total = questoes.size();
    int idx = 0;
    for (JsonObject q : questoes) {
      String en = q["enunciado"].as<String>();
      formHtml += "<div class='question' data-index='" + String(idx) + "'>";
      formHtml += "<div style='padding:14px;border-radius:10px;border:1px solid var(--n-100);background:var(--white);margin-bottom:12px'>";
      formHtml += "<div style='font-weight:700;color:var(--n-900);'>Questão " + String(idx+1) + "</div>";
      formHtml += "<div class='muted' style='margin-top:8px'>" + en + "</div>";
      for (int i = 1; i <= 5; i++) {
        String key = "alternativa" + String(i);
        if (!q[key].isNull()) {
          String txt = q[key].as<String>();
          formHtml += "<label class='option' data-alt=\"" + txt + "\" style='display:block;padding:10px;border-radius:8px;margin-top:10px;border:1px solid var(--n-100)'><input type='radio' name='q" + String(idx) + "' value='" + String(i) + "' style='margin-right:10px'> " + txt + "</label>";
        }
      }
      formHtml += "</div></div>";
      idx++;
    }
  } else {
    formHtml = "<div class='row'><div><strong>Erro</strong><div class='muted'>HTTP " + String(httpCode) + "</div></div></div>";
  }
  http.end();

  String script = R"scr(
    <script>
      (function(){
        const total = parseInt(document.getElementById('total').value || '0');
        let current = 0;
        const questions = Array.from(document.querySelectorAll('.question'));
        const btnNext = document.getElementById('btn-next');
        const btnPrev = document.getElementById('btn-prev');
        const progressBar = document.querySelector('.progress > i');
        const reviewArea = document.getElementById('review-area');
        const reviewList = document.getElementById('review-list');
        const btnConfirm = document.getElementById('btn-confirm');

        function showQuestion(i){
          questions.forEach((q, idx)=> {
            q.classList.toggle('active', idx === i);
          });
          current = i;
          updateControls();
          updateProgress();
          window.scrollTo({top:0, behavior:'smooth'});
        }

        function updateControls(){
          btnPrev.disabled = (current === 0);
          const radios = document.querySelectorAll('input[name="q'+current+'"]');
          const answered = Array.from(radios).some(r => r.checked);
          btnNext.disabled = !answered;
          btnNext.textContent = (current === total - 1) ? 'Revisar' : 'Próxima';
        }

        function updateProgress(){
          const pct = Math.round(((current) / Math.max(1, total - 1)) * 100);
          progressBar.style.width = pct + '%';
        }

        function buildReview(){
          reviewList.innerHTML = '';
          for(let i=0;i<total;i++){
            const sel = document.querySelector('input[name=q'+i+']:checked');
            let preview = '(sem resposta)';
            if(sel){
              const label = sel.closest('label');
              if(label && label.dataset && label.dataset.alt) preview = label.dataset.alt;
            }
            const node = document.createElement('div');
            node.className = 'review-item';
            node.innerHTML = '<div style="font-weight:700">Questão ' + (i+1) + '</div><div class="muted" style="margin-top:6px">' + preview + '</div><div style="margin-top:8px;text-align:right"><button data-index="'+i+'" class="btn-edit-question btn btn-ghost">Editar</button></div>';
            reviewList.appendChild(node);
          }
          Array.from(document.querySelectorAll('.btn-edit-question')).forEach(b => {
            b.addEventListener('click', (e)=>{
              const idx = parseInt(e.currentTarget.dataset.index,10);
              reviewArea.style.display='none';
              document.getElementById('questions-box').style.display='block';
              document.getElementById('controls').style.display='flex';
              showQuestion(idx);
            });
          });
        }

        document.addEventListener('change', (e)=> { if(e.target && e.target.type==='radio') updateControls(); });

        btnNext.addEventListener('click', function(e){
          e.preventDefault();
          if(current < total - 1){ current++; showQuestion(current); } else {
            document.getElementById('questions-box').style.display='none';
            document.getElementById('controls').style.display='none';
            reviewArea.style.display='block';
            buildReview();
            window.scrollTo({top:0, behavior:'smooth'});
          }
        });

        btnPrev.addEventListener('click', function(e){ e.preventDefault(); if(current > 0) showQuestion(current - 1); });

        btnConfirm.addEventListener('click', function(e){ e.preventDefault(); document.getElementById('form-quiz').submit(); });

        if(total>0) showQuestion(0);
        else { btnNext.disabled = true; btnPrev.disabled = true; }
      })();
    </script>
  )scr";

  String html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Prova</title>";
  html += EMBEDDED_CSS;
  html += "</head><body><div class=\"page\"><div class=\"shell\"><div class=\"topbar\"><div class=\"brand\"><div class=\"logo\">SS</div><div><h1>Avaliação Técnica</h1><p class=\"muted\">Responda com atenção</p></div></div><div></div></div>";
  html += "<div class=\"card\" style=\"grid-template-columns:1fr\"><div class=\"left\" style=\"padding:18px\">";
  html += "<div style=\"margin-bottom:12px\"><div class=\"progress\"><i style=\"width:0%\"></i></div></div>";
  html += "<form id=\"form-quiz\" action=\"/submit\" method=\"POST\">";
  html += "<input type=\"hidden\" name=\"id\" value=\"" + id + "\">";
  html += "<input type=\"hidden\" id=\"total\" name=\"total\" value=\"" + String(total) + "\">";
  html += "<div id=\"questions-box\">";
  html += formHtml;
  html += "</div>";
  html += "<div id=\"controls\" style=\"margin-top:8px;display:flex;justify-content:space-between;align-items:center\">";
  html += "<a class=\"btn btn-ghost\" href=\"/candidaturas\"><i class=\"fa-solid fa-arrow-left\"></i> Voltar</a>";
  html += "<div style=\"display:flex;gap:8px\"><button id=\"btn-prev\" class=\"btn btn-ghost\" type=\"button\">Anterior</button><button id=\"btn-next\" class=\"btn btn-primary\" type=\"button\" disabled>Próxima</button></div>";
  html += "</div>";
  html += "<div id=\"review-area\" style=\"display:none;margin-top:14px\"><h3 style=\"margin:0 0 8px 0;color:var(--n-900)\">Revisão</h3><div id=\"review-list\"></div><div style=\"display:flex;justify-content:flex-end;gap:10px;margin-top:12px\"><button id=\"btn-confirm\" class=\"btn btn-primary\" type=\"button\">Confirmar e Enviar</button></div></div>";
  html += "</form></div></div></div>";
  html += script;
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSubmit() {
  Serial.println("HTTP: /submit requisitado");
  if (currentAccessToken == "") { server.sendHeader("Location", "/"); server.send(302); return; }

  String id = server.arg("id");
  int total = server.arg("total").toInt();
  int acertos = 0;
  for (int i = 0; i < total; i++) if (server.arg("q" + String(i)) == "1") acertos++;
  double score = (total > 0) ? ((double)acertos / total) * 100.0 : 0.0;

  sendMqttScoreWithToken(id, score);

  Serial.print("HTTP: enviando nota ");
  Serial.println(String(score,1));

  HTTPClient http;
  http.begin(String(API_URL) + SUBMIT_ENDPOINT);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + currentAccessToken);
  String jsonPayload = "{\"idCandidatura\":" + id + ", \"score\":" + String(score) + "}";
  int httpCode = http.POST(jsonPayload);
  Serial.print("HTTP submit: retorno ");
  Serial.println(httpCode);
  http.end();

  String html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Concluído</title>";
  html += EMBEDDED_CSS;
  html += "</head><body><div class=\"page\"><div class=\"shell\"><div class=\"panel\" style=\"margin:0 auto;max-width:640px;text-align:center;padding:28px;border-radius:12px\"><div style=\"font-weight:700;color:var(--om-700);font-size:1.15rem\">Prova Finalizada</div><div class=\"muted\" style=\"margin-top:10px\">Nota preliminar: <strong style=\"color:var(--om-700)\">" + String(score,1) + "</strong></div><div style=\"margin-top:16px\"><a class=\"btn btn-primary\" href=\"/candidaturas\"><i class=\"fa-solid fa-house\"></i> Voltar</a></div></div></div></body></html>";

  server.send(200, "text/html", html);
}

// Setup / Loop

void setup() {
  Serial.begin(115200);
  pinMode(boardLED, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);

  u8g2.begin();
  Serial.println("Setup: inicializando rede e serviços");
  initWiFi();
  initMQTT();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/do-login", HTTP_POST, handleDoLogin);
  server.on("/candidaturas", HTTP_GET, handleCandidaturasList);
  server.on("/detalhes", HTTP_GET, handleDetalhes);
  server.on("/quiz", HTTP_GET, handleQuiz);
  server.on("/submit", HTTP_POST, handleSubmit);

  server.begin();
  Serial.println("Servidor HTTP iniciado: http://localhost:9080");
}

void loop() {
  // reset por botão (com debounce simples)
  if (digitalRead(RESET_BTN_PIN) == LOW) {
      delay(50);
      if (digitalRead(RESET_BTN_PIN) == LOW) {
          Serial.println("Reset solicitado...");
          for(int i=0; i<5; i++){
            digitalWrite(STATUS_LED_PIN, LOW); delay(100);
            digitalWrite(STATUS_LED_PIN, HIGH); delay(100);
          }
          ESP.restart();
      }
  }

  verificaConexoes();
  server.handleClient();

  // atualiza OLED periodicamente (não bloqueante)
  if (millis() - lastDisplayUpdate > 2000) {
      updateOLED();
      lastDisplayUpdate = millis();
  }

  delay(5);
}
