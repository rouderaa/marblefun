#include <Arduino.h>

/*
 * ESP32 Breathing LED Effect
 *
 * This sketch creates a "breathing" effect for an LED connected to pin 33
 * using ESP32's ledc peripheral for precise digital control.
 */

// Define the LED pin - pin 33 is valid on ESP32
const int ledPin = 19;

// LEDC configuration
#define LEDC_CHANNEL_0 0     // Use channel 0
#define LEDC_TIMER_13_BIT 13 // 13-bit resolution (0-8191)
#define LEDC_BASE_FREQ 5000  // Base frequency 5000Hz

// Constants for the breathing effect
#define MAX_BRIGHTNESS 8191 // Maximum brightness value (2^13 - 1)
#define BREATH_DELAY 15     // Delay between steps in milliseconds
#define MOTOR_SLEEP 27
#define MOTOR_LEFT 21
#define MOTOR_RIGHT 25

#define CLIMBDELAY 9309

static int ledAngle = 0;
static unsigned long timestamp;
static unsigned long climbtimestamp;
static unsigned long startuptimestamp;
// Define the light gate pin to read from
const int lightGatePin = 35; // uses channel7 adc
static int gateValue = 0;
static int state = 0;
static unsigned long startTime;
static unsigned long endTime;
unsigned long diffTime;

void setup()
{
  // Configure LED PWM functionality using LEDC
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);

  // Attach the channel to the GPIO pin for output
  ledcAttachPin(ledPin, LEDC_CHANNEL_0);
  ledAngle = 0;
  timestamp = millis();

  pinMode(lightGatePin, INPUT);
  pinMode(MOTOR_SLEEP, OUTPUT);
  pinMode(MOTOR_LEFT, OUTPUT);
  pinMode(MOTOR_RIGHT, OUTPUT);

  digitalWrite(MOTOR_SLEEP, 0);

  Serial.begin(115200);
  Serial.println("esp32Marblefun");
}

static void updateLed()
{
  // Convert degrees to radians for the sin() function
  float radians = ledAngle * (PI / 180.0);

  // Calculate brightness using sine function
  // Maps sin output (-1 to +1) to brightness values (0 to MAX_BRIGHTNESS)
  float sinValue = sin(radians);
  int brightness = (int)((sinValue + 1.0) * MAX_BRIGHTNESS / 2.0);

  // Set the LED brightness
  ledcWrite(LEDC_CHANNEL_0, brightness);
}

void motorLeft()
{
  digitalWrite(MOTOR_SLEEP, 0);
  digitalWrite(MOTOR_LEFT, 1);
  digitalWrite(MOTOR_RIGHT, 0);
  digitalWrite(MOTOR_SLEEP, 1);
}

void motorRight()
{
  digitalWrite(MOTOR_SLEEP, 0);
  digitalWrite(MOTOR_LEFT, 0);
  digitalWrite(MOTOR_RIGHT, 1);
  digitalWrite(MOTOR_SLEEP, 1);
}

void motorOff()
{
  digitalWrite(MOTOR_SLEEP, 0);
}

void showSensor()
{
  // Stop showing at any keypress
  while (!Serial.available())
  {
    int sensor = digitalRead(lightGatePin);
    Serial.printf("Sensor: %d\r\n", sensor);
    delay(500);
  }
  Serial.println("Done");
}

static void setState(int newState)
{
  state = newState;
  Serial.printf("State:%d\r\n", state);
}

void commandHandler()
{
  if (Serial.available())
  {
    int ch = Serial.read();
    switch (ch)
    {
    case 'q':
      Serial.printf("motor left on\r\n");
      startTime = millis();
      motorLeft();
      break;
    case 'w':
      Serial.printf("motor right on\r\n");
      startTime = millis();
      motorRight();
      break;
    case 'e':
      endTime = millis();
      motorOff();
      Serial.printf("motor off\r\n");
      diffTime = endTime - startTime;
      Serial.printf("Motor time:%ld\r\n", diffTime);
      break;
    case 's':
      Serial.printf("Sensor\r\n");
      showSensor();
      break;
    case 'g':
      Serial.printf("Go !\r\n");
      startuptimestamp = millis() + 3000;
      setState(1);
      break;
    default:
      Serial.printf("Key: %d %c\r\n", ch, ch);
      break;
    }
  }
}

static void stateHandler()
{
  switch (state)
  {
  case 0:
    break;
  case 1:
    if ((gateValue == 0) || (millis() > startuptimestamp))
    {
      motorRight();
      setState(2);
      climbtimestamp = millis() + CLIMBDELAY;
    }
    break;
  case 2:
    if (millis() > climbtimestamp)
    {
      motorOff();
      setState(3);
    }
    break;
  case 3:
    break;
  }
}

void loop()
{
  // Show pulsating led
  if (timestamp < millis())
  {
    timestamp = millis() + BREATH_DELAY;
    ledAngle++;
    if (ledAngle == 360)
      ledAngle = 0;
    updateLed();
  }

  commandHandler();
  stateHandler();

  // Sample gate
  gateValue = digitalRead(lightGatePin);
}