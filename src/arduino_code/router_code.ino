#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define SOIL_MOISTURE_PIN A0
#define TEMP_SENSOR_PIN A1
#define PH_SENSOR_PIN A2
#define HUMIDITY_SENSOR_PIN A3

// Define RF24 network pins and configuration
#define RADIO_CE_PIN 7
#define RADIO_CSN_PIN 8
#define MAX_RETRIES 25
#define RETRY_DELAY 2000  // 2 seconds delay

RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);  // nRF24L01(+) radio
RF24Network network(radio);  // Network uses that radio

const uint16_t THIS_NODE = 01;  // Address of our node in Octal format
const uint16_t RECEIVER_NODE = 00;  // Address of the receiver node

struct packet_t {  
  char type[5];     // Packet type
  char data1[10];   // Sensor data 1 (soil moisture)
  char data2[10];   // Sensor data 2 (temperature)
  char data3[10];   // Sensor data 3 (PH)
  char data4[10];   // Sensor data 4 (humidity)
};

struct sleep_time_t {
  unsigned long sleepDuration;  // Sleep duration in milliseconds
};

volatile bool library_watchdogTriggered = false;  
ISR(WDT_vect) {
  library_watchdogTriggered = true;
}

// Function to convert LM35 output voltage to temperature in Celsius
float readTemperature() {
  int analogValue = analogRead(TEMP_SENSOR_PIN);
  float voltage = analogValue * (5.0 / 1023.0);  // Convert analog value to voltage
  return voltage * 100.0;  // Convert voltage to temperature (LM35: 10mV per °C)
}

// Collect sensor data based on the new packet structure
void collectSensorData(packet_t &packet) {
  int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  float temperature = readTemperature();
  int phValue = analogRead(PH_SENSOR_PIN);
  int humidity = analogRead(HUMIDITY_SENSOR_PIN);

  snprintf(packet.data1, sizeof(packet.data1), "%d", soilMoisture);
  snprintf(packet.data2, sizeof(packet.data2), "%.2f", temperature);
  snprintf(packet.data3, sizeof(packet.data3), "%d", phValue);
  snprintf(packet.data4, sizeof(packet.data4), "%d", humidity);
}

void setup(void) {
  pinMode(2, OUTPUT); // Pin to power sensors
  digitalWrite(2, HIGH);
  Serial.begin(9600);

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!"));
    while (1);
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_1MBPS);
  radio.setRetries(15, 15);
  radio.setChannel(90);
  network.begin(THIS_NODE);
}

void loop(void) {
  packet_t packet;
  strcpy(packet.type, "data");  // Set packet type to "data"

  // Collect sensor data
  collectSensorData(packet);

  // Send data and wait for response
  while (!sendData(packet)) {
    Serial.println(F("Retrying..."));
  }
}

bool sendData(packet_t &packet) {
  RF24NetworkHeader header(RECEIVER_NODE);
  bool success = false;

  for (int i = 0; i < MAX_RETRIES; i++) {
    if (!success) {
      success = network.write(header, &packet, sizeof(packet));  // Attempt to send data
    }
    
    if (success) {
      Serial.println(F("Data Sent"));
      return waitForResponse();
    } else {
      delay(RETRY_DELAY);  // Wait before retrying
    }
  }

  Serial.println(F("Failed to send data after maximum retries."));
  return false;
}

bool waitForResponse() {
  unsigned long startWaiting = millis();
  while (millis() - startWaiting < RETRY_DELAY) {
    network.update();
    if (network.available()) {
      RF24NetworkHeader header;
      sleep_time_t sleepTime;
      network.read(header, &sleepTime, sizeof(sleepTime));
      goToSleep(sleepTime.sleepDuration);
      return true;
    }
  }
  return false;
}

void goToSleep(unsigned long sleepDuration) {
  digitalWrite(2, LOW);                   //Turen off power to sensors when going to sleep
  Serial.println(F("Going to sleep..."));

  radio.stopListening();
  radio.powerDown();
  ADCSRA &= ~(1 << ADEN);  // Disable ADC

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  unsigned long remainingSleepMillis = sleepDuration;
  while (remainingSleepMillis > 0) {
    byte wdtTimeout;
    if (remainingSleepMillis >= 8000) {
      wdtTimeout = WDTO_8S;
      remainingSleepMillis -= 8000;
    } else {
      delay(remainingSleepMillis);
      remainingSleepMillis = 0;
    }

    wdt_enable(wdtTimeout);
    WDTCSR |= (1 << WDIE);
    library_watchdogTriggered = false;
    sleep_mode();
    while (!library_watchdogTriggered);
    wdt_disable();
  }

  Serial.println(F("Woke up!"));
  sleep_disable();
  ADCSRA |= (1 << ADEN);  // Re-enable ADC

  radio.startListening();
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_1MBPS);
  radio.setRetries(15, 15);
  radio.setChannel(90);
  network.begin(THIS_NODE);
  digitalWrite(2, HIGH);
}
