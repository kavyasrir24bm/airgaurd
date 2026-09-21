#include <DHT.h>
#include <HardwareSerial.h>

// =====================================================
// PIN DEFINITIONS
// =====================================================

#define DHT_PIN 18
#define DHT_TYPE DHT22

#define RELAY_PIN 23
#define BUZZER_PIN 19

// PMS5003 UART
#define PMS_RX 16
#define PMS_TX 17

// MH-Z19B UART
#define CO2_RX 4
#define CO2_TX 5


// =====================================================
// SENSOR OBJECTS
// =====================================================

DHT dht(DHT_PIN, DHT_TYPE);

HardwareSerial PMS(2);
HardwareSerial CO2(1);


// =====================================================
// REAL SENSOR VARIABLES
// =====================================================

int pm1 = 0;
int pm25 = 0;
int pm10 = 0;

int co2ppm = 0;

float temperature = 0;
float humidity = 0;


// =====================================================
// ZONE STRUCTURE
// =====================================================

struct Zone {
  String name;
  float pm25;
  float co2;
  float temperature;
  float humidity;
  bool realZone;
};

Zone zones[5];


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // Start PMS5003
  PMS.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);

  // Start MH-Z19B
  CO2.begin(9600, SERIAL_8N1, CO2_RX, CO2_TX);

  // Start DHT22
  dht.begin();

  // Output pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initially OFF
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println();
  Serial.println("======================================");
  Serial.println("   MULTI-ZONE AIR QUALITY SYSTEM");
  Serial.println("======================================");
  Serial.println("Zone 1 = REAL SENSOR ZONE");
  Serial.println("Zone 2-5 = SIMULATED ZONES");
  Serial.println();
}


// =====================================================
// READ PMS5003
// =====================================================

bool readPMS5003() {

  if (PMS.available() < 32) {
    return false;
  }

  // Check frame header
  if (PMS.read() != 0x42) {
    return false;
  }

  if (PMS.read() != 0x4D) {
    return false;
  }

  uint8_t data[30];

  for (int i = 0; i < 30; i++) {

    unsigned long start = millis();

    while (!PMS.available()) {

      if (millis() - start > 100) {
        return false;
      }
    }

    data[i] = PMS.read();
  }

  // PM1.0
  pm1 = (data[8] << 8) | data[9];

  // PM2.5
  pm25 = (data[10] << 8) | data[11];

  // PM10
  pm10 = (data[12] << 8) | data[13];

  return true;
}


// =====================================================
// READ MH-Z19B CO2
// =====================================================

int readCO2() {

  byte command[9] = {
    0xFF,
    0x01,
    0x86,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x79
  };

  // Clear old data
  while (CO2.available()) {
    CO2.read();
  }

  // Send command
  CO2.write(command, 9);

  unsigned long start = millis();

  while (CO2.available() < 9) {

    if (millis() - start > 1000) {
      return -1;
    }

    delay(10);
  }

  byte response[9];

  for (int i = 0; i < 9; i++) {
    response[i] = CO2.read();
  }

  // Check response
  if (response[0] != 0xFF || response[1] != 0x86) {
    return -1;
  }

  int ppm = (response[2] << 8) | response[3];

  return ppm;
}


// =====================================================
// GENERATE SIMULATED ZONES
// =====================================================

void generateSimulatedZones() {

  // ---------------------------------------------------
  // ZONE 2 - HOSPITAL
  // ---------------------------------------------------

  zones[1].name = "Hospital";

  zones[1].pm25 = 45;
  zones[1].co2 = 900;

  zones[1].temperature = 28;
  zones[1].humidity = 60;

  zones[1].realZone = false;


  // ---------------------------------------------------
  // ZONE 3 - TRAFFIC JUNCTION
  // ---------------------------------------------------

  zones[2].name = "Traffic Junction";

  zones[2].pm25 = 85;
  zones[2].co2 = 1500;

  zones[2].temperature = 31;
  zones[2].humidity = 55;

  zones[2].realZone = false;


  // ---------------------------------------------------
  // ZONE 4 - INDUSTRIAL AREA
  // ---------------------------------------------------

  zones[3].name = "Industrial Area";

  zones[3].pm25 = 70;
  zones[3].co2 = 1200;

  zones[3].temperature = 32;
  zones[3].humidity = 50;

  zones[3].realZone = false;


  // ---------------------------------------------------
  // ZONE 5 - RESIDENTIAL AREA
  // ---------------------------------------------------

  zones[4].name = "Residential Area";

  zones[4].pm25 = 30;
  zones[4].co2 = 700;

  zones[4].temperature = 29;
  zones[4].humidity = 65;

  zones[4].realZone = false;
}


// =====================================================
// ANALYZE EACH ZONE
// =====================================================

void analyzeZone(int i) {

  Serial.println("--------------------------------------");

  Serial.print("ZONE : ");
  Serial.println(zones[i].name);

  Serial.print("PM2.5 : ");
  Serial.print(zones[i].pm25);
  Serial.println(" ug/m3");

  Serial.print("CO2   : ");
  Serial.print(zones[i].co2);
  Serial.println(" ppm");


  // ===================================================
  // CRITICAL
  // ===================================================

  if (zones[i].pm25 >= 100 || zones[i].co2 >= 2000) {

    Serial.println("STATUS : CRITICAL");

    Serial.println("ACTION : IMMEDIATE ALERT");

  }


  // ===================================================
  // HIGH
  // ===================================================

  else if (zones[i].pm25 >= 75 || zones[i].co2 >= 1200) {

    Serial.println("STATUS : HIGH");

    Serial.println("ACTION : VENTILATE");

  }


  // ===================================================
  // MODERATE
  // ===================================================

  else if (zones[i].pm25 >= 50 || zones[i].co2 >= 1000) {

    Serial.println("STATUS : MODERATE");

    Serial.println("ACTION : MONITOR");

  }


  // ===================================================
  // NORMAL
  // ===================================================

  else {

    Serial.println("STATUS : NORMAL");

    Serial.println("ACTION : NONE");
  }


  // Real or simulated
  if (zones[i].realZone) {

    Serial.println("DATA SOURCE : REAL SENSOR");

  } else {

    Serial.println("DATA SOURCE : SIMULATED");
  }
}


// =====================================================
// COMPARE ALL 5 ZONES
// =====================================================

void compareZones() {

  Serial.println();
  Serial.println();
  Serial.println("======================================");
  Serial.println("       5 ZONE AIR QUALITY STATUS");
  Serial.println("======================================");

  for (int i = 0; i < 5; i++) {

    analyzeZone(i);
  }

  Serial.println("--------------------------------------");
}


// =====================================================
// CONTROL REAL HARDWARE
// =====================================================

void controlHardware() {

  // Only REAL ZONE controls physical hardware

  if (zones[0].pm25 >= 100 ||
      zones[0].co2 >= 2000) {

    // CRITICAL

    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);

    Serial.println();
    Serial.println("!!! CRITICAL REAL ZONE !!!");
    Serial.println("FAN       : ON");
    Serial.println("BUZZER    : ON");
  }


  else if (zones[0].pm25 >= 75 ||
           zones[0].co2 >= 1200) {

    // HIGH

    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);

    Serial.println();
    Serial.println("HIGH POLLUTION - REAL ZONE");
    Serial.println("FAN       : ON");
    Serial.println("BUZZER    : ON");
  }


  else {

    // NORMAL

    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);

    Serial.println();
    Serial.println("REAL ZONE NORMAL");
    Serial.println("FAN       : OFF");
    Serial.println("BUZZER    : OFF");
  }
}


// =====================================================
// DISPLAY REAL SENSOR DATA
// =====================================================

void displayRealZone() {

  Serial.println();
  Serial.println("======================================");
  Serial.println("           REAL ZONE 1");
  Serial.println("======================================");

  Serial.print("PM1.0       : ");
  Serial.print(pm1);
  Serial.println(" ug/m3");

  Serial.print("PM2.5       : ");
  Serial.print(pm25);
  Serial.println(" ug/m3");

  Serial.print("PM10        : ");
  Serial.print(pm10);
  Serial.println(" ug/m3");

  Serial.print("CO2         : ");
  Serial.print(co2ppm);
  Serial.println(" ppm");

  if (!isnan(temperature)) {

    Serial.print("Temperature : ");
    Serial.print(temperature);
    Serial.println(" C");

  } else {

    Serial.println("Temperature : Sensor Error");
  }

  if (!isnan(humidity)) {

    Serial.print("Humidity    : ");
    Serial.print(humidity);
    Serial.println(" %");

  } else {

    Serial.println("Humidity    : Sensor Error");
  }
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // ===================================================
  // STEP 1: READ REAL SENSORS
  // ===================================================

  bool pmsOK = readPMS5003();

  int newCO2 = readCO2();

  if (newCO2 != -1) {

    co2ppm = newCO2;
  }

  temperature = dht.readTemperature();

  humidity = dht.readHumidity();


  // ===================================================
  // STEP 2: DISPLAY REAL SENSOR DATA
  // ===================================================

  displayRealZone();


  // ===================================================
  // STEP 3: STORE REAL ZONE DATA
  // ===================================================

  zones[0].name = "School";

  zones[0].pm25 = pm25;

  zones[0].co2 = co2ppm;

  zones[0].temperature = temperature;

  zones[0].humidity = humidity;

  zones[0].realZone = true;


  // ===================================================
  // STEP 4: GENERATE SIMULATED ZONES
  // ===================================================

  generateSimulatedZones();


  // ===================================================
  // STEP 5: ANALYZE ALL 5 ZONES
  // ===================================================

  compareZones();


  // ===================================================
  // STEP 6: CONTROL REAL HARDWARE
  // ===================================================

  controlHardware();


  Serial.println();
  Serial.println("Next reading in 5 seconds...");
  Serial.println();


  delay(5000);
}