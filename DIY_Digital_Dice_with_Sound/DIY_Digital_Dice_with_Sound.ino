#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <DFPlayerMini_Fast.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MPU6050 setup
Adafruit_MPU6050 mpu;
// Arrays to store the last NUM_SAMPLES values
float accelX[NUM_SAMPLES], accelY[NUM_SAMPLES], accelZ[NUM_SAMPLES];
int currentIndex = 0;

// DFPlayer Mini setup
SoftwareSerial dfPlayerSerial(16, 17); // RX, TX for DFPlayer
DFPlayerMini_Fast dfPlayer;
#define VOLUME 15           //Volume of Audio player from 0 to 30 only.

// Button pins
#define BUTTON1_PIN 12
#define BUTTON2_PIN 13

#define NUM_SAMPLES 10 // Number of samples for the moving average

//Global Variables
int diceNumber = 1;
bool shakeDetected = false;

//Wifi and MQTT configration
WiFiClient netLocal;
PubSubClient clientLocal(netLocal);
char message[50];
const uint8_t timeout     = 120; // seconds to run for AP to stop if ESP32 didn't connect to WiFi
uint8_t     Roll_Again    = 0;
const char* Local_Broker  = "192.168.1.2";
#define THINGNAME           "test1"
#define SUB_TOPIC           "esp32/sub"
#define PUB_TOPIC           "esp32/pub"



///////////////////////////////////////////////////////////////////
//********* Functions ******* Functions ***** Functions *********//
///////////////////////////////////////////////////////////////////
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
  if (!mpu.begin())
  {
    Serial.println("MPU6050 not connected");
    while (1);
  }

  //Wifi configration
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm;
  bool res = wm.autoConnect("Dice Roll AP"); // anonymous ap
  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }

  // Initialize DFPlayer Mini
  dfPlayerSerial.begin(9600);
  if (!dfPlayer.begin(dfPlayerSerial, false))
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

  // Connect to MQTT Broker and subcribe to a topic
  localConnect();
}

void loop()
{
  checkShake();
  checkButtons();
  clientLocal.loop();
}

//function used to check on shackes using MPU6050.
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
  if (totalAcceleration > shakeThreshold && totalGyro > gyroThreshold)
  {
    rollDice();  // Trigger the dice roll
    delay(1000); // Debounce time after detecting a shake
  }
  if (Roll_Again)
  {
    rollDice();  // Trigger the dice roll
    Roll_Again = 0;
    delay(1000); // Debounce time after detecting a shake
  }
}

//function used to generate random number of dice that is from 1 to 6 and send it to screen, topic and Serial
void rollDice()
{
  diceNumber = random(1, 7); // Random number between 1 and 6
  displayDiceRoll('?');

  // Play a random sound effect (adjust sound file number)
  dfPlayer.play(random(1, 9));
  delay(500);
  while (dfPlayer.isPlaying());
  delay(200);

  displayDiceRoll(diceNumber);
  snprintf(message, 50, "Rolled %d", diceNumber);
  clientLocal.publish(PUB_TOPIC, message);
  Serial.println("Dice rolled: " + String(diceNumber));
}

//function used to print new roll number on serial
void displayDiceRoll(int number)
{
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Rolled: ");
  if (number == '?') display.print('?');
  else              display.print(number);
  display.display();
}

// Function used to check if any Button is pressed one button for manual rolling and the other for wifi AP configration
void checkButtons()
{
  // Button 1: Change Game Mode (implement game mode logic)
  if (digitalRead(BUTTON1_PIN) == LOW)
  {
    Serial.println("Button 1 pressed - active Wifi configration AP");
    // Implement game mode switching here
    WiFiManager wm;
    wm.setConfigPortalTimeout(timeout); //Set a time that if the ESP32 is not connected to a wifi at it it would restart

    if (!wm.startConfigPortal("Dice Roll AP"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    delay(1000); // Debounce
  }

  // Button 2: Reset Game or Start new roll
  if (digitalRead(BUTTON2_PIN) == LOW)
  {
    Serial.println("Button 2 pressed - new roll");
    rollDice();
    delay(500); // Debounce
  }
}

//Function that is called if there is incomming messages from subscribed topic to handle that message.
void messageHandler(char *topic, uint8_t *payload, unsigned int length)
{
    Serial.print("incoming: ");
    Serial.println(topic);

    JsonDocument filter;
    filter["again"]          = true;

    JsonDocument doc;
    DeserializationError DE_error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (DE_error)
    {
        Serial.print(F("JSON parsing failed! Error: "));
        Serial.println(DE_error.c_str());
        return;
    }
    Serial.println();

    Roll_Again = (uint8_t) doc["again"];
    
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]); // Printing payload content
    }
    Serial.println();
}

//Function to connect and subscribe to a topic 
void localConnect()
{
  // // Connect to the local MQTT broker
  clientLocal.setServer(Local_Broker, 1883);

  // Create a message handler
  clientLocal.setCallback(messageHandler);

  Serial.println("Connecting to Local MQTT");
  while (!clientLocal.connect(THINGNAME))
  {
    static uint8_t counter = 0;
    Serial.print(".");
    delay(100);
    counter++;
    if (counter >= 2)
    {
      counter = 0;
      break;
    }
  }

  if (!clientLocal.connected())
  {
    Serial.println("MQTT local Timeout!");
    return;
  }

  // Subscribe to a topic
  clientLocal.subscribe(SUB_TOPIC);

  Serial.println("Local MQTT Connected!");
}
