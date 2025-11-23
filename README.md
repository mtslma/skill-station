# Skill Station: Recrutamento Ético e Tecnológico

## Visão geral

O Skill Station é parte do propósito One Matter, que visa eliminar vieses e garantir a ética em processos seletivos de tecnologia. Nascido da convicção de que o talento deve ser avaliado de forma pura, este projeto estabelece um ecossistema completo que conecta hardware, software e metodologia.

## O Ecossistema de Avaliação Blindada

Nosso sistema opera em múltiplas camadas para assegurar a justiça:

1. Candidatura Cega (**Mobile/ API Java**): O processo começa com a aplicação por meio de um aplicativo mobile, onde os candidatos são avaliados puramente com base em suas skills. Os recrutadores conduzem a primeira fase "às cegas", selecionando talentos unicamente pela competência.

2. A Estação Física (**IoT & Hardware**): Os candidatos pré-selecionados são convidados à Skill Station, um ambiente de avaliação física padronizado, controlado e monitorado por dispositivos IoT (ESP32). Este ambiente elimina interferências externas, garantindo que as condições sejam idênticas e justas para todos.

3. Tecnologia de Confiança (**Web Server & MQTT**): A avaliação técnica é executada via um servidor web embarcado no ESP32, que se comunica com a API corporativa para carregar as questões. As respostas são então enviadas de forma assíncrona e auditável via MQTT para serem armazenados no banco de dados e posteriormente avaliados por recrutadores, levando a última etapa: uma entrevista.

## Propósito e Vantagem

O Skill Station transforma o recrutamento, proporcionando uma experiência de avaliação técnica segura, justa e auditável. É a união estratégica de IoT, Desenvolvimento Avançado e Metodologia Ética para modernizar o mercado de trabalho, garantindo que o sucesso de um candidato seja determinado, de fato, apenas por suas habilidades.

## Uma Skill Station possui...

-   **Servidor Web Embarcado:** Roda um servidor web HTTP (ESP32) que hospeda a tela de login e o fluxo completo de avaliação, conectando-se dinamicamente à API corporativa.
-   **Comunicação Imediata:** Publica resultados e dados da prova via protocolo MQTT para um broker central, permitindo que os resultados sejam auditados e validados em tempo real.
-   **Feedback Visual e Diagnóstico:** Exibe status críticos (Conexão Wi-Fi, MQTT) em um Display OLED e utiliza LEDs para feedback operacional.
-   **Controle Físico:** Permite o reset imediato da placa por botão, garantindo um rápido retorno ao estado inicial em caso de falha, ou para desligamento seguro.

O firmware foi escrito para rodar em PlatformIO / Arduino e usa `PubSubClient`, `ArduinoJson` e `U8g2` para o display.

---

## Principais recursos implementados

-   **Servidor web com rotas:**
    | Rota | Descrição |
    | :--------- | :---------------------------------- |
    | `/` | Página inicial / login |
    | `/do-login` | POST para autenticar e guardar token |
    | `/candidaturas` | Lista de candidaturas (usa token) |
    | `/detalhes?id={idCandidatura}` | Detalhes da candidatura |
    | `/quiz?id={idAvaliacao}` | Carrega questões e apresenta UI de prova |
    | `/submit` | Envia respostas e publica nota via MQTT |

-   **MQTT:** Publicador para o tópico **2TDS/esp32/skillstation/submission**  
    Payload JSON inclui `idCandidatura`, `score` e `token`.

-   **Tela OLED:** Exibe título (“Skill Station”), status WiFi, IP e conexão MQTT.

-   **LEDs:**

    -   LED interno (`boardLED`) → feedback rápido ao publicar score.
    -   LED externo (`STATUS_LED_PIN`) → indica estado WiFi/MQTT.

-   Botão físico `RESET_BTN_PIN`: reinicia via `ESP.restart()`.

-   Logs básicos via Serial (ex.: “WiFi: conectado”, “MQTT: publish -> ok”).

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
  knolleary/PubSubClient@^2.8        ; comunicação MQTT
  bblanchon/ArduinoJson@^6.18        ; parse e montagem de JSON
  olikraus/U8g2@^2.29                ; driver do OLED (I2C)
```

Essas bibliotecas são essenciais para:

-   Comunicação MQTT com o broker,

-   Manipulação das mensagens JSON (API e MQTT),

-   Exibição no display OLED da estação.

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

## Pré-requisitos

### 1. Software Básico

-   Visual Studio Code
-   Node.js (LTS) — necessário para rodar o Node-RED.

### 2. Extensões do VS Code

-   PlatformIO IDE — para compilar e gerenciar o projeto ESP32.
-   Wokwi for VS Code — para simular o hardware.

### 3. Serviços Locais

-   Mosquitto (MQTT Broker)
-   Node-RED

---

## Instalação e Configuração

### Clone o repositório:

```
git clone https://github.com/mtslma/skill-station
```

### Abra no VSCode

-   Abra a pasta do projeto (`Arquivo > Abrir Pasta...`).
-   O PlatformIO carregará automaticamente as configs do projeto.

### Dependências do PlatformIO

O arquivo `platformio.ini` instalará automaticamente:

```
lib_deps =
  knolleary/PubSubClient@^2.8
  bblanchon/ArduinoJson@^6.18
  olikraus/U8g2@^2.29
```

Essas libs são usadas para:

-   MQTT (PubSubClient)
-   JSON (ArduinoJson)
-   Display OLED (U8g2)

Certifique-se de fazer o build da aplicação por meio da extensão do platform.io antes de executar 

---

### Licença Wokwi (Obrigatória para Rede)

Para simulação com Wi-Fi/MQTT:

1. Acesse: https://wokwi.com/license
2. Ative no VSCode (`F1 > Wokwi: Activate License`).
3. Cole sua chave de licença.

---

### Instale o Mosquitto

1. Baixe em: https://mosquitto.org/download/
2. Após instalar, abra `services.msc`.
3. Encontre **"Mosquitto Broker"** e verifique se está **Em Execução**.

---

### Instale o Node-RED

Execute no terminal:

```
npm install -g --unsafe-perm node-red
```

---

## Como Rodar o Projeto (Passo a Passo)

A ordem de inicialização é importante.

---

### Passo 1: Iniciar os Serviços Locais

#### Mosquitto

-   Verifique no `services.msc` se está rodando.
-   Inicie manualmente se necessário.

#### Node-RED

Execute:

```
node-red
```

O terminal mostrará algo como:

`Server running at http://127.0.0.1:1880`.

---

### Passo 2: Configurar o Node-RED

1. Abra http://127.0.0.1:1880
2. Menu > Importar
3. Importe o arquivo `flow.json` da pasta `/node-red`
4. Clique em **Deploy**
5. O nó **MQTT In** deve ficar com o quadrado **verde (connected)**

---

### Passo 3: Iniciar a Simulação do ESP32

1. Abra a paleta (F1 ou Ctrl+Shift+P)
2. Execute **"Wokwi: Start Simulator"**
3. No Monitor Serial, espere algo como:

```
Wi-Fi conectado!
MQTT conectado!
Servidor HTTP iniciado em http://localhost:9080
```

---

### Passo 4: Testar a Solução Completa

#### Testar o Web Server

1. Acesse `http://localhost:9080`
2. Faça login usando suas credenciais One Matter
3. Acesse candidaturas, abra uma prova e envie o resultado

Credenciais de teste:

```
usuário: user@onematter.com
senha: senhaSegura123
```

#### Testar o Fluxo MQTT

1. Complete a prova no totem
2. O ESP32 enviará automaticamente a nota via MQTT
3. Veja no Node-RED → aba **Debug** a mensagem JSON recebida

Exemplo esperado:

```
MQTT: publish -> ok
HTTP submit: retorno 200
```

Isso confirma que:

-   a nota chegou ao broker
-   e foi registrada na API

---

## Token e Autenticação

-   Após o login, o token é salvo em `currentAccessToken`
-   Todas as rotas internas usam esse token
-   Ele também é incluído no MQTT para auditoria

---

## Arquivos Relevantes

-   `src/main.cpp` — firmware principal
-   `platformio.ini` — dependências e config da placa
-   `README.md` — documentação
-   `node-red/flow.json` — fluxo para o Node-RED
