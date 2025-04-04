#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>

// Enable this line for detailed status messages (comment out for production to save memory)
// #define DEBUG_MODE 1

// Status LED to show transmission states
#define STATUS_LED LED_BUILTIN

// ------- MQ2 Sensor Configuration -------
#define MQ_PIN                  (A0)   // MQ2 Analog Pin
#define RL_VALUE                (5)    // Load resistance on board, in kilo ohms
#define RO_CLEAN_AIR_FACTOR     (9.83) // RO_CLEAN_AIR_FACTOR=(Sensor resistance in clean air)/RO
                                                     
// MQ2 Calibration Parameters - Reduced for memory optimization
#define CALIBRATION_SAMPLE_TIMES     (25)   // Reduced from 50
#define CALIBRATION_SAMPLE_INTERVAL  (500)  // Time interval between samples (ms)
#define READ_SAMPLE_INTERVAL         (50)   // Time interval between samples in normal operation
#define READ_SAMPLE_TIMES            (5)    // Number of samples in normal operation

// Only keep smoke curve since that's what we're using
float SmokeCurve[3] = {2.3,0.53,-0.44};
float Ro = 10;  // Initial Ro value

// ------- Bin Configuration -------
const float binHeight = 50.0;     // Height of bin in cm
const float sensorOffset = 2.0;   // Distance from sensor to top of bin in cm

// ------- Pin Definitions -------
#define DHTPIN 9       // DHT22 Data Pin
#define DHTTYPE DHT22  // DHT 22 sensor type
DHT dht(DHTPIN, DHTTYPE);
// SimpleDHT22 dht(DHTPIN);

#define trigPin 3      // HC-SR04 Trigger Pin
#define echoPin 4      // HC-SR04 Echo Pin

// LoRaWAN Configuration
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui(u1_t* buf) { memcpy_P(buf, APPEUI, 8); }

static const u1_t PROGMEM DEVEUI[8] = { 0x65, 0xEB, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getDevEui(u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }

static const u1_t PROGMEM APPKEY[16] = { 0x6A, 0x63, 0x7C, 0x0A, 0x87, 0x2F, 0x37, 0x8C, 0xBB, 0x51, 0x37, 0x91, 0x2F, 0x27, 0x26, 0x87 };
void os_getDevKey(u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

static osjob_t sendjob;
const unsigned TX_INTERVAL = 60;  // Transmission interval in seconds

// Device states for status tracking
enum LoraState {
  STATE_INIT,
  STATE_JOINING,
  STATE_JOINED,
  STATE_TRANSMITTING,
  STATE_TX_COMPLETE,
  STATE_ERROR
};

volatile LoraState loraState = STATE_INIT;
uint32_t txCount = 0;

// Pin mapping for LoRa shield
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 7,
    .dio = {2, 5, 6},
};

// ------- MQ2 Sensor Functions (Optimized) -------
float MQResistanceCalculation(int raw_adc) {
  return ((float)RL_VALUE * (1023 - raw_adc) / raw_adc);
}

float MQCalibration(int mq_pin) {
  float val = 0;
  for (int i = 0; i < CALIBRATION_SAMPLE_TIMES; i++) {
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBRATION_SAMPLE_TIMES / RO_CLEAN_AIR_FACTOR;
  return val;
}

float MQRead(int mq_pin) {
  float rs = 0;
  for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  return rs / READ_SAMPLE_TIMES;
}

int MQGetSmokeReading() {
  float rs_ro_ratio = MQRead(MQ_PIN) / Ro;
  return (int)(pow(10, (((log(rs_ro_ratio) - SmokeCurve[1]) / SmokeCurve[2]) + SmokeCurve[0])));
}

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

// Show LoRa connection status with LED
void showLoraStatus() {
  switch(loraState) {
    case STATE_JOINING:
      // Fast blink during joining
      digitalWrite(STATUS_LED, (millis() % 200 < 100));
      break;
    case STATE_JOINED:
      // Solid on when connected
      digitalWrite(STATUS_LED, HIGH);
      break;
    case STATE_TRANSMITTING:
      // Quick pulse during transmission
      digitalWrite(STATUS_LED, (millis() % 100 < 50));
      break;
    case STATE_TX_COMPLETE:
      // Pulse once on successful transmission
      digitalWrite(STATUS_LED, (millis() % 2000 < 200));
      break;
    case STATE_ERROR:
      // SOS pattern for errors (3 short, 3 long, 3 short)
      {
        unsigned long ms = millis() % 2000;
        if (ms < 600) { // 3 short
          digitalWrite(STATUS_LED, (ms % 200 < 100));
        } else if (ms < 1200) { // 3 long
          digitalWrite(STATUS_LED, (ms % 200 < 150)); 
        } else if (ms < 1800) { // 3 short again
          digitalWrite(STATUS_LED, (ms % 200 < 100));
        } else {
          digitalWrite(STATUS_LED, LOW);
        }
      }
      break;
    default:
      // Off when initializing
      digitalWrite(STATUS_LED, LOW);
  }
}

// LoRaWAN event handling
void onEvent(ev_t ev) {
    #ifdef DEBUG_MODE
    Serial.print(F("[LoRa] Event at "));
    Serial.print(os_getTime());
    Serial.print(F(": "));
    #endif
    
    switch(ev) {
        case EV_JOINING:
            #ifdef DEBUG_MODE
            Serial.println(F("JOINING"));
            #endif
            loraState = STATE_JOINING;
            break;
            
        case EV_JOINED:
            #ifdef DEBUG_MODE
            Serial.println(F("JOINED"));
            // Print the network session keys for debugging
            Serial.print(F("DevAddr: "));
            Serial.println(LMIC.devaddr, HEX);
            #endif
            loraState = STATE_JOINED;
            LMIC_setLinkCheckMode(0);
            break;
            
        case EV_JOIN_FAILED:
            #ifdef DEBUG_MODE
            Serial.println(F("JOIN FAILED"));
            #endif
            loraState = STATE_ERROR;
            break;
            
        case EV_REJOIN_FAILED:
            #ifdef DEBUG_MODE
            Serial.println(F("REJOIN FAILED"));
            #endif
            loraState = STATE_ERROR;
            break;
            
        case EV_TXSTART:
            #ifdef DEBUG_MODE
            Serial.println(F("TX START"));
            #endif
            loraState = STATE_TRANSMITTING;
            break;
            
        case EV_TXCOMPLETE:
            #ifdef DEBUG_MODE
            Serial.println(F("TX COMPLETE"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("✓ Received ack"));
            if (LMIC.dataLen) {
              Serial.print(F("↓ Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes downlink"));
            }
            #endif
            
            txCount++;
            loraState = STATE_TX_COMPLETE;
            
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
            
        case EV_RXSTART:
            /* Do not print anything -- it wrecks timing */
            break;
            
        case EV_JOIN_TXCOMPLETE:
            #ifdef DEBUG_MODE
            Serial.println(F("JOIN TX COMPLETE (no JoinAccept)"));
            #endif
            break;
            
        default:
            #ifdef DEBUG_MODE
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            #endif
            break;
    }
}

// Function to prepare and send data
void do_send(osjob_t* j) {
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        #ifdef DEBUG_MODE
        Serial.println(F("[LoRa] Skipping - previous TX pending"));
        #endif
        return;
    }

    #ifdef DEBUG_MODE
    Serial.println(F("===== COLLECTING SENSOR DATA ====="));
    #endif

    // ---- Read Bin Fill Level ----
    float rawDistance = measureDistance();
    float adjustedDistance = max(0, min(rawDistance - sensorOffset, binHeight));
    uint8_t fillLevel = (uint8_t)(100.0 * (1.0 - (adjustedDistance / binHeight)));

    // ---- Read Temperature & Humidity (DHT22) using SimpleDHT ----
    // float temperature = -99.9;
    // float humidity = -99.9;

    // ---- Read Temperature & Humidity (DHT22) ----
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Check if any reads failed
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("DHT22 read failed!");
      temperature = -99.99;
      humidity = -99.99;
    }

    // byte temp = 0;
    // byte hum = 0;

    // int err = dht.read(&temp, &hum, NULL);
    // if (err == SimpleDHTErrSuccess) {
    //     temperature = (float)temp;
    //     humidity = (float)hum;
        
    //     #ifdef DEBUG_MODE
    //     Serial.print(F("Temperature: "));
    //     Serial.print(temperature);
    //     Serial.print(F("°C, Humidity: "));
    //     Serial.print(humidity);
    //     Serial.println(F("%"));
    //     #endif
    // } else {
    //     #ifdef DEBUG_MODE
    //     Serial.print(F("DHT Read Error: "));
    //     Serial.println(err);
    //     #endif
    // }

    Serial.println(temperature);

    int16_t tempScaled = (int16_t)(temperature * 10); 
    int16_t humidScaled = (int16_t)(humidity * 10);   

    // ---- Read Smoke Level using MQ2 Sensor with Calibration ----
    uint16_t smokeLevel = (uint16_t)MQGetSmokeReading();
    
    #ifdef DEBUG_MODE
    Serial.print(F("Fill Level: ")); 
    Serial.print(fillLevel);
    Serial.print(F("%, Smoke: "));
    Serial.println(smokeLevel);
    #endif

    // Fixed location - saves memory versus float calculations
    int32_t lat_scaled = 1371038;  // 1.371038 * 1000000
    int32_t lon_scaled = 103825450; // 103.825450 * 1000000

    uint8_t payload[15];
    payload[0] = (tempScaled >> 8) & 0xFF;
    payload[1] = tempScaled & 0xFF;
    payload[2] = (humidScaled >> 8) & 0xFF;
    payload[3] = humidScaled & 0xFF;
    payload[4] = (smokeLevel >> 8) & 0xFF;
    payload[5] = smokeLevel & 0xFF;
    payload[6] = fillLevel;

    // Lat Payload (7-10)
    payload[7] = (lat_scaled >> 24) & 0xFF;
    payload[8] = (lat_scaled >> 16) & 0xFF;
    payload[9] = (lat_scaled >> 8) & 0xFF;
    payload[10] = lat_scaled & 0xFF;

    // Long Payload (11-14)
    payload[11] = (lon_scaled >> 24) & 0xFF;
    payload[12] = (lon_scaled >> 16) & 0xFF;
    payload[13] = (lon_scaled >> 8) & 0xFF;
    payload[14] = lon_scaled & 0xFF;

    // Transmit the data
    LMIC_setTxData2(1, payload, sizeof(payload), 0);
    
    #ifdef DEBUG_MODE
    Serial.println(F("[LoRa] Packet queued for transmission"));
    Serial.print(F("[LoRa] Total packets sent: "));
    Serial.println(txCount);
    #endif
}

void setup() {
    // Initialize status LED
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, HIGH);  // Turn on LED during setup
    
    // #ifdef DEBUG_MODE
    Serial.begin(9600);
    while (!Serial && millis() < 3000); // Wait for Serial but timeout after 3 seconds
    Serial.println(F("\n===== LoRaWAN Waste Monitor Starting ====="));
    // #endif

    // Initialize sensor pins
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    dht.begin();

    // Calibrate MQ2 sensor
    #ifdef DEBUG_MODE
    Serial.println(F("Calibrating MQ2 sensor..."));
    #endif
    
    Ro = MQCalibration(MQ_PIN);
    
    #ifdef DEBUG_MODE
    Serial.print(F("MQ2 Calibration done. Ro = "));
    Serial.println(Ro);
    #endif

    // LMIC initialization
    #ifdef DEBUG_MODE
    Serial.println(F("Initializing LoRaWAN..."));
    #endif
    
    os_init();
    LMIC_reset();
    
    // Disable link check validation to save memory
    LMIC_setLinkCheckMode(0);
    
    // TTN uses SF9 for its RX2 window
    LMIC.dn2Dr = DR_SF9;
    
    // Set data rate and transmit power for uplink
    LMIC_setDrTxpow(DR_SF7, 14);
    
    // Start join procedure and schedule first transmission
    #ifdef DEBUG_MODE
    Serial.println(F("Starting OTAA join procedure..."));
    #endif
    
    do_send(&sendjob);
    
    digitalWrite(STATUS_LED, LOW);  // Turn off LED after setup
}

void loop() {
    // Run LMIC scheduler
    os_runloop_once();
    
    // Update LED status based on LoRa state
    showLoraStatus();
    
    #ifdef DEBUG_MODE
    // Periodically display status (every ~10 seconds)
    static uint32_t lastStatusTime = 0;
    if (millis() - lastStatusTime > 10000) {
        Serial.print(F("[LoRa Status: "));
        switch(loraState) {
            case STATE_INIT: Serial.print(F("INITIALIZING")); break;
            case STATE_JOINING: Serial.print(F("JOINING")); break;
            case STATE_JOINED: Serial.print(F("JOINED")); break;
            case STATE_TRANSMITTING: Serial.print(F("TRANSMITTING")); break;
            case STATE_TX_COMPLETE: Serial.print(F("TX COMPLETE")); break;
            case STATE_ERROR: Serial.print(F("ERROR")); break;
        }
        Serial.print(F(", TX count: "));
        Serial.print(txCount);
        Serial.println(F("]"));
        lastStatusTime = millis();
    }
    #endif
}