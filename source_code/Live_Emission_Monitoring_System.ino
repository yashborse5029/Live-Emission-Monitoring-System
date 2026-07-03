#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pins
#define MQ7_PIN 35
#define MQ135_PIN 34
#define DUST_PIN 27
#define BUZZER_PIN 25
#define LED_WHITE 26
#define LED_YELLOW 14
#define LED_RED 12

// OLED Config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Thresholds
const int MODERATE_THRESHOLD = 1200; 
const int HIGH_THRESHOLD = 2500;

// Timing Variables
unsigned long lastUpdateMillis = 0;
const long interval = 2000; // 2 seconds update rate
unsigned long alertMillis = 0;
bool toggleState = false;

// Global Data for Display
int g_mq7 = 0;
int g_mq135 = 0;
float g_dust = 0.0;
String g_status = "OK";

void setup() {
  Serial.begin(115200);
  pinMode(DUST_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_WHITE, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
  display.clearDisplay();
  display.display();
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Every 2 Seconds: Sample and Logic
  if (currentMillis - lastUpdateMillis >= interval) {
    lastUpdateMillis = currentMillis;
    
    // Sampling
    g_mq7 = analogRead(MQ7_PIN);
    g_mq135 = analogRead(MQ135_PIN);
    
    unsigned long duration = pulseIn(DUST_PIN, LOW, 20000); 
    g_dust = (duration / 20000.0) * 100.0;
    if(g_dust > 100.0) g_dust = 100.0; // Cap at 100%

    // Determine Status
    int total = g_mq7 + g_mq135;
    if (total < MODERATE_THRESHOLD) g_status = "NORMAL";
    else if (total < HIGH_THRESHOLD) g_status = "MODERATE";
    else g_status = "HIGH";

    updateOLED();
  }

  // 2. Continuous Non-Blocking Alert Handling
  runAlertHardware(currentMillis);
}

void runAlertHardware(unsigned long now) {
  if (g_status == "NORMAL") {
    digitalWrite(LED_WHITE, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
    noTone(BUZZER_PIN);
  } 
  else if (g_status == "MODERATE") {
    digitalWrite(LED_WHITE, LOW);
    digitalWrite(LED_RED, LOW);
    if (now - alertMillis >= 1000) {
      alertMillis = now;
      toggleState = !toggleState;
      digitalWrite(LED_YELLOW, toggleState);
      if(toggleState) tone(BUZZER_PIN, 1000); else noTone(BUZZER_PIN);
    }
  } 
  else if (g_status == "HIGH") {
    digitalWrite(LED_WHITE, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, HIGH);
    tone(BUZZER_PIN, 1500);
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Values Section
  display.setCursor(0, 0);
  display.print("MQ7  : "); display.println(g_mq7);
  
  display.setCursor(0, 15);
  display.print("MQ135: "); display.println(g_mq135);
  
  // Dust formatted to 6 chars max (e.g., 12.34%)
  char dustStr[10];
  dtostrf(g_dust, 5, 2, dustStr); 
  display.setCursor(0, 30);
  display.print("DUST : "); display.print(dustStr); display.println("%");

  // Status Section (Separated visually)
  display.drawLine(0, 45, 128, 45, WHITE);
  display.setCursor(0, 52);
  display.setTextSize(1);
  display.print("STATUS: ");
  display.setTextSize(1); // Keep it clean
  display.print(g_status);

  display.display();
}