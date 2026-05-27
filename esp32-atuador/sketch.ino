// Incluir bibliotecas
#include <WiFi.h>           // biblioteca para conexao com redes Wi-Fi
#include <PubSubClient.h>   // biblioteca para comunicacao com broker MQTT

// Definicoes e constantes
char SSIDName[] = "Wokwi-GUEST";        // nome da rede Wi-Fi usada no simulador Wokwi
char SSIDPass[] = "";                   // senha vazia, rede Wokwi-GUEST e aberta

char BrokerURL[] = "broker.hivemq.com"; // URL do broker MQTT publico HiveMQ
char BrokerUserName[] = "";             // sem autenticacao no broker publico
char BrokerPassword[] = "";             // sem autenticacao no broker publico
char MQTTClientName[] = "esp32-atuador-lab-"; // prefixo do nome do cliente MQTT
int BrokerPort = 1883;                  // porta padrao MQTT sem TLS

// Topicos assinados pelo Atuador
char TopicoComando[] = "lab/bancada/+/comando"; // recebe comandos ON/OFF do Node-RED

// Pinos dos reles (um por bancada)
const int PIN_RELE_BANCADA_01 = 26;  // pino do modulo rele da bancada 01
const int PIN_RELE_BANCADA_02 = 25;  // pino do modulo rele da bancada 02

// Pinos dos LEDs de status (indicacao visual do estado do rele)
const int PIN_LED_BANCADA_01 = 18;   // LED verde indica tomada energizada - bancada 01
const int PIN_LED_BANCADA_02 = 19;   // LED verde indica tomada energizada - bancada 02

const int PIN_LED_CONEXAO = 2;       // LED onboard indica conexao MQTT ativa

const int NUM_BANCADAS = 2;          // quantidade de bancadas controladas

// Logica do rele: RELE_LIGADO e o nivel logico que ENERGIZA a tomada
// Modulos de rele tipicos sao acionados em nivel LOW (ativo baixo)
// No Wokwi o componente wokwi-relay-module segue essa mesma logica
const int RELE_LIGADO  = LOW;
const int RELE_DESLIGADO = HIGH;

// Variaveis globais e objetos
WiFiClient espClient;                   // instancia o objeto de conexao Wi-Fi
PubSubClient clienteMQTT(espClient);    // instancia o cliente MQTT

struct Bancada {                        // estrutura para armazenar dados de cada bancada
  const char* id;                       // identificador da bancada, ex: "01", "02"
  int pinRele;                          // pino do modulo rele
  int pinLed;                           // pino do LED de status
  bool energizado;                      // estado atual da tomada (true = energizada)
};

Bancada bancadas[NUM_BANCADAS] = {
  {"01", PIN_RELE_BANCADA_01, PIN_LED_BANCADA_01, true},  // bancada 01 inicia energizada
  {"02", PIN_RELE_BANCADA_02, PIN_LED_BANCADA_02, true}   // bancada 02 inicia energizada
};

// Funcoes definidas pelo usuario

// funcao para aplicar o estado do rele e LED de uma bancada
void aplicarEstadoBancada(int indiceBancada) {
  Bancada& b = bancadas[indiceBancada]; // referencia para a bancada pelo indice

  // Aciona o rele conforme o estado atual
  digitalWrite(b.pinRele, b.energizado ? RELE_LIGADO : RELE_DESLIGADO);

  // Atualiza o LED de status (HIGH = aceso = energizado)
  digitalWrite(b.pinLed, b.energizado ? HIGH : LOW);

  Serial.print("Bancada ");
  Serial.print(b.id);
  Serial.print(" - tomada: ");
  Serial.println(b.energizado ? "ENERGIZADA" : "CORTADA");
}

// funcao para publicar confirmacao de status no topico MQTT
void publicarStatus(int indiceBancada) {
  Bancada& b = bancadas[indiceBancada];

  char topico[48];
  char payload[4];

  snprintf(topico, sizeof(topico), "lab/bancada/%s/status", b.id); // monta o topico de status
  snprintf(payload, sizeof(payload), "%d", b.energizado ? 1 : 0);  // "1" = energizado, "0" = cortado

  clienteMQTT.publish(topico, payload, true); // publica com retain=true para persistir o ultimo estado

  Serial.print("Status publicado em ");
  Serial.print(topico);
  Serial.print(": ");
  Serial.println(payload);
}

// funcao chamada automaticamente quando uma mensagem MQTT e recebida
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char mensagem[8] = {0}; // vetor para armazenar o comando recebido

  if (length >= sizeof(mensagem)) {
    length = sizeof(mensagem) - 1;
  }
  memcpy(mensagem, payload, length); // copia o payload para o vetor mensagem

  Serial.print("Comando recebido em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensagem);

  // Percorre as bancadas para identificar qual recebeu o comando
  for (int i = 0; i < NUM_BANCADAS; i++) {
    char topicoEsperado[48];
    snprintf(topicoEsperado, sizeof(topicoEsperado), "lab/bancada/%s/comando", bancadas[i].id);

    if (strcmp(topic, topicoEsperado) == 0) { // verifica se o topico pertence a esta bancada
      // Aceita "ON", "on", "1" para energizar e "OFF", "off", "0" para cortar
      bool novoEstado = (strcmp(mensagem, "ON")  == 0 ||
                         strcmp(mensagem, "on")  == 0 ||
                         strcmp(mensagem, "1")   == 0);

      if (novoEstado != bancadas[i].energizado) { // so age se houver mudanca de estado
        bancadas[i].energizado = novoEstado;
        aplicarEstadoBancada(i);  // aciona rele e LED
        publicarStatus(i);        // confirma a execucao via MQTT
      } else {
        Serial.print("Bancada ");
        Serial.print(bancadas[i].id);
        Serial.println(" - estado ja era o solicitado, nenhuma acao necessaria.");
      }
      return;
    }
  }

  Serial.println("Topico de comando nao reconhecido, ignorando.");
}

// funcao para conectar o ESP32 a rede Wi-Fi
void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSIDName, SSIDPass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// funcao para conectar ou reconectar ao broker MQTT
void mqttReconnect() {
  while (!clienteMQTT.connected()) {
    String clienteId = String(MQTTClientName) + String((uint32_t)ESP.getEfuseMac(), HEX); // ID unico

    Serial.print("Conectando ao broker MQTT como ");
    Serial.println(clienteId);

    if (clienteMQTT.connect(clienteId.c_str(), BrokerUserName, BrokerPassword)) {
      Serial.println("Broker conectado!");

      clienteMQTT.subscribe(TopicoComando); // assina os comandos do Node-RED
      Serial.print("Topico assinado: ");
      Serial.println(TopicoComando);

      digitalWrite(PIN_LED_CONEXAO, HIGH); // acende LED onboard indicando conexao ativa

      // Publica o estado inicial de todas as bancadas ao reconectar
      for (int i = 0; i < NUM_BANCADAS; i++) {
        publicarStatus(i);
      }

    } else {
      Serial.print("Falha MQTT, rc=");
      Serial.println(clienteMQTT.state());
      Serial.println("Tentando novamente em 3 segundos...");
      digitalWrite(PIN_LED_CONEXAO, LOW);
      delay(3000);
    }
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("Sistema iniciado - ESP32 Atuador Subscriber");

  // Configura pinos dos reles como saida
  pinMode(PIN_RELE_BANCADA_01, OUTPUT);
  pinMode(PIN_RELE_BANCADA_02, OUTPUT);

  // Configura pinos dos LEDs como saida
  pinMode(PIN_LED_BANCADA_01, OUTPUT);
  pinMode(PIN_LED_BANCADA_02, OUTPUT);
  pinMode(PIN_LED_CONEXAO, OUTPUT);

  // Aplica estado inicial (ambas energizadas)
  for (int i = 0; i < NUM_BANCADAS; i++) {
    aplicarEstadoBancada(i);
  }

  clienteMQTT.setServer(BrokerURL, BrokerPort);
  clienteMQTT.setCallback(mqttCallback);

  conectarWiFi();
  mqttReconnect();

  Serial.println("Setup finalizado. Aguardando comandos...");
}

// Loop
void loop() {
  if (!clienteMQTT.connected()) {
    digitalWrite(PIN_LED_CONEXAO, LOW); // apaga LED ao perder conexao
    mqttReconnect();
  }

  clienteMQTT.loop(); // mantem conexao ativa e processa mensagens recebidas
}
