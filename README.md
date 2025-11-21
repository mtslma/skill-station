# Skill Station — Totem de Avaliação (ESP32, MQTT e Web)

## Visão geral

Projeto de Totem de Avaliação usando um ESP32 (simulado no Wokwi) que:

-   roda um **servidor web** com tela de login e fluxo de avaliação;
-   publica **resultados via MQTT** para um broker local;
-   mostra status em **OLED** e LEDs;
-   aceita **reset por botão**.

O firmware foi escrito para rodar em PlatformIO / Arduino e usa `PubSubClient`, `ArduinoJson` e `U8g2` para o display.

---

## Principais recursos implementados

-   Servidor web com rotas:

    -   `/` — página inicial / login
    -   `/do-login` — POST para autenticar e guardar token
    -   `/candidaturas` — lista de candidaturas (usa token)
    -   `/detalhes?id=...` — detalhes da candidatura
    -   `/quiz?id=...` — carrega questões e apresenta UI de prova
    -   `/submit` — envia respostas e publica nota via MQTT

-   MQTT: publicador para o tópico **2TDS/esp32/skillstation/submission**  
    Payload JSON inclui `idCandidatura`, `score` e `token`.

-   Tela OLED: exibe título (“Skill Station”), status WiFi, IP e conexão MQTT.

-   LEDs:

    -   LED interno (`boardLED`) → feedback rápido ao publicar score.
    -   LED externo (`STATUS_LED_PIN`) → indica estado WiFi/MQTT.

-   Botão físico `RESET_BTN_PIN`: reinicia via `ESP.restart()`.

-   Logs curtos e diretos via Serial (ex.: “WiFi: conectado”, “MQTT: publish -> ok”).

---

## Hardware (pinos usados)

-   `boardLED` = GPIO 2
-   `STATUS_LED_PIN` = GPIO 25
-   `RESET_BTN_PIN` = GPIO 32
-   OLED I2C
    -   SDA = 21
    -   SCL = 22

Compatível com Wokwi e hardware real ESP32.

---

## Dependências no PlatformIO

Use no `platformio.ini`:

```
lib_deps =
  knolleary/PubSubClient@^2.8
  bblanchon/ArduinoJson@^6.18
  olikraus/U8g2@^2.29
```

---

## Configurações importantes no `main.cpp`

-   `API_URL` — URL base da API
-   `LOGIN_ENDPOINT`, `MINHAS_CANDIDATURAS_ENDPOINT`,  
    `DETALHES_CANDIDATURA_ENDPOINT`, `QUESTOES_ENDPOINT`, `SUBMIT_ENDPOINT`

-   `SSID` e `PASSWORD` — configuração de rede
-   `BROKER_MQTT` e `BROKER_PORT`
-   `TOPICO_PUBLISH` — tópico MQTT usado para enviar a nota

---

## Como rodar o projeto

1. Abra no VSCode com PlatformIO.
2. Compile e faça upload para o ESP32 **ou** use o Wokwi.
3. Abra o Serial Monitor (115200) para acompanhar logs.
4. Acesse a interface web do ESP32:
    - Wokwi → porta indicada no serial (normalmente `http://localhost:9080`).
5. Faça login.
6. Acesse candidaturas, inicie a avaliação e envie a prova.
7. Veja no serial:

```
MQTT: publish -> ok
HTTP submit: retorno 200
```

Isso garante que a nota foi enviada ao broker e registrada na API.

---

## Token e autenticação

-   Após o login, o token é guardado em `currentAccessToken`.
-   Todas as rotas após o login validam o token antes de prosseguir.
-   A submissão inclui o token no payload MQTT para fins de auditoria.

---

## Observações de integração

-   A correção do quiz considera que alternativas corretas são marcadas como `"1"` pela API.
-   A interface gera um formulário `<input type="radio">` por alternativa.
-   Backend API deve responder lista de questões no formato esperado.

---

## Arquivos relevantes

-   `src/main.cpp` — firmware principal
-   `platformio.ini` — dependências e configuração da placa
-   `README.md` — este documento
-   `node-red/flow.json` (opcional, se você usa Node-RED no backend)

---

## Melhorias possíveis

-   Persistir configurações em SPIFFS/LittleFS
-   Reconnect MQTT com backoff
-   Remover token do payload MQTT se segurança exigir
-   Logs opcionais via nível de debug
