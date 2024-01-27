#include "DHTesp.h"
#include <WiFi.h>
#include "PubSubClient.h"
#include <LiquidCrystal_I2C.h>


const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "broker.hivemq.com";
const char* mqttClient= "21127147_21127365_21127644";
const int port = 1883;

const int LED_PIN_1 = 15;
const int LED_PIN_2 = 2;
const int LED_PIN_3 = 4;
const int LED_PIN_4 = 5;
const int DHT22_PIN = 18;
const int BUZZER_PIN = 19;
const int mq135_simulator = 32;

DHTesp dhtSensor;
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 20, 4);
bool wifiConnected = false;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  Serial.print("Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// decorations while connecting to wifi.
void spinner() 
{
  static int8_t counter = 0;
  const char* glyphs = "\xa1\xa5\xdb";
  LCD.setCursor(15, 1);
  LCD.print(glyphs[counter++]);
  if (counter == strlen(glyphs)) {
    counter = 0;
  }
}
// connect to wifi. 
void wifiConnect() 
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    spinner();
    Serial.print(".");
  }
  Serial.println(" Connected");
}
// activate BUZZER_PIN on sin value
// void activate_BUZZER_PIN()
// {
//   for(int x = 0; x < 180; x++)
//   {
//     float sinVal = sin(x*(3.1412/180));
//     int toneVal = 2000 + int(sinVal * 1000);
//     tone(BUZZER_PIN, toneVal);
//     delay(2);
//   }
// }
// deactivate BUZZER_PIN.
// initialize LCD
void LCD_init()
{
  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Connecting to ");
  LCD.setCursor(0, 1);
  LCD.print("WiFi ");
}
// print local IP on LCD after connected.
void LCD_show_localIP()
{
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("Online");
  LCD.setCursor(0, 1);
  LCD.print("Local IP: ");
  LCD.print(WiFi.localIP());
  LCD.setCursor(0, 2);
  LCD.print("Getting data in ");
  LCD.print("3");
  delay(1000);
  LCD.setCursor(16, 2);
  LCD.print("2");
  delay(1000);
  LCD.setCursor(16, 2);
  LCD.print("1");
  delay(1000);
  LCD.clear();
}
// Print data on LCD
void LCD_handle(int temperature, int humidity, int mq135_value)
{
  LCD.setCursor(0, 0);
  LCD.print("Current data: ");
  LCD.setCursor(0, 1);
  LCD.print("Temperature: ");
  LCD.print(String(temperature) + "oC");
  LCD.setCursor(0, 2);
  LCD.print("Humidity: ");
  LCD.print(String(humidity) + "%");
  LCD.setCursor(0, 3);
  LCD.print("MQ135: ");
  LCD.print(String(mq135_value) + "PPM");
}

void Turn_On_All_Led()
{
  digitalWrite(LED_PIN_1, HIGH);
  digitalWrite(LED_PIN_2, HIGH);
  digitalWrite(LED_PIN_3, HIGH);
  digitalWrite(LED_PIN_4, HIGH);
}

void Turn_Off_All_Led()
{
  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, LOW);
  digitalWrite(LED_PIN_3, LOW);
  digitalWrite(LED_PIN_4, LOW);
}

void Turn_On_Warning_Led()
{
  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, LOW);
  digitalWrite(LED_PIN_3, HIGH);
  digitalWrite(LED_PIN_4, HIGH);
}

float Read_ppm_data()
{
  int value = analogRead(mq135_simulator);

  int to_5V_value = map(value, 0, 4095, 0, 1023);

  float to_ppm = to_5V_value * 1200 / 1023;

  return to_ppm;
}


void Warning(float temp, float humidity, float ppm)
{
  if (temp > 40 || ppm > 800) // Air quality warning
  {
    if (temp > 70 || ppm > 1000 ) // Temperature or air quality alarm
  {
    Turn_On_All_Led();
    tone(BUZZER_PIN, 1000, 50);
    delay(150);
    tone(BUZZER_PIN, 1000, 50);
    client.publish("21127147_21127365_21127644/Warning", "Temperature is very high or air quality is critical. Modify the air conditioner or open the window!");
  }

    else{
    Turn_On_Warning_Led();
    tone(BUZZER_PIN, 400, 50);
    delay(150);
    tone(BUZZER_PIN, 400, 50);
    // Publish air quality warning message
    client.publish("21127147_21127365_21127644/Warning", "Temperature is quite high or air quality is getting worse. Be careful!");
    }
  }
  else if (temp <= 5){
    Turn_On_All_Led();
    tone(BUZZER_PIN, 1000, 50);
    delay(150);
    tone(BUZZER_PIN, 1000, 50);
    client.publish("21127147_21127365_21127644/Warning", "Temperature is very low .Heat up the room!!");

  }
  
  else
  {
    Turn_Off_All_Led();
    noTone(BUZZER_PIN);
    // Publish normal status message
    client.publish("21127147_21127365_21127644/Status", "Air quality and temperature are normal.");
  }


  Serial.println("Temp: " + String(temp, 2) + "°C");
  Serial.println("Humidity: " + String(humidity, 1) + "%");
  Serial.println("gaz In Air(PPM): " + String(ppm, 2) + "ppm");
  Serial.println("--------");
}


void mqttReconnect()
{
  while (!client.connected())
  {
    Serial.println("Attempting MQTT reconnection...");
    if (client.connect("21127147_21127365_21127644"))
    {
      Serial.println("Connected to your device!");
    }
    else
    {
      Serial.println("Try again in 5 seconds...");
      delay(5000);
    }
  }
}
void setup() {
    Serial.begin(115200);
    dhtSensor.setup(DHT22_PIN, DHTesp::DHT22);
    pinMode(LED_PIN_1, OUTPUT);
    pinMode(LED_PIN_2, OUTPUT);
    pinMode(LED_PIN_3, OUTPUT);
    pinMode(LED_PIN_4, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(mq135_simulator, INPUT);
    LCD_init();

    // Connect to WiFi only once
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    LCD_show_localIP();

    // Setup MQTT
    client.setServer(mqttServer, port);
    client.setCallback(callback);
    while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqttClient )) {//in this case no user, no password are needed
      Serial.println("connected");  
    } 
    else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

}



void loop() {
  // Connect to MQTT if not connected
  if (!client.connected()) {
    mqttReconnect();
  }

  client.loop();

  // Read sensor data
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float ppm = Read_ppm_data();
  float temperature = data.temperature;
  float humidity = data.humidity;
  LCD_handle(data.temperature, data.humidity, ppm);
  // Send data to MQTT
  if (client.connected()) {
    char buffer[100];
    sprintf(buffer, "{\"temperature\": %f, \"humidity\": %f, \"gaz concentration(PPM)\": %f}", temperature, humidity, ppm);
    client.publish("21127147_21127365_21127644/Data", buffer);
  }

  // Handle warnings
  Warning(data.temperature, data.humidity, ppm);

  delay(1000);
}
