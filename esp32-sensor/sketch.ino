//Incluir bibliotecas
#include <WiFi.h> //biblioteca para conexao com redes Wi-Fi
#include <PubSubClient.h> //biblioteca para comunicacao com broker MQTT

//Definicoes e constantes
char SSIDName[] = "Wokwi-GUEST"; //nome da rede Wi-Fi usada no simulador Wokwi
char SSIDPass[] = ""; //senha da rede Wi-Fi, vazia porque a rede Wokwi-GUEST e aberta

char BrokerURL[] = "broker.hivemq.com"; //URL do broker MQTT publico HiveMQ
char BrokerUserName[] = ""; //nome do usuario para autenticar no broker MQTT, vazio para broker publico
char BrokerPassword[] = ""; //senha para autenticar no broker MQTT, vazia para broker publico
char MQTTClientName[] = "esp32-sensor-lab-"; //prefixo do nome do cliente MQTT
int BrokerPort = 1883; //porta padrao MQTT sem TLS

char TopicoStatus[] = "lab/bancada/+/status"; //topico assinado para receber status dos atuadores

const int PIN_BOTAO_BANCADA_01 = 14; //pino do botao que simula presenca na bancada 01
const int PIN_POT_BANCADA_01 = 34; //pino do potenciometro que simula corrente na bancada 01
const int PIN_BOTAO_BANCADA_02 = 27; //pino do botao que simula presenca na bancada 02
const int PIN_POT_BANCADA_02 = 35; //pino do potenciometro que simula corrente na bancada 02
const int PIN_LED_PUBLICANDO = 2; //LED onboard usado para indicar publicacao MQTT

const int NUM_BANCADAS = 2; //quantidade de bancadas monitoradas pelo sistema

const unsigned long INTERVALO_PUBLICACAO_MS = 10000; //intervalo de publicacao dos dados, em milissegundos
const float CORRENTE_MAX_MA = 2000.0; //corrente maxima simulada pelo potenciometro, em mA

const float BATERIA_TAXA_CARGA_PCT_S = 0.5; //taxa de aumento da bateria por segundo quando esta carregando
const float BATERIA_TAXA_DESCARGA_PCT_S = 0.2; //taxa de queda da bateria por segundo quando esta descarregando
const float BATERIA_MIN_PCT = 0.0; //valor minimo permitido para a bateria
const float BATERIA_MAX_PCT = 100.0; //valor maximo permitido para a bateria

//Variaveis globais e objetos
WiFiClient espClient; //instancia o objeto espClient para conexao com a Internet
PubSubClient clienteMQTT(espClient); //instancia o cliente MQTT usando a conexao Wi-Fi

struct Bancada { //estrutura para armazenar as informacoes de cada bancada
  const char* id; //identificador da bancada, por exemplo "01" ou "02"
  int pinBotao; //pino do botao de presenca
  int pinPot; //pino do potenciometro de corrente
  bool relayEnergizado; //estado do rele recebido pelo topico MQTT de status
  float bateriaPct; //percentual atual da bateria simulada
};

Bancada bancadas[NUM_BANCADAS] = { //vetor com as bancadas monitoradas
  {"01", PIN_BOTAO_BANCADA_01, PIN_POT_BANCADA_01, true, 50.0}, //bancada 01 inicia com rele ligado e bateria em 50%
  {"02", PIN_BOTAO_BANCADA_02, PIN_POT_BANCADA_02, true, 50.0} //bancada 02 inicia com rele ligado e bateria em 50%
};

unsigned long ultimaPublicacaoMs = 0; //armazena o instante da ultima publicacao MQTT
unsigned long ultimaAtualizacaoBateriaMs = 0; //armazena o instante da ultima atualizacao das baterias

//Funcoes definidas pelo usuario

//funcao para conectar o ESP32 a rede Wi-Fi
void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi");

  WiFi.mode(WIFI_STA); //configura o ESP32 como estacao Wi-Fi
  WiFi.begin(SSIDName, SSIDPass); //inicializa a conexao Wi-Fi com nome e senha definidos

  while (WiFi.status() != WL_CONNECTED) { //repete enquanto a conexao com a rede nao for estabelecida
    delay(250);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP()); //imprime o endereco IP recebido pelo ESP32
}

//funcao chamada automaticamente quando uma mensagem MQTT e recebida
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char mensagem[16] = {0}; //vetor para armazenar a mensagem recebida como texto

  if (length >= sizeof(mensagem)) { //garante que a mensagem nao ultrapasse o tamanho do vetor
    length = sizeof(mensagem) - 1;
  }

  memcpy(mensagem, payload, length); //copia o conteudo recebido para o vetor mensagem

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensagem);

  for (int i = 0; i < NUM_BANCADAS; i++) { //percorre todas as bancadas cadastradas
    char topicoEsperado[40]; //vetor para montar o topico esperado de cada bancada
    snprintf(topicoEsperado, sizeof(topicoEsperado), "lab/bancada/%s/status", bancadas[i].id); //monta o topico de status da bancada atual

    if (strcmp(topic, topicoEsperado) == 0) { //verifica se o topico recebido pertence a bancada atual
      bancadas[i].relayEnergizado = (strcmp(mensagem, "1") == 0 || strcmp(mensagem, "ON") == 0 || strcmp(mensagem, "on") == 0); //atualiza o estado do rele

      Serial.print("Bancada ");
      Serial.print(bancadas[i].id);
      Serial.print(" - rele energizado: ");
      Serial.println(bancadas[i].relayEnergizado ? "sim" : "nao");
      return;
    }
  }
}

//funcao para reconectar ao broker MQTT
void mqttReconnect() {
  while (!clienteMQTT.connected()) { //repete enquanto o cliente MQTT nao estiver conectado
    String clienteId = String(MQTTClientName) + String((uint32_t)ESP.getEfuseMac(), HEX); //gera um nome unico para o cliente MQTT

    Serial.print("Conectando ao broker MQTT como ");
    Serial.println(clienteId);

    if (clienteMQTT.connect(clienteId.c_str(), BrokerUserName, BrokerPassword)) { //tenta conectar ao broker MQTT
      Serial.println("Broker conectado!");
      clienteMQTT.subscribe(TopicoStatus); //assina o topico de status dos atuadores
      Serial.print("Topico assinado: ");
      Serial.println(TopicoStatus);
    } else {
      Serial.print("Falha MQTT, rc=");
      Serial.println(clienteMQTT.state()); //imprime o codigo de erro da conexao MQTT
      Serial.println("Tentando novamente em 3 segundos...");
      delay(3000);
    }
  }
}

//funcao para ler a presenca em uma bancada
bool lerPresenca(int pinBotao) {
  return digitalRead(pinBotao) == LOW; //como o botao usa INPUT_PULLUP, pressionado significa LOW
}

//funcao para ler a corrente simulada pelo potenciometro
float lerCorrenteMa(int pinPot) {
  int leituraADC = analogRead(pinPot); //realiza a leitura analogica do pino informado
  float corrente = (leituraADC / 4095.0) * CORRENTE_MAX_MA; //converte a leitura ADC para corrente em mA
  return corrente; //retorna o valor calculado da corrente
}

//funcao para atualizar a simulacao da bateria das bancadas
void atualizarBaterias() {
  unsigned long agora = millis(); //obtem o tempo atual desde que o ESP32 foi ligado

  if (ultimaAtualizacaoBateriaMs == 0) { //verifica se e a primeira atualizacao
    ultimaAtualizacaoBateriaMs = agora;
    return;
  }

  float deltaSegundos = (agora - ultimaAtualizacaoBateriaMs) / 1000.0; //calcula o tempo decorrido em segundos

  if (deltaSegundos < 1.0) { //evita atualizar a bateria em intervalos muito pequenos
    return;
  }

  for (int i = 0; i < NUM_BANCADAS; i++) { //percorre todas as bancadas
    bool ocupado = lerPresenca(bancadas[i].pinBotao); //verifica se ha presenca na bancada atual

    if (bancadas[i].relayEnergizado && ocupado) { //se o rele esta ligado e existe presenca, a bateria carrega
      bancadas[i].bateriaPct += BATERIA_TAXA_CARGA_PCT_S * deltaSegundos;
    } else if (!bancadas[i].relayEnergizado) { //se o rele esta desligado, a bateria descarrega
      bancadas[i].bateriaPct -= BATERIA_TAXA_DESCARGA_PCT_S * deltaSegundos;
    }

    if (bancadas[i].bateriaPct > BATERIA_MAX_PCT) { //limita a bateria ao valor maximo definido
      bancadas[i].bateriaPct = BATERIA_MAX_PCT;
    }

    if (bancadas[i].bateriaPct < BATERIA_MIN_PCT) { //limita a bateria ao valor minimo definido
      bancadas[i].bateriaPct = BATERIA_MIN_PCT;
    }
  }

  ultimaAtualizacaoBateriaMs = agora; //atualiza o instante da ultima simulacao de bateria
}

//funcao para publicar os dados das bancadas no broker MQTT
void publicarTelemetria() {
  digitalWrite(PIN_LED_PUBLICANDO, HIGH); //acende o LED para indicar inicio da publicacao

  for (int i = 0; i < NUM_BANCADAS; i++) { //percorre todas as bancadas cadastradas
    char topico[48]; //vetor para armazenar o nome do topico MQTT
    char payload[16]; //vetor para armazenar a mensagem MQTT

    bool ocupado = lerPresenca(bancadas[i].pinBotao); //le a presenca da bancada atual
    float corrente = lerCorrenteMa(bancadas[i].pinPot); //le a corrente simulada da bancada atual
    float bateria = bancadas[i].bateriaPct; //obtem o percentual de bateria da bancada atual

    snprintf(topico, sizeof(topico), "lab/bancada/%s/presenca", bancadas[i].id); //monta o topico de presenca
    snprintf(payload, sizeof(payload), "%d", ocupado ? 1 : 0); //converte a presenca para texto
    clienteMQTT.publish(topico, payload); //publica a presenca no broker MQTT

    snprintf(topico, sizeof(topico), "lab/bancada/%s/corrente", bancadas[i].id); //monta o topico de corrente
    snprintf(payload, sizeof(payload), "%.2f", corrente); //converte a corrente para texto com duas casas decimais
    clienteMQTT.publish(topico, payload); //publica a corrente no broker MQTT

    snprintf(topico, sizeof(topico), "lab/bancada/%s/bateria", bancadas[i].id); //monta o topico de bateria
    snprintf(payload, sizeof(payload), "%.2f", bateria); //converte a bateria para texto com duas casas decimais
    clienteMQTT.publish(topico, payload); //publica a bateria no broker MQTT

    Serial.print("Bancada ");
    Serial.print(bancadas[i].id);
    Serial.print(" | presenca: ");
    Serial.print(ocupado ? 1 : 0);
    Serial.print(" | corrente: ");
    Serial.print(corrente);
    Serial.print(" mA | bateria: ");
    Serial.print(bateria);
    Serial.println("%");
  }

  digitalWrite(PIN_LED_PUBLICANDO, LOW); //apaga o LED para indicar fim da publicacao
}

//Setup
void setup() {
  Serial.begin(115200); //inicializa a comunicacao serial
  delay(500); //pequena pausa para estabilizar a inicializacao

  Serial.println("Sistema iniciado - ESP32 Sensor Publisher");

  pinMode(PIN_LED_PUBLICANDO, OUTPUT); //configura o LED como saida
  pinMode(PIN_BOTAO_BANCADA_01, INPUT_PULLUP); //configura o botao da bancada 01 com resistor interno de pull-up
  pinMode(PIN_BOTAO_BANCADA_02, INPUT_PULLUP); //configura o botao da bancada 02 com resistor interno de pull-up

  clienteMQTT.setServer(BrokerURL, BrokerPort); //configura o broker MQTT passando URL e porta
  clienteMQTT.setCallback(mqttCallback); //define a funcao que sera chamada ao receber mensagens MQTT

  conectarWiFi(); //conecta o ESP32 a rede Wi-Fi
  mqttReconnect(); //conecta o ESP32 ao broker MQTT

  Serial.println("Setup finalizado. Sistema pronto.");
}

//Loop
void loop() {
  if (!clienteMQTT.connected()) { //verifica se o cliente ainda esta conectado ao broker MQTT
    mqttReconnect(); //se nao estiver conectado, tenta reconectar
  }

  clienteMQTT.loop(); //mantem a conexao MQTT ativa e processa mensagens recebidas

  atualizarBaterias(); //atualiza a simulacao da bateria das bancadas

  unsigned long agora = millis(); //obtem o tempo atual desde que o ESP32 foi ligado

  if (agora - ultimaPublicacaoMs >= INTERVALO_PUBLICACAO_MS) { //verifica se ja chegou o momento de publicar novamente
    publicarTelemetria(); //publica presenca, corrente e bateria no broker MQTT
    ultimaPublicacaoMs = agora; //atualiza o instante da ultima publicacao
  }
}