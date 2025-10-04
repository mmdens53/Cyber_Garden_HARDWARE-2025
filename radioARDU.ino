#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(9600);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  mySwitch.enableTransmit(10);
  mySwitch.setProtocol(1);
  mySwitch.setPulseLength(350);
  Serial.println("Starting...");
}

void loop() {
  if (digitalRead(8) == LOW) {
    mySwitch.send(1, 24);
    Serial.println("Sent: ON");
  }

  if (digitalRead(9) == LOW) {
    mySwitch.send(2, 24);
    Serial.println("Sent: OFF");
  }
  if (digitalRead(11) == LOW) {
    mySwitch.send(3, 24);
    Serial.println("Sent: ON");
  }
  if (digitalRead(12) == LOW) {
    mySwitch.send(4, 24);
    Serial.println("Sent: OFF");
  }
}