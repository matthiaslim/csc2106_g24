#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>

// ------- Bin Configuration -------
const float binHeight = 50.0;  // Height of bin in cm
const float sensorOffset = 2.0;  // Distance from sensor to top of bin in cm
const int numReadings = 5;       // Moving Average Filter
float readings[numReadings] = {0}; 
int readIndex = 0;
float total = 0;

// ------- Transmission Interval -------
const unsigned TX_INTERVAL = 60;  // Transmit every 60 seconds

// ------- LoRaWAN Credentials (OTAA) -------
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8] = { 0x65, 0xEB, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x6A, 0x63, 0x7C, 0x0A, 0x87, 0x2F, 0x37, 0x8C, 0xBB, 0x51, 0x37, 0x91, 0x2F, 0x27, 0x26, 0x87 };
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}

// ------- Pin Definitions -------
#define DHTPIN 5       // DHT22 Data Pin
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define trigPin 3      // HC-SR04 Trigger Pin
#define echoPin 4      // HC-SR04 Echo Pin

#define co2Pin A0      // MQ-2 Smoke Sensor Analog Output

// ------- LoRa Pin Mapping for RFM95 Shield -------
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 7,
    .dio = {2, 5, 6},
};

static osjob_t sendjob;

// ------- Function to Measure Distance (Bin Fill Level) -------
float measureDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    float duration = pulseIn(echoPin, HIGH);
    float distance = (duration / 2.0) / 29.1;
    
    return distance; 
}

// ------- LoRaWAN Send Function -------
void do_send(osjob_t* j) {
    // ---- Read Bin Fill Level ----
    float rawDistance = measureDistance();
    float adjustedDistance = rawDistance - sensorOffset;
    adjustedDistance = max(0, min(adjustedDistance, binHeight)); // Constrain
    uint8_t fillLevel = (uint8_t)(100.0 * (1.0 - (adjustedDistance / binHeight)));

    // ---- Read Temperature & Humidity (DHT22) ----
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature)) temperature = -99.99;
    if (isnan(humidity)) humidity = -99.99;

    int16_t tempScaled = (int16_t)(temperature * 100);
    int16_t humidScaled = (int16_t)(humidity * 100);

    // ---- Read Smoke/CO2 Level (MQ-2 Sensor) ----
    int co2Raw = analogRead(co2Pin);
    float voltage = co2Raw * (5.0 / 1023.0);
    uint16_t smokeLevel = (uint16_t)(voltage * 200); // Example scaling

    // ---- GPS Location (Fixed for now) ----
    float latitude = 1.370653;
    float longitude = 103.8268;
    int32_t lat_scaled = (int32_t)(latitude * 1000000);
    int32_t lon_scaled = (int32_t)(longitude * 1000000);

    // ---- Prepare Payload (15 Bytes) ----
    uint8_t payload[15];
    payload[0] = (tempScaled >> 8) & 0xFF;
    payload[1] = tempScaled & 0xFF;
    payload[2] = (humidScaled >> 8) & 0xFF;
    payload[3] = humidScaled & 0xFF;
    payload[4] = (smokeLevel >> 8) & 0xFF;
    payload[5] = smokeLevel & 0xFF;
    payload[6] = fillLevel;

    // Lat Payload (7 - 10)
    payload[7] = (lat_scaled >> 24) & 0xFF;
    payload[8] = (lat_scaled >> 16) & 0xFF;
    payload[9] = (lat_scaled >> 8) & 0xFF;
    payload[10] = lat_scaled & 0xFF;

    // Long Payload (11 - 14)
    payload[11] = (lon_scaled >> 24) & 0xFF;
    payload[12] = (lon_scaled >> 16) & 0xFF;
    payload[13] = (lon_scaled >> 8) & 0xFF;
    payload[14] = lon_scaled & 0xFF;

    Serial.print("Binary Payload: ");
    for (int i = 0; i < 15; i++) {
        Serial.print(payload[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    // ---- Send via LoRaWAN ----
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        LMIC_setTxData2(1, payload, sizeof(payload), 0);
        Serial.println(F("Packet queued"));
    }

    // Schedule next transmission
    os_setTimedCallback(j, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
}

// ------- Setup Function -------
void setup() {
    Serial.begin(9600);
    Serial.println(F("Starting Sensor Node"));

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    dht.begin();

    os_init();
    LMIC_reset();

    // First transmission
    do_send(&sendjob);
}

// ------- Loop Function (LoRa Processing) -------
void loop() {
    os_runloop_once();
}