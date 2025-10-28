# Projeto IoT: Totem de Avaliação com ESP32, MQTT e Node-RED

## Descrição

Este projeto implementa um **Totem de Avaliação de Atendimento** completo, utilizando um **ESP32 simulado no Wokwi** e um **fluxo de dados 100% local**.  
O sistema captura a avaliação de botões físicos, envia os dados via **MQTT** e os processa no **Node-RED**.

A simulação é desenvolvida no **Visual Studio Code** com **PlatformIO**.

---

## Funcionalidades

-   **Totem de Avaliação:**  
    5 botões físicos simulados (Péssimo, Ruim, Regular, Bom, Ótimo) com 5 LEDs de feedback.

-   **Painel de Controle Web:**  
    O ESP32 hospeda um servidor web (`http://localhost:9080`) que permite configurar dinamicamente o Setor e o Local do totem.

-   **Comunicação MQTT:**  
    Ao pressionar um botão, o ESP32 envia um payload JSON para um broker MQTT local (Mosquitto) com os dados da avaliação, o setor configurado e o timestamp.

-   **Processamento de Dados:**  
    Um fluxo Node-RED local se inscreve no broker MQTT, recebe os dados do totem em tempo real e os exibe na aba de depuração.

---

## Arquitetura do Sistema Local

Este protótipo opera inteiramente na sua máquina local:

### ESP32 (Simulado no Wokwi)

-   Coleta os cliques dos botões.
-   Serve o painel de configuração (porta 80).
-   Envia dados para o Broker.

### Broker MQTT (Mosquitto)

-   Roda em `localhost:1883`.
-   Recebe os dados do ESP32 e os distribui para os inscritos.

### Processador (Node-RED)

-   Roda em `localhost:1880`.
-   Se inscreve no Broker e recebe os dados JSON para processamento.

### Wokwi IoT Gateway (Ponte)

-   Ativado pela extensão Wokwi no VSCode, permite que o ESP32 simulado (`host.wokwi.internal`) se comunique com o seu localhost (Mosquitto).

---

## Pré-requisitos

### 1. Software Básico

-   Visual Studio Code
-   Node.js (LTS) — necessário para instalar e rodar o Node-RED.

### 2. Extensões do VS Code

-   PlatformIO IDE — essencial para compilar e gerenciar o projeto C++ do ESP32.
-   Wokwi for VS Code — executa a simulação do hardware.

### 3. Serviços Locais

-   Mosquitto (MQTT Broker)
-   Node-RED

---

## Instalação e Configuração

### Clone o repositório:

```bash
git clone https://github.com/mtslma/totem-avaliacao-esp32
```

### Abra no VSCode:

Abra a pasta do projeto no Visual Studio Code  
(`Arquivo > Abrir Pasta...`).

### Instale as Bibliotecas (PlatformIO):

O PlatformIO deve detectar o arquivo `platformio.ini` e instalar automaticamente as dependências (`PubSubClient`, `ArduinoJson`, `NTPClient`) na primeira compilação.

---

### Licença Wokwi (Obrigatória para Rede)

A simulação de rede (Wokwi IoT Gateway), que permite ao ESP32 se conectar ao seu Broker MQTT, requer uma licença.

1. Obtenha sua licença em [https://wokwi.com/license](https://wokwi.com/license) (Planos **Pro** ou **Community**).
2. No VSCode, pressione `F1`, digite **"Wokwi: Activate License"** e cole sua chave.

---

### Instale o Mosquitto

1. Baixe e instale o **Mosquitto** para o seu sistema. [https://mosquitto.org/download/](https://mosquitto.org/download/)
2. Após a instalação, verifique se ele está rodando como um serviço:
    - Pressione `Win + R`, digite `services.msc`.
    - Procure por **"Mosquitto Broker"** e verifique se o status é **"Em Execução"**.

---

### Instale o Node-RED

Abra um terminal (PowerShell/CMD) e execute:

```bash
npm install -g --unsafe-perm node-red
```

---

## Como Rodar o Projeto (Passo a Passo)

A ordem de inicialização é importante.

### **Passo 1: Iniciar os Serviços Locais**

-   **Mosquitto:**  
    Verifique se o serviço (como descrito acima) está "Em Execução".  
    Se não estiver, inicie-o.

-   **Node-RED:**  
    No seu terminal, execute:
    ```bash
    node-red
    ```
    Mantenha este terminal aberto.  
    Ele mostrará que o Node-RED está rodando em [http://127.0.0.1:1880](http://127.0.0.1:1880).

---

### **Passo 2: Configurar o Node-RED**

1. Abra o navegador e acesse [http://127.0.0.1:1880](http://127.0.0.1:1880).
2. Vá para o **Menu (canto superior direito) > Importar**.
3. Clique em **"Selecionar arquivo"** e escolha o arquivo `flow.json` localizado na pasta `/node-red` deste projeto.
4. Clique em **"Importar"** e, em seguida, no botão vermelho **"Deploy"**.
5. O nó **"MQTT In"** deve mostrar um quadrado **verde ("conectado")** abaixo dele.

---

### **Passo 3: Iniciar a Simulação do ESP32**

1. No VSCode, abra a paleta de comandos (`F1` ou `Ctrl+Shift+P`).
2. Execute o comando **"Wokwi: Start Simulator"**.
3. Aguarde o Monitor Serial exibir:
    ```
    Wi-Fi conectado!
    ...
    Conectado ao Broker Local (Mosquitto)!
    Servidor HTTP iniciado! Acesse http://localhost:9080
    ```

---

### **Passo 4: Testar a Solução Completa**

#### Testar o Web Server:

1. Acesse `http://localhost:9080` no navegador.
2. Mude o "Nome do Setor" (ex: `Banheiro`) e o "ID/Local" (ex: `Totem_Entrada_02`).
3. Clique em **"Salvar Configurações"**.

#### Testar o Fluxo MQTT:

1. No simulador Wokwi, clique em um dos botões de avaliação (ex: **"Bom"**).
2. Observe a aba **"Debug"** (ícone de inseto) no Node-RED.
3. A mensagem JSON correspondente, já com os novos nomes do Setor/Local, aparecerá instantaneamente.

---
