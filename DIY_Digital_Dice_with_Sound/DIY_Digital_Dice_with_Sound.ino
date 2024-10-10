#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <DFPlayerMini_Fast.h>

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define NUM_SAMPLES 10 // Number of samples for the moving average

// Arrays to store the last NUM_SAMPLES values
float accelX[NUM_SAMPLES], accelY[NUM_SAMPLES], accelZ[NUM_SAMPLES];
int currentIndex = 0;

// MPU6050 setup
Adafruit_MPU6050 mpu;

// DFPlayer Mini setup
SoftwareSerial dfPlayerSerial(16, 17); // RX, TX for DFPlayer
DFPlayerMini_Fast dfPlayer;
#define VOLUME 30           //Volume of Audio player from 0 to 30 only.

// Button pins
#define BUTTON1_PIN 12
#define BUTTON2_PIN 13

int diceNumber = 1;
bool shakeDetected = false;

void setup()
{
  Serial.begin(115200);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not connected");
    while (1);
  }

  // Initialize DFPlayer Mini
  dfPlayerSerial.begin(9600);
  if (!dfPlayer.begin(dfPlayerSerial,true))
  {
    Serial.println(F("DFPlayer Mini not detected"));
    while (1);
  }
  delay(1000);
  dfPlayer.volume(VOLUME); // Set volume
  delay(1000);
  dfPlayer.volume(VOLUME); // Set volume

  // Button setup
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // Display initial message
  displayDiceRoll(diceNumber);
}

void loop()
{
  checkShake();
  checkButtons();
}

void checkShake()
{
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Add the new data to the arrays for averaging
  accelX[currentIndex] = a.acceleration.x;
  accelY[currentIndex] = a.acceleration.y;
  accelZ[currentIndex] = a.acceleration.z;
  
  currentIndex = (currentIndex + 1) % NUM_SAMPLES; // Move to the next index

  // Compute the average acceleration over the last NUM_SAMPLES
  float avgAccelX = 0, avgAccelY = 0, avgAccelZ = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    avgAccelX += accelX[i];
    avgAccelY += accelY[i];
    avgAccelZ += accelZ[i];
  }
  avgAccelX /= NUM_SAMPLES;
  avgAccelY /= NUM_SAMPLES;
  avgAccelZ /= NUM_SAMPLES;

  // Calculate total average acceleration from all axes
  float totalAcceleration = sqrt(avgAccelX * avgAccelX + 
                                 avgAccelY * avgAccelY + 
                                 avgAccelZ * avgAccelZ);

  // Calculate total angular velocity (gyroscope)
  float totalGyro = sqrt(g.gyro.x * g.gyro.x + g.gyro.y * g.gyro.y + g.gyro.z * g.gyro.z);

  // Thresholds
  float shakeThreshold = 2.5;  // Sensitivity for acceleration
  float gyroThreshold = 1.0;   // Sensitivity for gyroscope

  // Check if both acceleration and angular velocity exceed their respective thresholds
  if (totalAcceleration > shakeThreshold && totalGyro > gyroThreshold) {
    rollDice();  // Trigger the dice roll
    delay(1000); // Debounce time after detecting a shake
  }
}


void rollDice()
{
  diceNumber = random(1, 7); // Random number between 1 and 6
  displayDiceRoll('?');

  // Play a random sound effect (adjust sound file number)
  dfPlayer.play(random(1, 9));
  delay(500);
  while(dfPlayer.isPlaying());
  delay(200);
  
  displayDiceRoll(diceNumber);
  Serial.println("Dice rolled: " + String(diceNumber));
}

void displayDiceRoll(int number)
{
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Rolled: ");
  if(number == '?') display.print('?');
  else              display.print(number);
  display.display();
}

void checkButtons()
{
  // Button 1: Change Game Mode (implement game mode logic)
  if (digitalRead(BUTTON1_PIN) == LOW) {
    Serial.println("Button 1 pressed - change mode");
    // Implement game mode switching here
    delay(500); // Debounce
  }

  // Button 2: Reset Game or Start new roll
  if (digitalRead(BUTTON2_PIN) == LOW) {
    Serial.println("Button 2 pressed - new roll");
    rollDice();
    delay(500); // Debounce
  }
}
