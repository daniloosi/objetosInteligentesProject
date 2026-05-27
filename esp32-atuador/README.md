# ESP32 Atuador — Nó Subscriber (Etapa 3)

Segundo componente da arquitetura. Assina tópicos de comando do Node-RED, aciona os relés das bancadas e publica confirmação de status no broker MQTT.

## Como subir no Wokwi

1. Crie um **novo projeto ESP32** no Wokwi (separado do Sensor)
2. Substitua o `sketch.ino` e o `diagram.json` pelos arquivos deste diretório
3. A biblioteca **PubSubClient** já deve estar disponível se você a adicionou no projeto Sensor — caso contrário, adicione novamente
4. Clique em ▶ para iniciar a simulação

>  Os dois projetos (Sensor e Atuador) devem rodar **simultaneamente** em abas separadas do Wokwi para que o ciclo completo funcione.

## O que esperar no Serial Monitor

```
Sistema iniciado - ESP32 Atuador Subscriber
Conectando ao Wi-Fi.....
Wi-Fi conectado!
IP: 10.13.37.2
Conectando ao broker MQTT como esp32-atuador-lab-xyz...
Broker conectado!
Topico assinado: lab/bancada/+/comando
Bancada 01 - tomada: ENERGIZADA
Bancada 02 - tomada: ENERGIZADA
Status publicado em lab/bancada/01/status: 1
Status publicado em lab/bancada/02/status: 1
Setup finalizado. Aguardando comandos...
```

## Diagrama de hardware

| Componente | Pino ESP32 | Função |
|---|---|---|
| Relé Bancada 01 IN | GPIO 26 | Sinal de controle do relé |
| Relé Bancada 02 IN | GPIO 25 | Sinal de controle do relé |
| LED Verde B01 (+ resistor 220Ω) | GPIO 18 | Indica tomada energizada |
| LED Verde B02 (+ resistor 220Ω) | GPIO 19 | Indica tomada energizada |
| LED Onboard | GPIO 2 | Indica conexão MQTT ativa |

## Tópicos MQTT

| Tópico | Direção | Payload | Descrição |
|---|---|---|---|
| `lab/bancada/+/comando` | Entrada (subscribe) | `ON`/`OFF` ou `1`/`0` | Comando recebido do Node-RED |
| `lab/bancada/01/status` | Saída (publish) | `1` ou `0` | Confirmação do estado do relé B01 |
| `lab/bancada/02/status` | Saída (publish) | `1` ou `0` | Confirmação do estado do relé B02 |

## Checklist de testes isolados

### Teste 1 — Desligar bancada via MQTT Explorer

No MQTT Explorer, seção Publish:
- **Topic:** `lab/bancada/01/comando`
- **Payload:** `OFF`
- Clique em PUBLISH

**Resultado esperado:**
- Serial Monitor: `Comando recebido em lab/bancada/01/comando: OFF`
- Serial Monitor: `Bancada 01 - tomada: CORTADA`
- Serial Monitor: `Status publicado em lab/bancada/01/status: 0`
- No Wokwi: LED verde da B01 apaga, relé clica (muda de posição)
- No MQTT Explorer: `lab/bancada/01/status` atualiza para `0`

### Teste 2 — Religar bancada via MQTT Explorer

- **Topic:** `lab/bancada/01/comando`
- **Payload:** `ON`
- Clique em PUBLISH

**Resultado esperado:**
- LED verde da B01 acende novamente
- `lab/bancada/01/status` volta para `1`

### Teste 3 — Comando redundante (mesmo estado)

Com a bancada 01 já energizada, publique `ON` novamente.

**Resultado esperado no Serial Monitor:**
```
Bancada 01 - estado ja era o solicitado, nenhuma acao necessaria.
```
Sem nova publicação de status — evita loop de mensagens desnecessárias.

### Teste 4 — Ciclo completo Sensor → Atuador

Com os dois Wokwis rodando simultaneamente:
1. Publique `OFF` em `lab/bancada/01/comando`
2. Observe o LED do Atuador apagar
3. Observe o Serial Monitor do **Sensor** mostrar `rele energizado: nao` na próxima leitura de status
4. Observe a bateria da B01 começar a cair no MQTT Explorer

Este teste valida o ciclo completo de comunicação entre os dois dispositivos.

## Critério de aprovação desta etapa

 Wokwi mostra "Broker conectado" e "Aguardando comandos"
 LEDs acesos ao iniciar (ambas energizadas por padrão)
 Comando `OFF` apaga o LED e atualiza `status` para `0`
 Comando `ON` acende o LED e atualiza `status` para `1`
 Comando redundante não gera nova publicação de status
 Ciclo completo Sensor → Atuador funcionando com os dois Wokwis abertos

Aprovada esta etapa, avançamos para a Etapa 4 — Fluxo Node-RED.
