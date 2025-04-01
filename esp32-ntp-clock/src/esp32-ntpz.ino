#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C 0x27, display 16x2

// Configuração dos botões (usando pull-up interno)
#define BTN_HOUR_UP 12
#define BTN_MIN_UP 13
#define BTN_HOUR_DN 14
#define BTN_MIN_DN 15
#define BTN_CONFIRM 16

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600; // UTC-3 (Brasília)
const int daylightOffset_sec = 0;

bool adjustMode = false;
int adjustedHour = 0;
int adjustedMinute = 0;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);
  
  // Inicializa o LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Configura os botões com pull-up interno
  pinMode(BTN_HOUR_UP, INPUT_PULLUP);
  pinMode(BTN_MIN_UP, INPUT_PULLUP);
  pinMode(BTN_HOUR_DN, INPUT_PULLUP);
  pinMode(BTN_MIN_DN, INPUT_PULLUP);
  pinMode(BTN_CONFIRM, INPUT_PULLUP);

  // Conecta ao Wi-Fi
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    lcd.setCursor(0, 1);
    lcd.print(".");
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Conectado!");
  delay(1000);

  // Configura o NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  static unsigned long lastUpdate = 0;
  
  // Verifica os botões
  if (millis() - lastDebounceTime > debounceDelay) {
    checkButtons();
    lastDebounceTime = millis();
  }

  // Atualiza o display
  if (millis() - lastUpdate >= 500) {
    if (adjustMode) {
      displayAdjustedTime();
    } else {
      displayNetworkTime();
    }
    lastUpdate = millis();
  }
}

void checkButtons() {
  // Botão de confirmação
  if (digitalRead(BTN_CONFIRM) == LOW) {
    adjustMode = !adjustMode;
    if (!adjustMode) {
      // Aplica os ajustes
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        timeinfo.tm_hour = adjustedHour;
        timeinfo.tm_min = adjustedMinute;
        time_t adjustedTime = mktime(&timeinfo);
        struct timeval tv = { adjustedTime, 0 };
        settimeofday(&tv, NULL);
      }
    } else {
      // Entra no modo de ajuste
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        adjustedHour = timeinfo.tm_hour;
        adjustedMinute = timeinfo.tm_min;
      }
    }
    delay(300); // Debounce simples
    return;
  }

  if (adjustMode) {
    if (digitalRead(BTN_HOUR_UP) == LOW) {
      adjustedHour = (adjustedHour + 1) % 24;
      delay(200);
    }
    if (digitalRead(BTN_HOUR_DN) == LOW) {
      adjustedHour = (adjustedHour - 1 + 24) % 24;
      delay(200);
    }
    if (digitalRead(BTN_MIN_UP) == LOW) {
      adjustedMinute = (adjustedMinute + 1) % 60;
      delay(200);
    }
    if (digitalRead(BTN_MIN_DN) == LOW) {
      adjustedMinute = (adjustedMinute - 1 + 60) % 60;
      delay(200);
    }
  }
}

void displayNetworkTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    lcd.setCursor(0, 0);
    lcd.print("Erro sincronia");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.printf("Hora: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  lcd.setCursor(0, 1);
  lcd.printf("Data: %02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
}

void displayAdjustedTime() {
  lcd.setCursor(0, 0);
  lcd.print("Ajuste de Hora  ");
  
  lcd.setCursor(0, 1);
  lcd.printf("Hora: %02d:%02d  ", adjustedHour, adjustedMinute);
  
  // Pisca os dois pontos para indicar modo de ajuste
  static bool blink = false;
  lcd.setCursor(8, 1);
  lcd.print(blink ? ":" : " ");
  blink = !blink;
}