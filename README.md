#  Gestão Inteligente de Energia em Laboratórios Acadêmicos de Informática

> Projeto de IoT desenvolvido na Universidade Presbiteriana Mackenzie — Faculdade de Computação e Informática

Sistema de automação do fornecimento de energia em bancadas de laboratório, que combina sensores de presença e medição de corrente elétrica para identificar desperdício e agir de forma autônoma, com visualização gerencial em tempo real e notificações via WhatsApp.

---

##  Índice

- [Sobre o Projeto](#sobre-o-projeto)
- [Arquitetura](#arquitetura)
- [Tecnologias Utilizadas](#tecnologias-utilizadas)
- [Pré-requisitos](#pré-requisitos)
- [Estrutura do Repositório](#estrutura-do-repositório)
- [Como Executar](#como-executar)
- [Regras de Negócio](#regras-de-negócio)
- [Tópicos MQTT](#tópicos-mqtt)
- [Schema do Banco de Dados](#schema-do-banco-de-dados)
- [Dashboards](#dashboards)
- [Autores](#autores)
- [Vídeo](#vídeo)

---

## Sobre o Projeto

Em laboratórios estudantis de informática, notebooks e monitores frequentemente permanecem conectados à rede elétrica mesmo quando o ambiente está vazio, gerando desperdício energético e acelerando a degradação das baterias por sobrecarga contínua.

Este projeto propõe um protótipo de **tomada inteligente baseado em IoT** que automatiza o fornecimento de energia por meio do cruzamento de dados de ocupação física com a medição de corrente elétrica, contribuindo para os **ODS 7** (Energia Limpa e Acessível) e **ODS 12** (Consumo e Produção Responsáveis) da Agenda 2030 da ONU.

---

## Arquitetura

```
┌─────────────────────────────────────────────────────────────────┐
│                        WOKWI (Simulação)                        │
│                                                                 │
│  ┌──────────────────┐          ┌──────────────────────────┐    │
│  │  ESP32 Sensor    │          │     ESP32 Atuador         │    │
│  │  (Publisher)     │          │     (Subscriber)          │    │
│  │                  │          │                           │    │
│  │  • Botão (PIR)   │          │  • Relé Bancada 01        │    │
│  │  • Potenciômetro │          │  • Relé Bancada 02        │    │
│  │    (ACS712)      │          │  • LEDs indicadores       │    │
│  └────────┬─────────┘          └────────────┬──────────────┘    │
└───────────┼─────────────────────────────────┼───────────────────┘
            │ MQTT Publish                     │ MQTT Subscribe
            ▼                                 ▼
┌───────────────────────────────────────────────────────────────┐
│                  HiveMQ Broker (broker.hivemq.com)            │
└───────────────────────────┬───────────────────────────────────┘
                            │
                            ▼
┌───────────────────────────────────────────────────────────────┐
│                    Node-RED (AWS)                             │
│                                                               │
│  Subscribe telemetria → Regras de negócio → Publish comandos  │
│                       ↓                ↓                      │
│               InfluxDB Cloud     CallMeBot API                │
│               (persistência)     (WhatsApp)                   │
└───────────────────────────┬───────────────────────────────────┘
                            │
                            ▼
┌───────────────────────────────────────────────────────────────┐
│                    Grafana Cloud                              │
│              (Dashboards gerenciais)                          │
└───────────────────────────────────────────────────────────────┘
```

---

## Tecnologias Utilizadas

| Tecnologia | Versão | Função |
|---|---|---|
| [Wokwi](https://wokwi.com) | — | Simulação dos ESP32 |
| ESP32 + Arduino Framework | — | Firmware dos dispositivos |
| [PubSubClient](https://github.com/knolleary/pubsubclient) | 2.8+ | Cliente MQTT para ESP32 |
| [HiveMQ](https://www.hivemq.com) | Público | Broker MQTT |
| [Node-RED](https://nodered.org) | 4.1.8 | Orquestração e regras de negócio |
| [node-red-contrib-influxdb](https://flows.nodered.org/node/node-red-contrib-influxdb) | — | Integração InfluxDB no Node-RED |
| [node-red-contrib-whatsapp-cmb](https://flows.nodered.org/node/node-red-contrib-whatsapp-cmb) | — | Notificações WhatsApp |
| [InfluxDB Cloud](https://cloud2.influxdata.com) | v3.x | Banco de séries temporais |
| [Grafana Cloud](https://grafana.com) | — | Dashboards e visualização |
| [CallMeBot](https://www.callmebot.com) | — | API de notificações WhatsApp |

---

## Pré-requisitos

- Conta gratuita no [Wokwi](https://wokwi.com)
- Conta gratuita no [InfluxDB Cloud](https://cloud.influxdata.com)
- Conta gratuita no [Grafana Cloud](https://grafana.com)
- Node-RED instalado (local ou em nuvem)
  - Nodes adicionais: `node-red-contrib-influxdb`, `node-red-contrib-whatsapp-cmb`
- API Key do [CallMeBot](https://www.callmebot.com/blog/free-api-whatsapp-messages/) (gratuita)

---

## Estrutura do Repositório

```
 iot-gestao-energia/
├──  esp32-sensor/
│   ├── sketch.ino          # Firmware do Nó Sensor (Publisher)
│   ├── diagram.json        # Diagrama de hardware Wokwi
│   ├── libraries.txt       # Bibliotecas necessárias
│   └── README.md           # Instruções e checklist de teste
│
├──  esp32-atuador/
│   ├── sketch.ino          # Firmware do Nó Atuador (Subscriber)
│   ├── diagram.json        # Diagrama de hardware Wokwi
│   └── README.md           # Instruções e checklist de teste
│
├──  node-red/
│   ├── flow.json           # Fluxo completo exportado
│   └── README.md           # Instruções de importação e configuração
│
└── README.md               # Este arquivo
```

---

## Como Executar

### 1. ESP32 Sensor (Nó Publisher)

1. Acesse [wokwi.com](https://wokwi.com) e crie um novo projeto **ESP32**
2. Cole o conteúdo de `esp32-sensor/sketch.ino` no editor
3. Cole o conteúdo de `esp32-sensor/diagram.json` no diagrama
4. Adicione a biblioteca **PubSubClient** pelo Library Manager
5. Clique em ▶ para iniciar a simulação

### 2. ESP32 Atuador (Nó Subscriber)

1. Crie um **segundo projeto ESP32** no Wokwi (aba separada)
2. Cole o conteúdo de `esp32-atuador/sketch.ino` no editor
3. Cole o conteúdo de `esp32-atuador/diagram.json` no diagrama
4. Clique em ▶ — mantenha os dois projetos rodando simultaneamente

### 3. Node-RED

1. Acesse sua instância do Node-RED
2. Menu **☰ → Import → select a file** → selecione `node-red/flow.json`
3. Configure as credenciais (ver seção abaixo)
4. Clique em **Deploy**

#### Configurações obrigatórias no Node-RED

**InfluxDB Cloud** — edite o nó de configuração `InfluxDB Cloud`:

| Campo | Valor |
|---|---|
| Version | 2.0 |
| URL | URL da sua instância InfluxDB Cloud |
| Token | Token de API com permissão de escrita |
| Organization | ID da sua organização |
| Bucket | Nome do bucket |

Nos nós **HTTP Request** (escrita no InfluxDB), atualize a URL:
```
https://<sua-url>/api/v2/write?org=<sua-org>&bucket=<seu-bucket>&precision=s
```
E o header `Authorization`:
```
Token <seu-token>
```

**WhatsApp / CallMeBot** — edite o nó `Montar MSG WhatsApp`:
```javascript
var NUMERO_WHATSAPP = "55XXXXXXXXXXX"; // seu número com DDI+DDD
var API_KEY = "SUA_API_KEY_CALLMEBOT"; // chave gerada pelo CallMeBot
```

>  Para obter a API Key do CallMeBot: salve o número +34 644 44 23 05 e envie a mensagem `I allow callmebot to send me messages` via WhatsApp.

### 4. InfluxDB Cloud

Crie um bucket com o nome configurado no Node-RED. Os measurements são criados automaticamente na primeira gravação:

| Measurement | Descrição |
|---|---|
| `telemetria` | Leituras contínuas de presença, corrente e bateria |
| `status_atuador` | Transições de estado do relé |
| `eventos` | Log de intervenções automáticas |
| `notificacoes` | Resultado das chamadas à API WhatsApp |

### 5. Grafana Cloud

1. Adicione o InfluxDB como data source (**Connections → Data sources → InfluxDB**)
   - Query Language: **SQL**
   - URL: URL da sua instância InfluxDB Cloud
   - Token: seu token de API
2. Importe ou recrie os painéis conforme descrito na seção [Dashboards](#dashboards)

---

## Regras de Negócio

### Regra 1 — Ausência no Laboratório

| Parâmetro | Valor |
|---|---|
| Condição de gatilho | Ausência de presença em **todas** as bancadas |
| Tempo limite | 10 minutos contínuos |
| Ação | Desliga todas as tomadas (`OFF`) |
| Religamento | Imediato ao detectar qualquer presença |

### Regra 2 — Ciclo Inteligente de Carga

| Condição | Ação |
|---|---|
| Bateria ≥ 100% | Desliga a tomada da bancada correspondente |
| Bateria ≤ 20% | Religa a tomada da bancada correspondente |

---

## Tópicos MQTT

```
lab/bancada/{id}/presenca    → Sensor publica  | 0 = vazio, 1 = ocupado
lab/bancada/{id}/corrente    → Sensor publica  | valor em mA (float)
lab/bancada/{id}/bateria     → Sensor publica  | percentual (float)
lab/bancada/{id}/comando     → Node-RED publica | "ON" ou "OFF"
lab/bancada/{id}/status      → Atuador publica | 0 = cortado, 1 = energizado
```

`{id}` = `01`, `02`, ... (escalável para N bancadas sem alteração do fluxo)

---

## Schema do Banco de Dados

### `telemetria`
```
tags:   bancada_id, laboratorio_id
fields: presenca (int), corrente_ma (float), bateria_pct (float)
freq:   a cada 10 segundos
```

### `status_atuador`
```
tags:   bancada_id
fields: energizado (int 0/1)
freq:   por transição de estado
```

### `eventos`
```
tags:   bancada_id, tipo (desligamento|religamento),
        motivo (ausencia_excedida|bateria_carregada|presenca_detectada|bateria_baixa)
fields: duracao_estado_anterior_s (int)
freq:   por intervenção automática
```

### `notificacoes`
```
tags:   bancada_id, canal (whatsapp)
fields: sucesso (int 0/1), http_status (int), latencia_ms (int)
freq:   por tentativa de envio
```

---

## Dashboards

O dashboard **Gestão Inteligente de Energia — Lab 01** é composto por 4 painéis:

| Painel | Tipo | Descrição |
|---|---|---|
| Desligamentos Automáticos | Stat | Total de intervenções de desligamento |
| Consumo Total | Stat | Consumo elétrico estimado em Wh |
| Economia Estimada | Stat | Percentual de economia gerado |
| Status em Tempo Real | Table | Ocupação e estado das tomadas por bancada |
| Consumo de Energia por Bancada | Time series | Corrente elétrica ao longo do tempo |
| Histórico de Desligamentos | Bar chart | Intervenções por bancada agrupadas por data |

---

## Autores

| Nome | RA |
|---|---|
| Arthur Santos | 10437356 |
| Danilo Oliveira | 10410905 |
| Wallace Santana | 1165744 |

**Universidade Presbiteriana Mackenzie** — Faculdade de Computação e Informática  
Disciplina: Objetos Inteligentes Conectados

---

## Vídeo

O funcionamento de ponta a ponta do protótipo, a validação das regras de negócio no Node-RED e a exibição dos indicadores em tempo real podem ser acompanhados no vídeo abaixo:

**Link:** [https://youtu.be/xBdChfcQaL4](https://youtu.be/xBdChfcQaL4)

## Licença

Este projeto foi desenvolvido para fins acadêmicos.
