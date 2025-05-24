#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
const int ECG_PIN = A0; // Analog input pin
const int UPPER_THRESHOLD = 570; // 2.9V (2.5V + 0.4V)
const int LOWER_THRESHOLD = 555; // 2.8V (2.5V + 0.3V)
unsigned long lastPeakTime = 0;
boolean isPeakDetected = false;
int bpm = 0;
const int NUM_READINGS = 10; // Average over 10 beats for stability
unsigned long peakTimes[10]; // Store timestamps of peaks
int peakIndex = 0;

const int PLOT_WIDTH = 96;  
const int PLOT_HEIGHT = 35; 
int ecgPlotData[PLOT_WIDTH]; 

// Amplification factor for pulse size
const float PULSE_AMPLIFICATION = 3;

void setup() {
  Serial.begin(9600);
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("ECG Monitor"));
  display.println(F("Initializing..."));
  display.display();
  delay(2000);
  
  for (int i = 0; i < PLOT_WIDTH; i++) {
    ecgPlotData[i] = PLOT_HEIGHT / 2;
  }
}

void loop() {
  int ecgValue = analogRead(ECG_PIN);
  
  updateEcgPlot(ecgValue);
  
  // Peak detection algorithm
  if (ecgValue > UPPER_THRESHOLD && !isPeakDetected) {
    unsigned long currentTime = millis();
    if (lastPeakTime > 0) {
      peakTimes[peakIndex] = currentTime - lastPeakTime;
      peakIndex = (peakIndex + 1) % NUM_READINGS;
      if (peakTimes[0] > 0) {
        unsigned long totalTime = 0;
        int validReadings = 0;
        for (int i = 0; i < NUM_READINGS; i++) {
          if (peakTimes[i] > 100) {
            totalTime += peakTimes[i];
            validReadings++;
          }
        }
        if (validReadings > 0) {
          float averageInterval = totalTime / (float)validReadings;
          bpm = int(60000.0 / averageInterval);
          if (bpm >= 40 && bpm <= 200) {
            Serial.print("BPM: ");
            Serial.println(bpm);
            updateDisplay();
          }
        }
      }
    }
    lastPeakTime = currentTime;
    isPeakDetected = true;
  }
  
  if (ecgValue < LOWER_THRESHOLD) {
    isPeakDetected = false;
  }

  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 100) { 
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  
  delay(10);
}

void updateEcgPlot(int ecgValue) {
  int midPoint = PLOT_HEIGHT / 2;
  for (int i = 0; i < PLOT_WIDTH - 1; i++) {
    ecgPlotData[i] = ecgPlotData[i + 1];
  }

  int mappedValue = map(constrain(ecgValue, 0, 1023), 0, 1023, PLOT_HEIGHT, 0);
  int amplifiedValue;
  if (mappedValue < midPoint) {
    amplifiedValue = midPoint - (int)((midPoint - mappedValue) * PULSE_AMPLIFICATION);
    amplifiedValue = constrain(amplifiedValue, 0, PLOT_HEIGHT);
  } else {
    amplifiedValue = midPoint + (int)((mappedValue - midPoint) * PULSE_AMPLIFICATION);
    amplifiedValue = constrain(amplifiedValue, 0, PLOT_HEIGHT);
  }

  ecgPlotData[PLOT_WIDTH - 1] = amplifiedValue;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("BPM: ");
  if (bpm >= 40 && bpm <= 200) {
    display.println(bpm);
  } else {
    display.println("--");
  }
  
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("ECG Signal:");
  
  display.drawRect(0, 28, PLOT_WIDTH, PLOT_HEIGHT, SSD1306_WHITE);

  for (int i = 0; i < PLOT_WIDTH - 1; i++) {
    display.drawLine(i, ecgPlotData[i] + 28, i + 1, ecgPlotData[i + 1] + 28, SSD1306_WHITE);
  }
  if (isPeakDetected) {
    display.fillCircle(120, 15, 5, SSD1306_WHITE);
  }
  display.display();
}