#include <RCSwitch.h>
#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- RF настройки ----------------
RCSwitch mySwitch = RCSwitch();
const int led = 21;
const int rele = 18;
const int pisklya = 22;
// ---------------- RGB LED настройки ----------------
const int gPin = 16;  // Пин для красного канала
const int rPin = 17;  // Пин для зеленого канала
const int bPin = 19;  // Пин для синего канала
const int ldrPin = 34;
// ---------------- WiFi настройки ----------------
const char* ssid = "IPhoneP";
const char* password = "11111111";

WiFiServer server(5000);
WiFiClient client;

unsigned long lastSend = 0;

// ====== Настройки порогов ======
const float TEMP_HIGH = 30.0;
const float HUM_LOW = 30.0;
const float HUM_HIGH = 70.0;

// ====== Статические флаги и таймеры ======
static unsigned long tempTimer = 0;
static bool tempWarning = false;
static bool buzzerActive = false;

static bool humLowWarning = false;
static bool humHighWarning = false;

// ------------------- Освещенность -------------------
static bool lightWarning = false;
const int LIGHT_LOW = 1500;  // чем меньше — тем темнее
const int LIGHT_HIGH = 2000;

// ---------------- DHT11 ----------------
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ---------------- LCD I2C ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Адрес 0x27, 16x2 LCD

void setup() {
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  pinMode(rele, OUTPUT);
  pinMode(pisklya, OUTPUT);

  // RGB пины
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);
  analogWrite(rPin, 0);
  analogWrite(gPin, 0);
  analogWrite(bPin, 0);

  // RF приёмник
  mySwitch.enableReceive(12);  // Пин 12 для DATA приёмника
  mySwitch.setProtocol(1);
  mySwitch.setPulseLength(350);

  // DHT
  dht.begin();

  // Настройка I2C на нестандартные пины
  Wire.begin(4, 5);  // SDA = GPIO4, SCL = GPIO5

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 init...");

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
  Serial.println("TCP server started on port 5000");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  Serial.println("All systems ready!");
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int lightValue = analogRead(ldrPin);
  // ------------------- Температура -------------------
  if (!tempWarning && temperature >= TEMP_HIGH) {
    tempWarning = true;
    buzzerActive = true;
    sendMessage("Слишком высокая температура!");
    lcd.clear();
    lcd.print("High temperature!");
    tone(pisklya, 1000);
    tempTimer = millis();
  }

  if (buzzerActive && millis() - tempTimer > 10000) {
    noTone(pisklya);
    buzzerActive = false;
  }

  if (tempWarning && temperature < TEMP_HIGH - 1) {
    tempWarning = false;
    sendMessage("Температура в норме");
    lcd.clear();
    lcd.print("Good temperature");
  }

  // ------------------- Влажность -------------------
  // Слишком низкая влажность → включить реле
  if (!humLowWarning && humidity < HUM_LOW) {
    humLowWarning = true;
    digitalWrite(rele, HIGH);  // включить реле
    sendMessage("Влажность слишком низкая! Включаю реле.");
    lcd.clear();
    lcd.print("Low humidity!");
  }

  // Если влажность вернулась в норму → выключить реле
  if (humLowWarning && humidity >= HUM_LOW + 5) {
    humLowWarning = false;
    digitalWrite(rele, LOW);
    sendMessage("Влажность восстановилась. Реле выключено.");
    lcd.clear();
    lcd.print("Humidity OK!");
  }

  // Слишком высокая влажность → выключить реле
  if (!humHighWarning && humidity > HUM_HIGH) {
    humHighWarning = true;
    digitalWrite(rele, LOW);
    sendMessage("Влажность слишком высокая! Выключаю реле.");
    lcd.clear();
    lcd.print("High humidity!");
  }

  // Если влажность вернулась в норму после высокой → можно снова включать реле
  if (humHighWarning && humidity <= HUM_HIGH - 5) {
    humHighWarning = false;
    sendMessage("Влажность снова в норме.");
    lcd.clear();
    lcd.print("Humidity OK!");
  }

  // Если стало темно → включаем LED
  if (!lightWarning && lightValue < LIGHT_LOW) {
    lightWarning = true;
    digitalWrite(led, HIGH);
    sendMessage("Темно! Включаю освещение.");
    lcd.clear();
    lcd.print("Light: ON");
  }

  // Если стало светло → выключаем LED
  if (lightWarning && lightValue > LIGHT_HIGH) {
    lightWarning = false;
    digitalWrite(led, LOW);
    sendMessage("Достаточно света. Выключаю LED.");
    lcd.clear();
    lcd.print("Light: OFF");
  }
  // ---------------- RF приём ----------------
  if (mySwitch.available()) {
    int value = mySwitch.getReceivedValue();

    if (value == 1) {
      analogWrite(rPin, 125);
      analogWrite(gPin, 125);
      analogWrite(bPin, 125);
      Serial.println("RF Received: ON");
      lcd.clear();
      lcd.print("RF Received: LED ON");
      if (client && client.connected()) client.println("RF: LED turned ON");
    } else if (value == 2) {
      analogWrite(rPin, 0);
      analogWrite(gPin, 0);
      analogWrite(bPin, 0);
      Serial.println("RF Received: OFF");
      lcd.clear();
      lcd.print("RF Received: LED OFF");
      if (client && client.connected()) client.println("RF: LED turned OFF");
    }
    if (value == 3) {
      digitalWrite(rele, HIGH);
      Serial.println("RF Received: ON");
      lcd.clear();
      lcd.print("RF Received: RELE ON");
      if (client && client.connected()) client.println("RF: Rele turned ON");
    } else if (value == 4) {
      digitalWrite(rele, LOW);
      Serial.println("RF Received: OFF");
      lcd.clear();
      lcd.print("RF Received: RELE OFF");
      if (client && client.connected()) client.println("RF: Rele turned OFF");
    }
    mySwitch.resetAvailable();
  }

  // ---------------- WiFi клиент ----------------
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      Serial.println("Client connected!");
      client.println("Welcome from ESP32 Master!");
    }
  } else {
    if (client.available()) {
      String msg = client.readStringUntil('\n');
      msg.trim();
      if (msg.length() > 0) {
        Serial.print("➡ From Client: ");
        Serial.println(msg);
        handleCommand(msg);
      }
    }
  }

  // ---------------- Периодическая отправка данных ----------------
  if (millis() - lastSend > 5000 * 3) {
    lastSend = millis();
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.clear();
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if (isnan(humidity) || isnan(temperature)) {
      sendMessage("Ошибка чтения с датчика DHT11!");
      lcd.setCursor(0, 0);
      lcd.print("DHT11 error   ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      return;
    }
    String message = "Влажность: " + String(humidity) + "%\nТемпература: " + String(temperature) + "°C";
    sendMessage(message);

    // Вывод на LCD
    lcd.setCursor(0, 0);
    lcd.print("T:" + String(temperature, 1) + "C     ");  // пробелы очищают старые данные
    lcd.setCursor(0, 1);
    lcd.print("H:" + String(humidity, 1) + "%     ");
  }
}

void handleCommand(String msg) {
  msg.trim();
  if (msg == "LED_ON") {
    analogWrite(rPin, 125);
    analogWrite(gPin, 125);
    analogWrite(bPin, 125);
    sendMessage("WiFi: LED turned ON");
    lcd.clear();
    lcd.print("WiFi: LED turned ON");
  } else if (msg == "LED_OFF") {
    analogWrite(rPin, 0);
    analogWrite(gPin, 0);
    analogWrite(bPin, 0);
    sendMessage("WiFi: LED turned OFF");
    lcd.clear();
    lcd.print("WiFi: LED turned OFF");
  } else if (msg == "Rele_ON") {
    digitalWrite(rele, HIGH);
    sendMessage("WiFi: Rele turned ON");
    lcd.clear();
    lcd.print("WiFi: Rele turned ON");
  } else if (msg == "Rele_OFF") {
    digitalWrite(rele, LOW);
    sendMessage("WiFi: Rele turned OFF");
    lcd.clear();
    lcd.print("WiFi: Rele turned OFF");
  } else if (msg.startsWith("COLOR:")) {
    // Парсинг команды COLOR: r,g,b
    String values = msg.substring(6);  // Убираем "COLOR:"
    int comma1 = values.indexOf(',');
    int comma2 = values.indexOf(',', comma1 + 1);
    if (comma1 > 0 && comma2 > comma1 + 1) {
      int r = values.substring(0, comma1).toInt();
      int g = values.substring(comma1 + 1, comma2).toInt();
      int b = values.substring(comma2 + 1).toInt();

      // Ограничиваем значения 0-255
      r = constrain(r, 0, 255);
      g = constrain(g, 0, 255);
      b = constrain(b, 0, 255);

      // Устанавливаем PWM значения для RGB
      analogWrite(rPin, r);
      analogWrite(gPin, g);
      analogWrite(bPin, b);

      String colorMsg = "WiFi: Color set to " + String(r) + "," + String(g) + "," + String(b);
      sendMessage(colorMsg);

      // Обновляем LCD (показываем кратко)
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("RGB: ");
      lcd.print(String(r));
      lcd.print(",");
      lcd.print(String(g));
      lcd.print(",");
      lcd.print(String(b));
    } else {
      sendMessage("ESP32: Invalid color format");
    }
  } else {
    sendMessage("ESP32: Unknown command");
  }
}

void sendMessage(String msg) {
  if (client && client.connected()) {
    client.println(msg);
  }
  Serial.println(msg);
}