# Fluxo Node-RED — Etapa 4

Orquestrador central da arquitetura. Recebe telemetria dos ESP32, aplica as regras de negócio, publica comandos para o atuador, grava no InfluxDB Cloud e envia alertas via WhatsApp.

## Estrutura do fluxo

```
[MQTT In] presença/corrente/bateria/status
    │
    ▼
[Parse] extrai bancada_id, converte payload, atualiza contexto global
    │
    ├──▶ [InfluxDB Out] → measurement: telemetria / status_atuador
    │
    ├──▶ [Regra 1: Ausência 10 min]
    │        └──▶ [MQTT Out] comando OFF para todas as bancadas
    │        └──▶ [InfluxDB Out] → measurement: eventos
    │        └──▶ [WhatsApp] alerta para gestor
    │
    └──▶ [Regra 2: Ciclo de Carga]
             └──▶ [MQTT Out] comando ON/OFF por bancada
             └──▶ [InfluxDB Out] → measurement: eventos
             └──▶ [WhatsApp] alerta para gestor (só desligamentos)
```

## Como importar no Node-RED

1. Acesse o Node-RED na AWS
2. Menu **☰ → Import**
3. Clique em **select a file to import** e selecione o `flow.json`
4. Clique em **Import**
5. O fluxo aparecerá no canvas — clique em **Deploy** (botão vermelho no canto superior direito)

## Configuração obrigatória antes do Deploy

### InfluxDB Cloud
As credenciais já estão preenchidas no `config-influxdb`. Verifique se o nó está configurado clicando duas vezes em qualquer nó **InfluxDB Out** e verificando se a conexão `InfluxDB Cloud - iotProject` está selecionada.

### WhatsApp / CallMeBot 
O nó **"Montar URL CallMeBot"** contém dois placeholders que **precisam ser substituídos** antes de funcionar:

```javascript
var NUMERO_WHATSAPP = "55XXXXXXXXXXX"; // ← substituir pelo número real (ex: 5511999999999)
var API_KEY = "SUA_API_KEY_CALLMEBOT"; // ← substituir pela chave gerada no CallMeBot
```

**Como obter a API Key do CallMeBot:**
1. Salve o número `+34 644 44 23 05` na agenda como "CallMeBot"
2. Envie a mensagem: `I allow callmebot to send me messages`
3. Você receberá a API Key em alguns segundos via WhatsApp
4. Cole a key no nó do Node-RED

## Contexto global — como o estado é mantido

O fluxo usa `global.set/get("estado")` para manter o estado de cada bancada entre mensagens. A estrutura é:

```javascript
{
  "01": {
    presenca: 0,           // última leitura do sensor
    corrente: 1458.36,     // última leitura em mA
    bateria: 78.5,         // último percentual de bateria
    energizado: 1,         // último status do relé
    inicioAusencia: null,  // timestamp do início da ausência (null = presença detectada)
    ultimaPresencaTs: 1234567890
  },
  "02": { ... }
}
```

## Checklist de testes

### Teste 1 — Dados chegando no InfluxDB

Com os dois Wokwis rodando:
1. Acesse o InfluxDB Cloud → **Data Explorer**
2. Selecione bucket `iotProject`
3. Filtre por `measurement = telemetria`
4. Deve mostrar dados de `corrente`, `bateria` e `presenca` para as bancadas 01 e 02

**Critério:** dados aparecendo com timestamps atualizados a cada 10s 

### Teste 2 — Regra 1: Ausência 10 minutos

>  Para testar sem esperar 10 minutos reais, abra o nó **"Regra 1 — Ausência 10 min"** e altere temporariamente:
> ```javascript
> var TEMPO_LIMITE_MS = 1 * 60 * 1000; // 1 minuto para teste
> ```
> Lembre de restaurar para `10 * 60 * 1000` após o teste.

1. Garanta que nenhum botão está pressionado nos dois Wokwis (presença = 0 em todas)
2. Aguarde o tempo configurado
3. **Resultado esperado:**
   - Debug sidebar mostra comandos `OFF` sendo publicados
   - LEDs do Atuador apagam no Wokwi
   - `lab/bancada/01/status` e `lab/bancada/02/status` mudam para `0` no MQTT Explorer
   - Evento gravado no InfluxDB (measurement `eventos`, motivo `ausencia_excedida`)
   - Mensagem WhatsApp recebida (se CallMeBot configurado)

4. Pressione qualquer botão de presença → ambas as bancadas devem religar imediatamente

### Teste 3 — Regra 2: Bateria cheia (100%)

A bateria sobe automaticamente quando o botão está pressionado (bancada ocupada + relé ON).

1. Pressione e segure o botão da Bancada 01 no Wokwi Sensor
2. Aguarde a bateria chegar a 100% (visível no MQTT Explorer em `lab/bancada/01/bateria`)
3. **Resultado esperado:**
   - Node-RED publica `OFF` em `lab/bancada/01/comando`
   - LED da B01 apaga no Wokwi Atuador
   - Evento gravado com motivo `bateria_carregada`
4. Solte o botão → bateria começa a cair
5. Quando chegar a 20% → Node-RED publica `ON` → LED acende novamente

### Teste 4 — Verificar todos os measurements no InfluxDB

No Data Explorer do InfluxDB Cloud, verifique se os 4 measurements estão sendo populados:
- `telemetria` — dados contínuos 
- `status_atuador` — muda quando relé é acionado 
- `eventos` — aparece após qualquer intervenção automática 
- `notificacoes` — aparece após cada tentativa de envio WhatsApp 

## Critério de aprovação desta etapa

 Fluxo importado e deployado sem erros (sem nós com borda vermelha)
 Dados chegando no measurement `telemetria` do InfluxDB
 Regra 1 (ausência) dispara comando OFF e grava evento
 Religamento por presença funciona após Regra 1
 Regra 2 (bateria 100%) desliga a bancada correta
 Religamento por bateria baixa (20%) funciona
 Measurement `eventos` populado com `tipo` e `motivo` corretos
 WhatsApp recebendo alertas (após configurar CallMeBot)

Aprovada esta etapa, avançamos para a Etapa 6 — Dashboards no Grafana.
