#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESPping.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

SoftwareSerial camera(-1, 2); // RX, TX (nodemcu) -> UOR(ESP32 CAM)
// (RX tidak dipakai) D0

#define SDA_PIN 4
#define SCL_PIN 5
#define RST_PIN 16
#define SS_PIN 0
#define BUZZER 15

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Sesuaikan alamat I2C jika diperlukan

const char* ssid = "Incidal";    // Your Wifi SSID
const char* password = "casio1314";   // Wifi Password
String server_addr = "192.168.1.5";  // your server address or computer IP

unsigned long lastScanTime = 0;
bool isCardPresent = false;

byte readCard[4];
String UIDCard;

void setup() {
  Serial.begin(115200);
  camera.begin(9600);

  Wire.begin(SDA_PIN, SCL_PIN);

  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(BUZZER, OUTPUT);
  analogWrite(BUZZER, LOW);

  SPI.begin();
  mfrc522.PCD_Init();
  ConnectWIFI();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  displayWelcomeMessage();
  getID();
}

bool readRFID() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    return true;
  } 
    return false;
}

uint8_t getID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }
  UIDCard = "";
  for (uint8_t i = 0; i < mfrc522.uid.size; i++) {
    UIDCard += String(mfrc522.uid.uidByte[i], HEX);
  }
  UIDCard.toUpperCase();
  analogWrite(BUZZER, HIGH); delay(300); analogWrite(BUZZER, LOW);
  clearDisplay();

  if (isCardRegistered()) {
    displayThanksMessage();
    camera.print("Sending command to take picture...");
    Serial.print(UIDCard);
    camera.print("#");
    Serial.println("Kirim UID RFID ke ESP32 CAM");

  } else {
    displayFailedMessage();
  }

  storeData();

  delay(1000);
  clearDisplay();
  mfrc522.PICC_HaltA();
  return 1;

}

bool isCardRegistered() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  String url = "http://" + server_addr + "/absensi/webapi/api/check.php?uid=" + UIDCard;

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(3000);

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();

    const size_t capacity = JSON_OBJECT_SIZE(1) + 50;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, payload);

    String status_res = doc["status"];

    if (status_res == "success") {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }

  http.end();
}

void storeData() {
  ConnectWIFI();
  WiFiClient client;
  String address, massage, first_name;

  address = "http://" + server_addr + "/absensi/webapi/api/create.php?uid=" + UIDCard;

  HTTPClient http;
  http.begin(client, address);
  int httpCode = http.GET();
  String payload;
  Serial.print("Response: ");
  if (httpCode > 0) {
    payload = http.getString();
    payload.trim();
    if (payload.length() > 0) {
      Serial.println(payload + "\n");
    }
  }
  http.end();

  const size_t capacity = JSON_OBJECT_SIZE(4) + 70;
  DynamicJsonDocument doc(capacity);

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  int id_res = doc["id"];
  const char* waktu_res = doc["waktu"];
  String nama_res = doc["nama"];
  const char* uid_res = doc["uid"];
  String status_res = doc["status"];

  for (int i = 0; i < nama_res.length(); i++) {
    if (nama_res.charAt(i) == ' ') {
      first_name = nama_res.substring(0, i);
      break;
    }
  }
}

void ConnectWIFI() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    delay(2000);
  } else {
    Serial.print("Connected to WiFi");
    IPAddress serverIP(192, 168, 1, 6);
  }
}

void displayWelcomeMessage() {
  lcd.setCursor(3, 0);
  lcd.print("TEMPELKAN");
  lcd.setCursor(5, 1);
  lcd.print("KARTU");
}

void displayThanksMessage() {
  lcd.setCursor(5, 0);
  lcd.print("ABSEN");
  lcd.setCursor(4, 1);
  lcd.print("BERHASIL");
}

void displayFailedMessage() {
  lcd.setCursor(2, 0);
  lcd.print("KARTU TIDAK");
  lcd.setCursor(4, 1);
  lcd.print("DIKENAL");
}

void clearDisplay() {
  lcd.clear();
}