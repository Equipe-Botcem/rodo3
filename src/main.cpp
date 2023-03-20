#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <SPI.h>

// Configuração de credenciais e informações
const char *ssid = "APTO_401";
const char *password = "naolembro";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;
String GOOGLE_SCRIPT_ID = "AKfycbzr8c1AU2bDjbV35bXLWaD0uSXaee99dTfYEu8vMTg3KzVSxCYKGFJd2Ga4ULprFQDz";

// Define pinos e objetos
#define SS_PIN 21
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Variáveis
String listaPresenca = "";

// Função para conectar ao Wi-Fi
void conectarWiFi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado a ");
  Serial.println(ssid);
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}


String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      //encodedString += code2;
    }
    yield();
  }
  return encodedString;
}


void enviarDadosPlanilha(String listaPresenca) {
  if (WiFi.status() == WL_CONNECTED) {
    // Remova a última vírgula da lista de presença
    listaPresenca.trim();
    if (listaPresenca.endsWith(",")) {
      listaPresenca.remove(listaPresenca.length() - 1);
    }
    
    String urlFinal = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec";
    Serial.print("Enviando dados para a planilha: ");
    Serial.println(urlFinal);

    HTTPClient http;
    http.begin(urlFinal);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postData = "idList=" + urlencode(listaPresenca);
    int httpCode = http.POST(postData);
    Serial.print("Código de Status HTTP: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Resposta da planilha: " + payload);
    }

    http.end();
  }
}




// Função de configuração
void setup() {
  Serial.begin(115200);

  SPI.begin();
  mfrc522.PCD_Init();

  conectarWiFi();
}

// Função principal do loop
void loop() {
  // Verifica se há um novo cartão presente e lê o cartão
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String id = "";
    // Obtém o ID do cartão
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      id += String(mfrc522.uid.uidByte[i], HEX);
    }

    id.toUpperCase();
    Serial.print("ID do cartão: ");
    Serial.println(id);

    // Adiciona o ID do cartão à lista de presença
    if (listaPresenca.indexOf(id) < 0) {
      listaPresenca += id + ",";
      Serial.println("ID adicionado à lista de presença.");
    } else {
      Serial.println("ID já está na lista de presença.");
    }
  }

  // Envia a lista de presença para a planilha do Google ao pressionar ENTER no Serial Monitor
  if (Serial.available() > 0) {
    if (Serial.read() == '\n') {
      Serial.println("Enviando a lista de presença para a planilha do Google...");
      enviarDadosPlanilha(listaPresenca);
      listaPresenca = ""; // Limpa a lista de presença
    }
  }

  delay(200);
}

