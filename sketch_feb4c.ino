/* Usando Arduino Uno */
/* Using Arduino Uno */
 
#include <SPI.h>
#include <MFRC522.h>         // Biblioteca RFID / RFID library
#include <SoftwareSerial.h>  // Biblioteca para comunicação Bluetooth HC-05 / HC-05 Bluetooth communication library
#include <U8g2lib.h>         // Biblioteca de drivers para o display OLED SH1106 / Driver library for SH1106 OLED display
#include <Wire.h>            // Biblioteca de comunicação I2C / I2C communication library

// Definição dos pinos utilizados
// Pin definitions

#define SDA_SELECT 10
#define RST 9
#define BUTTON_RST 6
#define LED 5

// Criação do objeto RFID
// RFID object creation

MFRC522 rfid_select(SDA_SELECT, RST);

// Criação do objeto Bluetooth (RX, TX)
// Bluetooth object creation (RX, TX)

SoftwareSerial BT_HC_05(8, 7);

// Criação do objeto do display OLED usando U8X8
// OLED display object creation using U8X8

U8X8_SH1106_128X64_NONAME_HW_I2C displayI2C(U8X8_PIN_NONE);

// Estrutura que representa um produto
// Structure that represents a product

struct Product {
  float price;             // Preço do produto / Product price
  const char *name;        // Nome do produto / Product name
  const char *id_product;  // ID da tag RFID em HEX / RFID tag ID in HEX
};

// Variáveis de controle de tempo
// Time control variables

unsigned long previusTime = 0;
unsigned long intervalTime = 2000;  // Intervalo para evitar múltiplas leituras da mesma tag
                                    // Interval to avoid multiple readings of the same tag

// Variáveis de controle da compra
// Purchase control variables

int totalProducts = 0;
float totalCost = 0;

// Quantidade total de produtos cadastrados
// Total number of registered products

int const qnt = 3;
Product product[qnt];

void setup() {

  // Inicialização da comunicação serial
  // Serial communication initialization

  Serial.begin(9600);

  // Inicialização do módulo Bluetooth
  // Bluetooth module initialization

  BT_HC_05.begin(38400);

  // Inicialização do barramento SPI e do leitor RFID
  // SPI bus and RFID reader initialization

  SPI.begin();
  rfid_select.PCD_Init();

  // Inicialização do display OLED
  // OLED display initialization

  displayI2C.begin();                               // Inicia o display / Starts the display
  displayI2C.setPowerSave(0);                       // Desativa modo economia / Disables power save mode
  displayI2C.setFont(u8x8_font_chroma48medium8_r);  // Define a fonte / Sets the font
  displayI2C.clear();                               // Limpa o display / Clears the display
  displayI2C.print("Successful");                   // Mensagem de inicialização / Initialization message

  // Inicialização dos produtos cadastrados
  // Registered products initialization

  product[0] = { 21.40, "CARROT TRAY - 0.99lb", "A3.F1.C9.B6" };
  product[1] = { 4.05,  "POTATO PACKET - 2.2lb", "4D.8E.F1.E4" };
  product[2] = { 4.00,  "BEETROOT PACKET - 2.2lb", "E9.4B.3D.C1" };

  // Inicialização do botão de reset e LED indicador
  // Reset button and indicator LED initialization

  pinMode(BUTTON_RST, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void loop() {

  // Controle de tempo para evitar leituras repetidas
  // Time control to avoid repeated readings

  unsigned long currentTime = millis();

  if (currentTime - previusTime >= intervalTime)
    previusTime = currentTime;

  // Verifica se há uma nova tag RFID presente
  // Checks if a new RFID tag is present

  if (!rfid_select.PICC_IsNewCardPresent() || !rfid_select.PICC_ReadCardSerial())
    return;

  // String para armazenar o UID da tag em formato HEX
  // String to store the tag UID in HEX format

  String id_key = "";

  // Conversão do UID byte a byte para HEX (ex: A3.F1.C9.B6)
  // Byte-by-byte UID conversion to HEX format

  for (byte i = 0; i < rfid_select.uid.size; i++) {
    if (rfid_select.uid.uidByte[i] < 0x10) id_key += "0";
    id_key += String(rfid_select.uid.uidByte[i], HEX);
    if (i < rfid_select.uid.size - 1) id_key += ".";
  }

  id_key.toUpperCase();

  // Finaliza comunicação com a tag atual
  // Ends communication with the current tag

  rfid_select.PICC_HaltA();

  // Validação do produto lido e exibição no display
  // Product validation and display output

  for (int i = 0; i < qnt; i++) {
    if (id_key == product[i].id_product) {
      ShowOnDisplay(&product[i]);
      totalProducts++;
      totalCost += product[i].price;

      // Pisca o LED para indicar leitura válida
      // LED blink to indicate valid reading

      digitalWrite(LED, HIGH);
      delay(200);
      digitalWrite(LED, LOW);
    }
  }

  // Botão para finalizar compra e enviar nota fiscal
  // Button to finalize purchase and send receipt

  if (digitalRead(BUTTON_RST) == LOW) {
    SendToConnectedDevice();
    ResetSession();
    delay(300);  // Debounce simples / Simple debounce
  }
}

// Função para exibir informações no display
// Function to display information on the OLED

void ShowOnDisplay(Product *p) {

  displayI2C.clear();

  // Cabeçalho
  // Header

  displayI2C.setCursor(0, 0);
  displayI2C.print("-|LacostineLJ|-");

  // Exibe informações do produto se existir
  // Displays product information if available

  if (p != nullptr) {
    displayI2C.setCursor(0, 1);
    displayI2C.print(p->name);
    displayI2C.setCursor(0, 2);
    displayI2C.print(p->price);
    displayI2C.setCursor(0, 3);
    displayI2C.print(p->id_product);
  }

  // Exibe total de produtos e custo
  // Displays total products and total cost

  displayI2C.setCursor(0, 4);
  displayI2C.print("Products: ");
  displayI2C.print(totalProducts);
  displayI2C.setCursor(0, 5);
  displayI2C.print("Total Cost: ");
  displayI2C.print(totalCost);

  // Rodapé
  // Footer

  displayI2C.setCursor(0, 6);
  displayI2C.print("-|LacostineLJ|-");
}

// Função para enviar nota fiscal via Bluetooth
// Function to send receipt via Bluetooth

void SendToConnectedDevice() {

  BT_HC_05.println(F("-|Fiscal_Note|-"));
  BT_HC_05.println(F("Products: "));
  BT_HC_05.println(totalProducts);
  BT_HC_05.println(F("Total Cost: "));
  BT_HC_05.println(totalCost);
  BT_HC_05.println(F("-|LacostineLJ|-"));
}

// Função para resetar a sessão de compra
// Function to reset the purchase session

void ResetSession() {

  totalProducts = 0;
  totalCost = 0;

  displayI2C.clear();
  displayI2C.setCursor(0, 2);
  displayI2C.print("Session Reset");
}
