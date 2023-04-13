#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ESP32Time.h>
ESP32Time rtc;

#ifdef U8X8_HAVE_HW_SPI
//#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

TaskHandle_t Task1 = NULL;
TaskHandle_t Task2 = NULL;
TaskHandle_t Task3 = NULL;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "rajan"
#define WIFI_PASSWORD "12345678"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDMp_uzbqG3UyocJ-aSETAeLKGv-M86PaU"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://ii-project-8ff73-default-rtdb.firebaseio.com/"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
float c = 0;
char char_arr[3];
bool signupOK = false;
const int pingPin = 5; // Trigger Pin of Ultrasonic Sensor
const int echoPin = 4; // Echo Pin of Ultrasonic Sensor
long duration, inches, cm;
int ok = 27;
int up = 25;
int down = 26;
int oled_line = 1;
int sp = 0;
int prev = 1;
int old_val;
int sp_old;

long microsecondsToInches(long microseconds) {
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

void displaySetup()
{
  u8g2.begin();
  u8g2.enableUTF8Print();
}

void drawStaticString(uint8_t align, const char *s)
{
  //  u8g2.setDrawColor(0); // clear the scrolling area
  //  u8g2.drawBox(0, 18, u8g2.getDisplayWidth(), u8g2.getDisplayHeight() - 40);
  //  u8g2.setDrawColor(1); // set the color for the text
  u8g2.setFont(u8g2_font_t0_12_tf);
  u8g2.setCursor(0, align);
  u8g2.print(s);
}

void setup() {
  Serial.begin(115200);
  displaySetup();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  drawStaticString(0, "Connected!");
  vTaskDelay(500);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  xTaskCreatePinnedToCore(
    fb_com,   /* Task function. */
    "Task1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);
  vTaskDelay(10);
  xTaskCreatePinnedToCore(
    GPIO_task,   /* Task function. */
    "Task2",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task2,      /* Task handle to keep track of created task */
    1);
  vTaskDelay(10);
  xTaskCreatePinnedToCore(
    Screen_display,   /* Task function. */
    "Task3",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task3,      /* Task handle to keep track of created task */
    1);
  pinMode(pingPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ok, INPUT);
  pinMode(up, INPUT);
  pinMode(down, INPUT);
  pinMode(2, OUTPUT);
}
void loop() {
  vTaskDelay(10);
}

void fb_com(void* args) {
  while (1) {
    uint32_t state;
    xTaskNotifyWait(0, 0, &state, portMAX_DELAY);
    //&& (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)
    if (Firebase.ready() && signupOK ) {
      sendDataPrevMillis = millis();
      if (Firebase.RTDB.setFloat(&fbdo, "Distance", cm)) {
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
      if (Firebase.RTDB.setFloat(&fbdo, "sp", sp)) {
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
      c++;
      vTaskDelay(10);
    }
  }
}

void GPIO_task(void* args) {
  while (1) {

    int ok_read = digitalRead(ok);
    int up_read = digitalRead(up);
    int down_read = digitalRead(down);
    digitalWrite(pingPin, LOW);
    vTaskDelay(2);
    digitalWrite(pingPin, HIGH);
    vTaskDelay(10);
    digitalWrite(pingPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    inches = microsecondsToInches(duration);
    cm = microsecondsToCentimeters(duration);
    if (ok_read == 1) {
      vTaskDelay(200);
      Serial.println("ok was pressed");
      ok_read = digitalRead(ok);
      if (oled_line == 1) {
        sp_old = sp;
        xTaskNotify(Task3, 5, eSetValueWithOverwrite);
        vTaskDelay(10);
        while (oled_line == 1) {
          int ok_read = digitalRead(ok);
          int up_read = digitalRead(up);
          int down_read = digitalRead(down);
          if (up_read == 1) {
            vTaskDelay(200);
            sp++;
            Serial.println(sp);
          }
          if (down_read == 1) {
            vTaskDelay(200);
            sp--;
            Serial.println(sp);
          }
          if (ok_read == 1) {
            Serial.println("ok pressed, loop break!");
            xTaskNotify(Task3, oled_line , eSetValueWithOverwrite);
            vTaskDelay(1500);
            break;
          }
          if (sp != sp_old) {
            xTaskNotify(Task3, 5, eSetValueWithOverwrite);
            sp_old = sp;
            vTaskDelay(10);
          }
          vTaskDelay(20);
        }
      }
      else if (oled_line == 2) {
        xTaskNotify(Task3, 6, eSetValueWithOverwrite);
        vTaskDelay(10);

        while (oled_line == 2) {
          digitalWrite(pingPin, LOW);
          vTaskDelay(2);
          digitalWrite(pingPin, HIGH);
          vTaskDelay(10);
          digitalWrite(pingPin, LOW);
          duration = pulseIn(echoPin, HIGH);
          inches = microsecondsToInches(duration);
          cm = microsecondsToCentimeters(duration);
          digitalWrite(2, HIGH);
          if (cm >= (21-sp)*0.96)) {
            xTaskNotify(Task3, 7, eSetValueWithOverwrite);
            vTaskDelay(1000);
            digitalWrite(2, LOW);
            xTaskNotify(Task3, 1, eSetValueWithOverwrite);
            oled_line = 1;
            break;
          }
          if (cm != old_val) {
            Serial.println("Value update");
            xTaskNotify(Task3, 6, eSetValueWithOverwrite);
            old_val = cm;
            vTaskDelay(100);
          }
          vTaskDelay(1000);
          xTaskNotify(Task1, 1, eSetValueWithOverwrite);
        }
      }
    }
    else if (up_read == 1) {
      oled_line++;
      vTaskDelay(500);
    }
    else if (oled_line > 2) {
      oled_line = 1;
    }
    if (prev != oled_line) {
      Serial.print("line = "); Serial.println(oled_line);
      Serial.print("prev = "); Serial.println(prev);
      xTaskNotify(Task3, oled_line , eSetValueWithOverwrite);
      prev = oled_line;
    }
    vTaskDelay(10);
  }
}
void Screen_display(void* args) {
  while (1) {
    uint32_t state;
    xTaskNotifyWait(0, 0, &state, portMAX_DELAY);
    //Serial.println("recieved state : " + String(state));
    //  xTaskNotify(Task2, 2, eSetValueWithOverwrite);
    switch (state) {
      case 1:
        u8g2.clearDisplay();
        u8g2.drawFrame(0, 0, 128, 17);
        drawStaticString(15, "Set point");
        drawStaticString(31, "Start");
        u8g2.sendBuffer();
        break;
      case 2:
        u8g2.clearDisplay();
        u8g2.drawFrame(0, 17, 128, 21);
        drawStaticString(15, "Set point");
        drawStaticString(31, "Start");
        u8g2.sendBuffer();
        break;
      //      case 3:
      //        u8g2.clearDisplay();
      //        u8g2.drawFrame(0, 33, 128, 37);
      //        drawStaticString(15, "Set point");
      //        drawStaticString(31, "start");
      //        u8g2.sendBuffer();
      //        break;
      case 4:
        u8g2.clearDisplay();
        u8g2.drawFrame(0, 49, 128, 50);
        drawStaticString(15, "line 1");
        drawStaticString(31, "line 2");
        u8g2.sendBuffer();
        break;
      case 5:
        u8g2.clearDisplay();
        u8g2.setFont(u8g2_font_ncenR12_tf);
        u8g2.setCursor(43, 21);
        u8g2.print(u8x8_u8toa(sp, 3));
        u8g2.sendBuffer();
        break;
      case 6:
        u8g2.clearDisplay();
        u8g2.setFont(u8g2_font_ncenR12_tf);
        u8g2.setCursor(43, 42);
        u8g2.print(u8x8_u16toa(cm, 4));
        u8g2.sendBuffer();
        break;
      case 7:
        u8g2.clearDisplay();
        u8g2.setFont(u8g2_font_ncenR12_tf);
        u8g2.setCursor(20, 40);
        u8g2.print("FILLED!");
        u8g2.sendBuffer();
        break;
    }
    vTaskDelay(10);
  }
}
