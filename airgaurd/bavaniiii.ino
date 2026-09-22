
// Bavani ESP32 Test Code
// AirGuard - Arduino Test

#define LED_PIN 2

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);

  Serial.println("Bavani ESP32 code started");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("AirGuard: System ON");
  delay(1000);

  digitalWrite(LED_PIN, LOW);
  Serial.println("AirGuard: System OFF");
  delay(1000);
}
