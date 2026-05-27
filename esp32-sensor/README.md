# ESP32 Sensor — Nó Publisher (Etapa 2)

Primeiro componente da arquitetura. Lê presença e corrente de 2 bancadas e publica telemetria no broker MQTT público HiveMQ.

## Como subir no Wokwi

1. Acesse https://wokwi.com e crie um novo projeto **ESP32**
2. Substitua o conteúdo de `sketch.ino` pelo arquivo deste diretório
3. Substitua o conteúdo de `diagram.json` pelo arquivo deste diretório
4. Adicione a biblioteca **PubSubClient** pela aba *Library Manager* do Wokwi (ícone de livro no canto esquerdo) — pesquise por `PubSubClient` (autor: Nick O'Leary)
5. Clique em ▶ para iniciar a simulação

## O que esperar no Serial Monitor

```
=== ESP32 Sensor (Publisher) — Gestão Inteligente de Energia ===
[WiFi] Conectando a Wokwi-GUEST.....
[WiFi] Conectado. IP: 10.13.37.2
[MQTT] Conectando como esp32-sensor-lab-abc123... OK
[MQTT] Assinado: lab/bancada/+/status
[SETUP] Pronto.
[TX] Bancada 01 | presenca=0 | corrente=0.00 mA | bateria=50.00%
[TX] Bancada 02 | presenca=0 | corrente=0.00 mA | bateria=50.00%
```

A cada **10 segundos** uma nova rodada de publicações deve aparecer.

## Como validar que está funcionando (teste isolado)

Você precisa de um cliente MQTT para "espiar" o broker e confirmar que as mensagens estão chegando. Recomendo o **MQTT Explorer** (gratuito, multiplataforma): http://mqtt-explorer.com/

### Configuração do MQTT Explorer
- **Name:** HiveMQ Público
- **Protocol:** mqtt://
- **Host:** broker.hivemq.com
- **Port:** 1883
- **Validate certificate:** desativado
- **Encryption:** desativado
- Deixe Username/Password em branco

### O que você deve ver no MQTT Explorer
Uma árvore como esta vai aparecer:
```
lab/
└── bancada/
    ├── 01/
    │   ├── presenca   → 0 ou 1
    │   ├── corrente   → 0.00 a 2000.00
    │   └── bateria    → 0.00 a 100.00
    └── 02/
        ├── presenca   → 0 ou 1
        ├── corrente   → 0.00 a 2000.00
        └── bateria    → 0.00 a 100.00
```

### Testes a realizar no Wokwi

| Ação no Wokwi | Resultado esperado |
|---|---|
| Pressionar e segurar o botão verde (B01) | `lab/bancada/01/presenca` muda para `1` na próxima publicação |
| Soltar o botão verde | `lab/bancada/01/presenca` volta para `0` |
| Girar o potenciômetro 1 | `lab/bancada/01/corrente` varia de 0 a ~2000 |
| Girar o potenciômetro 2 | `lab/bancada/02/corrente` varia de 0 a ~2000 |
| Aguardar (sem fazer nada) | `bateria` permanece em 50% (relé default = energizado, sem presença = estável) |
| Segurar botão verde por 1 min | `bateria` da B01 sobe gradualmente (carga ativa) |

### Testando a simulação de bateria (subscriber)

Publique manualmente um comando no tópico de status para simular o atuador:

No MQTT Explorer, vá em "Publish":
- **Topic:** `lab/bancada/01/status`
- **Payload:** `0`
- Clique em Publish

O Serial Monitor do ESP32 deve mostrar:
```
[MQTT-RX] lab/bancada/01/status -> 0
[STATE] Bancada 01 relayEnergizado=false
```

A partir daí, a bateria da B01 começa a **descer** (relé cortado, simulando uso em bateria pura).

Publique `1` no mesmo tópico para reativar e ver a carga voltando (se o botão estiver pressionado).

## Critério de aprovação desta etapa

✅ Wokwi mostra "WiFi Conectado" e "MQTT Conectando OK"  
✅ MQTT Explorer recebe mensagens nos 6 tópicos a cada 10s  
✅ Pressionar botão muda o valor de `presenca` na próxima publicação  
✅ Girar potenciômetro varia o valor de `corrente`  
✅ Publicar manualmente em `status` é refletido na simulação de bateria

Só avance para a Etapa 3 (ESP32 Atuador) depois que todos os critérios acima estiverem ok.
