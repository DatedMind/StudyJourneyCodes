/*Usando Arduino Uno*/

#include <SPI.h>
#include <MFRC522.h>         // Biblioteca RFID
#include <SoftwareSerial.h>  // Biblioteca HC-05
#include <U8g2lib.h>         // Esta é a biblioteca de drivers para o displayI2C SH1106
#include <Wire.h>            // Biblioteca de comunicação do  displayI2C SH1106

#define SDA_SELECT 10
#define RST 9
#define BUTTON_RST 6
#define LED 5

MFRC522 rfid_select(SDA_SELECT, RST);

SoftwareSerial BT_HC_05(8, 7);

U8X8_SH1106_128X64_NONAME_HW_I2C displayI2C(U8X8_PIN_NONE);  // Criação do objeto da biblioteca U8g2lib.h para a utilização de suas intruções

// Adicionando a struct Product e suas respectivas variáveis

struct Product {
  float price;
  const char *name;
  const char *id_product;
};

unsigned long previusTime = 0;
unsigned long intervalTime = 2000;

int totalProducts = 0;
float totalCost = 0;

int const qnt = 3;
Product product[qnt];

void setup() {

  Serial.begin(9600);

  // Inicializando o bluetooth

  BT_HC_05.begin(38400);

  // Inicializando o RFID

  SPI.begin();
  rfid_select.PCD_Init();

  //Inicialização do displayI2C

  displayI2C.begin();                               // Inicia o displayI2C
  displayI2C.setPowerSave(0);                       // tira o display do modo economy
  displayI2C.setFont(u8x8_font_chroma48medium8_r);  // seta a fonte que será utilizada na saída do display
  displayI2C.clear;                                 // Limpa as mensagens do display
  displayI2C.print("Successful");                   // Mensagem de sucesso de inicialização

  //Inicialização dos produtos com seus valores finais

  product[0] = { 21.40, "CARROT TRAY - 0.99lb", "A3.F1.C9.B6" };
  product[1] = { 4.05, "POTATO PACKET - 2.2lb", "4D.8E.F1.E4" };
  product[2] = { 4.00, "BEETROOT PACKET - 2.2lb", "E9.4B.3D.C1" };

  // inicializando botão de reset e led para tirar a prova real

  pinMode(BUTTON_RST, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}


void loop() {

  // Criando intervalo de tempo para não passar várias vezes a mesma tag sem querer

  unsigned long currentTime = millis();

  if (currentTime - previusTime >= intervalTime) previusTime = currentTime;

  if (!rfid_select.PICC_IsNewCardPresent() || !rfid_select.PICC_ReadCardSerial()) return;

  // Validação para a saída do product no displayI2C SH1106

  // Adicionando uma id_key para ter em mão o codHEX das tags para futuras validações

  String id_key = "";

  // Transformando codigo byte em HEX. Ex: 0xA3, 0xF1, 0xC9, 0xB6 -> A3.F1.C9.B6

  for (byte i = 0; i < rfid_select.uid.size; i++) {
    if (rfid_select.uid.uidByte[i] < 0x10) id_key += "0";
    id_key += String(rfid_select.uid.uidByte[i], HEX);
    if (i < rfid_select.uid.size - 1) id_key += ".";
  }

  id_key.toUpperCase();

  rfid_select.PICC_HaltA();

  // Validando e executando um código de saída do produto[i] no displayI2C. Lógica para tirar o maximo de proveito e dinamização

  for (int i = 0; i < qnt; i++)

    if (id_key == product[i].id_product) {
      ShowOnDisplay(&product[i]);
      totalProducts++;
      totalCost += product[i].price;
      digitalWrite(LED, HIGH);
      delay(200);
      digitalWrite(LED, LOW);
    }

  // Botão de reset

  if (digitalRead(BUTTON_RST) == LOW) {
    SendToConnectedDevice();
    ResetSession();
    delay(300);
  }
}

// Criando funções para dinamização do código

// Função para mostrar apenas o layout no displayI2C

void ShowOnDisplay(Product *p) {

  displayI2C.clear();

  // Cabeçalho

  displayI2C.setCursor(0, 0);
  displayI2C.print("-|LacostineLJ|-");

  // if, se tiver produto, mostrar

  if (p != nullptr) {
    displayI2C.setCursor(0, 1);
    displayI2C.print(p->name);
    displayI2C.setCursor(0, 2);
    displayI2C.print(p->price);
    displayI2C.setCursor(0, 3);
    displayI2C.print(p->id_product);
  }

  // Info de total de produtos e custo geral

  displayI2C.setCursor(0, 4);
  displayI2C.print("Products: ");
  displayI2C.print(totalProducts);
  displayI2C.setCursor(0, 5);
  displayI2C.print("Total Cost: ");
  displayI2C.print(totalCost);

  // Rodapé

  displayI2C.setCursor(0, 6);
  displayI2C.print("-|LacostineLJ|-");
}

// Função para enviar uma nota fiscal para o dispositivo conectado pelo bluetooth

void SendToConnectedDevice() {

  // Cabeçalho

  BT_HC_05.println(F("-|Fiscal_Note|-"));

  // Info de total de produtos e custo geral

  BT_HC_05.println(F("Products: "));
  BT_HC_05.println(totalProducts);
  BT_HC_05.println(F("Total Cost: "));
  BT_HC_05.println(totalCost);

  // Rodapé

  BT_HC_05.println(F("-|LacostineLJ|-"));
}

// Função para resetar variáveis e o displayI2C

void ResetSession() {

  totalProducts = 0;
  totalCost = 0;

  displayI2C.setCursor(8, 4);
  displayI2C.print("-|LacostineLJ|-");
}