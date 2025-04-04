#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>

// ------- MQ2 Sensor Configuration -------
#define         MQ_PIN                       (A0)    // MQ2 Analog Pin
#define         RL_VALUE                     (5)     // Load resistance on board, in kilo ohms
#define         RO_CLEAN_AIR_FACTOR          (9.83)  // RO_CLEAN_AIR_FACTOR=(Sensor resistance in clean air)/RO
                                                     
// MQ2 Calibration Parameters
#define         CALIBARAION_SAMPLE_TIMES     (50)    // Number of samples in calibration phase
#define         CALIBRATION_SAMPLE_INTERVAL  (500)   // Time interval between samples (ms)
#define         READ_SAMPLE_INTERVAL         (50)    // Time interval between samples in normal operation
#define         READ_SAMPLE_TIMES            (5)     // Number of samples in normal operation

// MQ2 Gas Identifiers
#define         GAS_LPG                      (0)
#define         GAS_CO                       (1)
#define         GAS_SMOKE                    (2)

// MQ2 Gas Curves - from datasheet
float           LPGCurve[3]  =  {2.3,0.21,-0.47};
float           COCurve[3]   =  {2.3,0.72,-0.34};
float           SmokeCurve[3]=  {2.3,0.53,-0.44};
float           Ro           =  10;  // Initial Ro value

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
#define DHTPIN 8       // DHT22 Data Pin
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define trigPin 3      // HC-SR04 Trigger Pin
#define echoPin 4      // HC-SR04 Echo Pin

// ------- LoRa Pin Mapping for RFM95 Shield -------
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 7,
    .dio = {2, 5, 6},
};

static osjob_t sendjob;

// ------- MQ2 Sensor Functions -------
float MQResistanceCalculation(int raw_adc) {
  return ((float)RL_VALUE * (1023 - raw_adc) / raw_adc);
}

float MQCalibration(int mq_pin) {
  int i;
  float val = 0;

  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES;
  val = val / RO_CLEAN_AIR_FACTOR;

  return val;
}

float MQRead(int mq_pin) {
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  rs = rs / READ_SAMPLE_TIMES;
  return rs;
}

int MQGetGasPercentage(float rs_ro_ratio, float *pcurve) {
  return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}

int MQGetSmokeReading() {
  float rs_ro_ratio = MQRead(MQ_PIN) / Ro;
  return MQGetGasPercentage(rs_ro_ratio, SmokeCurve);
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

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
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

    int16_t tempScaled = (int16_t)(temperature * 10);  // Scaled value to integer
    int16_t humidScaled = (int16_t)(humidity * 10);    // Scaled value to integer

    // ---- Read Smoke Level using MQ2 Sensor with Calibration ----
    uint16_t smokeLevel = (uint16_t)MQGetSmokeReading(); // Get calibrated smoke reading

    // ---- Logging (comment out later) ----
    Serial.print("Bin fill level: ");
    Serial.println(fillLevel);
    Serial.print("Smoke PPM: ");
    Serial.println(smokeLevel);
    Serial.print("Temperature: ");
    Serial.println(tempScaled);
    Serial.print("Humidity: ");
    Serial.println(humidScaled);

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
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial.println();
            }
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

// ------- Setup Function -------
void setup() {
    Serial.begin(9600);
    Serial.println(F("Starting Smart Bin with MQ2 Sensor"));

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    dht.begin();

    // MQ2 Sensor Calibration
    Serial.println("Calibrating MQ2 sensor...");
    Serial.println("This will take about 25 seconds");
    Ro = MQCalibration(MQ_PIN);
    Serial.print("Calibration done. Ro = ");
    Serial.print(Ro);
    Serial.println(" kohm");

    // Initialize LMIC
    os_init();
    LMIC_reset();

    // First transmission
    do_send(&sendjob);
}

// ------- Loop Function (LoRa Processing) -------
void loop() {
    os_runloop_once();
}