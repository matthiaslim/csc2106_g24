/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>

//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

// ------- Bin Configuration -------
const float binHeight = 50.0;  // Height of bin in cm
const float sensorOffset = 2.0;  // Distance from sensor to top of bin in cm
const int numReadings = 5;       // Moving Average Filter
float readings[numReadings] = {0}; 
int readIndex = 0;
float total = 0;

// ------- Pin Definitions -------
#define DHTPIN 5       // DHT22 Data Pin
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define trigPin 3      // HC-SR04 Trigger Pin
#define echoPin 4      // HC-SR04 Echo Pin

#define co2Pin A0      // MQ-2 Smoke Sensor Analog Output

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Device 1
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={ 0x65, 0xEB, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 }; // Device 2
// static const u1_t PROGMEM DEVEUI[8]={ 0xD7, 0xE9, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 }; // Device 1

void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0x6A, 0x63, 0x7C, 0x0A, 0x87, 0x2F, 0x37, 0x8C, 0xBB, 0x51, 0x37, 0x91, 0x2F, 0x27, 0x26, 0x87 }; // Device 2
// static const u1_t PROGMEM APPKEY[16] = { 0xD1, 0x07, 0x9E, 0x57, 0x33, 0x8A, 0x04, 0x8A, 0xD8, 0x18, 0xFD, 0xE2, 0xF5, 0xD7, 0xF2, 0xD9 }; // Device 1
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

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


// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 7,
    .dio = {2, 5, 6},
};

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
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
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
	    // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
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
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
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

// Send stuff here
void do_send(osjob_t* j){
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

    // ---- Read Smoke/CO2 Level (MQ-2 Sensor) ----
    int co2Raw = analogRead(co2Pin);
    float voltage = co2Raw * (5.0 / 1023.0);
    uint16_t smokeLevel = (uint16_t)(voltage * 200); // Example scaling

    float latitude = 1.371038;
    float longitude = 103.825450;

    int32_t lat_scaled = (int32_t)(latitude * 1000000);
    int32_t lon_scaled = (int32_t)(longitude * 1000000);
    

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

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, payload, sizeof(payload), 0); // 0 for unconfirmed ACK, 1 for confirmed ACK
        // Serial.println(temperature);
        // Serial.println(humidity);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(9600);
    Serial.println(F("Starting"));

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    dht.begin();

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}