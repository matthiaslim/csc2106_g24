#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin definitions
const int trigPin = 9;
const int echoPin = 10;

// Bin configuration
const float binHeight = 50.0; // Height of bin in cm - ADJUST THIS!
const float sensorOffset = 2.0; // Distance from sensor to top of bin in cm

// Variables for sensor readings
float duration, distance, lastValidDistance;
float fillPercentage;
const int numReadings = 5;
float readings[5];
int readIndex = 0;
float total = 0;

// Variables for error detection
const float maxValidDistance = binHeight + 10.0; // Maximum valid distance reading
const float minValidDistance = 2.0; // Minimum valid distance reading
boolean sensorError = false;

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
  
  // Initialize all readings to 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  // Initial display setup
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Bin Level Monitor"));
  display.setCursor(0,16);
  display.println(F("Initializing..."));
  display.display();
  delay(2000);
}

void loop() {
  // Get distance measurement
  float rawDistance = measureDistance();
  
  // Check if reading is valid
  if (rawDistance > minValidDistance && rawDistance < maxValidDistance) {
    sensorError = false;
    lastValidDistance = rawDistance;
    
    // Add to moving average
    total = total - readings[readIndex];
    readings[readIndex] = rawDistance;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;
    
    // Calculate the smoothed distance
    distance = total / numReadings;
    
    // Calculate fill percentage
    // Adjusted distance accounts for sensor placement
    float adjustedDistance = distance - sensorOffset;
    if (adjustedDistance < 0) adjustedDistance = 0;
    fillPercentage = 100.0 * (1.0 - (adjustedDistance / binHeight));
    
    // Constrain percentage between 0-100
    if (fillPercentage < 0) fillPercentage = 0;
    if (fillPercentage > 100) fillPercentage = 100;
  } else {
    sensorError = true;
  }
  
  // Log to serial
  Serial.print("Raw Distance: ");
  Serial.print(rawDistance);
  Serial.print(" cm, Filtered: ");
  Serial.print(distance);
  Serial.print(" cm, Fill: ");
  Serial.print(fillPercentage);
  Serial.println("%");
  
  // Update display
  updateDisplay();
  
  delay(1000); // Time between readings
}

float measureDistance() {
  // Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Send a 10µs pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Measure the response
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate distance
  return (duration / 2) / 29.1; // Convert to cm
}

void updateDisplay() {
  display.clearDisplay();
  
  // Title
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Bin Level Monitor"));
  
  if (sensorError) {
    // Error message
    display.setCursor(0,16);
    display.println(F("Sensor Error!"));
    display.setCursor(0,26);
    display.println(F("Check positioning"));
  } else {
    // Distance reading
    display.setCursor(0,16);
    display.print(F("Dist: "));
    display.print(distance);
    display.println(F(" cm"));
    
    // Fill percentage
    display.setTextSize(2);
    display.setCursor(0,26);
    display.print(round(fillPercentage));
    display.println(F("%"));
    
    // Draw fill level bar
    int barWidth = map(round(fillPercentage), 0, 100, 0, SCREEN_WIDTH-4);
    display.drawRect(0, 48, SCREEN_WIDTH, 16, SSD1306_WHITE);
    display.fillRect(2, 50, barWidth, 12, SSD1306_WHITE);
  }
  
  display.display();
}