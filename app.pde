import processing.net.*; 
import java.util.regex.*;

Client myClient;
ArrayList<String> logs = new ArrayList<String>();
String tempInfo = "Температура: нет данных";
String humInfo = "Влажность: нет данных";
boolean connected = false;
PFont font;
int maxLogs = 20;  // Максимум строк в логе

// Переменные для выбора цвета RGB
int r = 128, g = 128, b = 128;
int draggingSlider = -1; // -1: none, 0: R, 1: G, 2: B
int sliderX = 20;
int sliderY = 120;
int sliderWidth = 200;
int sliderHeight = 20;
int knobSize = 10;

void setup() {
  size(700, 900);
  font = createFont("Arial", 16);
  textFont(font);
  myClient = new Client(this, "172.20.10.2", 5000);
  logs.add("Подключение к мастеру...");
}

void draw() {
  background(240);
  
  // Статус подключения
  fill(0);
  text("Статус: " + (connected ? "✅ Подключено" : "❌ Нет соединения"), 20, 25);
  
  // Кнопки (более удобные, с отступами и лучшим стилем)
  drawButton(20, 50, 120, 50, "LED ON", color(200, 255, 200));
  drawButton(160, 50, 120, 50, "LED OFF", color(255, 200, 200));
  drawButton(300, 50, 120, 50, "Реле ВКЛ", color(200, 255, 200));
  drawButton(440, 50, 120, 50, "Реле ВЫКЛ", color(255, 200, 200));
  
  // Слайдеры для RGB
  drawSlider(sliderX, sliderY, r, "R: " + r, color(255, 0, 0));
  drawSlider(sliderX, sliderY + 40, g, "G: " + g, color(0, 255, 0));
  drawSlider(sliderX, sliderY + 80, b, "B: " + b, color(0, 0, 255));
  
  // Кнопка отправки цвета
  drawButton(sliderX + sliderWidth + 70, sliderY + 30, 120, 40, "Отправить цвет", color(200, 200, 255));
  
  // Превью цвета
  fill(r, g, b);
  rect(sliderX + sliderWidth + 70, sliderY + 80, 50, 30);
  fill(0);
  text("Превью", sliderX + sliderWidth + 130, sliderY + 95);
  
  // Информационное окно (температура и влажность) - сдвинуто вниз
  fill(200, 220, 255);
  rect(20, 240, 460, 80);
  fill(0);
  text(tempInfo, 30, 265);
  text(humInfo, 30, 290);
  
  // Логовое окно - сдвинуто вниз
  fill(230, 230, 230);
  rect(20, 340, 460, 350);
  fill(0);
  text("Лог:", 25, 360);
  
  // Отображение лога (последние maxLogs строк, с прокруткой если нужно)
  int yPos = 380;
  for (int i = max(0, logs.size() - maxLogs); i < logs.size(); i++) {
    if (yPos > 670) break;  // Ограничение высоты
    text(logs.get(i), 25, yPos);
    yPos += 18;
  }
}

void drawButton(int x, int y, int w, int h, String label, color col) {
  fill(col);
  stroke(0);
  rect(x, y, w, h, 10);  // Закругленные углы
  fill(0);
  textAlign(CENTER, CENTER);
  text(label, x + w/2, y + h/2);
  textAlign(LEFT, BASELINE);
}

void drawSlider(int x, int y, int val, String label, color barColor) {
  // Фон слайдера
  fill(200);
  stroke(0);
  rect(x, y, sliderWidth, sliderHeight, 5);
  
  // Полоска цвета
  fill(barColor);
  noStroke();
  rect(x, y, (val / 255.0) * sliderWidth, sliderHeight);
  
  // Ползунок
  stroke(0);
  fill(255);
  float knobX = x + (val / 255.0) * sliderWidth;
  rect(knobX - knobSize/2, y - knobSize/2, knobSize, sliderHeight + knobSize);
  
  // Лейбл
  fill(0);
  text(label, x + sliderWidth + 10, y + sliderHeight/2 + 5);
}

void mousePressed() {
  if (!connected) return;
  
  // Проверка кнопок LED и реле
  if (mouseX > 20 && mouseX < 140 && mouseY > 50 && mouseY < 100) {
    sendMessage("LED_ON");
  }
  if (mouseX > 160 && mouseX < 280 && mouseY > 50 && mouseY < 100) {
    sendMessage("LED_OFF");
  }
  if (mouseX > 300 && mouseX < 420 && mouseY > 50 && mouseY < 100) {
    sendMessage("Rele_ON");
  }
  if (mouseX > 440 && mouseX < 560 && mouseY > 50 && mouseY < 100) {
    sendMessage("Rele_OFF");
  }
  
  // Проверка кнопки отправки цвета
  int btnX = sliderX + sliderWidth + 20;
  int btnY = sliderY + 30;
  if (mouseX > btnX && mouseX < btnX + 120 && mouseY > btnY && mouseY < btnY + 40) {
    sendColorMessage();
  }
  
  // Проверка слайдеров
  checkSliderPress(0, sliderY);     // R
  checkSliderPress(1, sliderY + 40); // G
  checkSliderPress(2, sliderY + 80); // B
}

void mouseDragged() {
  if (draggingSlider >= 0) {
    updateSliderValue(draggingSlider);
  }
}

void mouseReleased() {
  draggingSlider = -1;
}

void checkSliderPress(int sliderIndex, int yPos) {
  if (mouseX >= sliderX && mouseX <= sliderX + sliderWidth &&
      mouseY >= yPos && mouseY <= yPos + sliderHeight) {
    draggingSlider = sliderIndex;
    updateSliderValue(sliderIndex);
  }
}

void updateSliderValue(int sliderIndex) {
  float normalized = constrain((mouseX - sliderX) / (float)sliderWidth, 0, 1);
  int newVal = (int)(normalized * 255);
  
  switch(sliderIndex) {
    case 0: r = newVal; break;
    case 1: g = newVal; break;
    case 2: b = newVal; break;
  }
}

void sendColorMessage() {
  String colorCmd = "COLOR: " + r + "," + g + "," + b;
  sendMessage(colorCmd);
}

void sendMessage(String msg) {
  if (myClient.active()) {
    myClient.write(msg + "\n");
    addLog("➡ Отправлено: " + msg);
  } else {
    addLog("❌ Ошибка: нет соединения для отправки");
  }
}

void addLog(String msg) {
  logs.add(msg);
  if (logs.size() > maxLogs) {
    logs.remove(0);  // Удаляем старые сообщения
  }
}

void clientEvent(Client c) {
  String input = c.readStringUntil('\n');
  if (input != null) {
    input = trim(input);
    if (input.length() > 0) {
      if (!connected) {
        connected = true;
        addLog("✅ Соединение установлено!");
      }
      addLog("⬅ Получено: " + input);
      
      // Парсинг данных датчика (на русском)
      parseSensorData(input);
    }
  }
}

void parseSensorData(String input) {
  // Извлекаем температуру с помощью regex
  Matcher tempMatcher = Pattern.compile("Температура: (\\d+\\.\\d+)°C").matcher(input);
  if (tempMatcher.find()) {
    tempInfo = "Температура: " + tempMatcher.group(1) + "°C";
  }
  
  // Извлекаем влажность
  Matcher humMatcher = Pattern.compile("Влажность: (\\d+\\.\\d+)").matcher(input);
  if (humMatcher.find()) {
    humInfo = "Влажность: " + humMatcher.group(1) + "%";
  }
}
