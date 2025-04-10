#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <SPI.h>
#include <SD.h>
#include <NTPClient.h>

#ifndef PSTR
#define PSTR 
#endif

#define MATRIX_WIDTH 16
#define MATRIX_HEIGHT 16
#define LED D3
#define DATA_PIN D2
#define SD_CS D8
#define DHTPIN D1
#define FIREBASE_HOST "https://itbootcamp-de119-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "b54fe5fe9f3c38fe612c84ed63a1222a396f1953"
#define WIFI_SSID "Igor"
#define WIFI_PASSWORD "12345678-10"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix( //объявление матрицы
  MATRIX_WIDTH, MATRIX_HEIGHT, DATA_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800
);

DHT dht(DHTPIN, DHT11);

// Таймеры для замены delay()
unsigned long previousSensorUpdate = 0;
const long sensorInterval = 10000; // Интервал обновления датчиков (1 сек)
unsigned long previousImageUpdate = 0;
const long imageInterval = 1000;  // Интервал обновления изображения (1 сек)

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(D2, INPUT);
  pinMode(LED, OUTPUT);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("Ошибка инициализации SD-карты!");
    digitalWrite(LED, 1);
    return;
  }
  Serial.println("SD-карта инициализирована");
  digitalWrite(LED ,0);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
    digitalWrite(LED, 1);
  }
  Serial.println("WiFi connected");
  digitalWrite(LED, 0);

  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  config.database_url = FIREBASE_HOST;
  Firebase.reconnectNetwork(true);
  Firebase.begin(&config, &auth);
  matrix.begin();
  matrix.setBrightness(100);
  matrix.fillScreen(0);
  matrix.show();
}

void loop() {
  digitalWrite(LED, 0);
  unsigned long currentMillis = millis();

  // Обновление данных с датчиков и Firebase
  if (currentMillis - previousSensorUpdate >= sensorInterval) {
    previousSensorUpdate = currentMillis;
    
    float h = dht.readHumidity(); //Измеряем влажность
    float t = dht.readTemperature(); //Измеряем температуру
    if (isnan(h) || isnan(t)) {  // Проверка.t Если не удается считать показания, выводится «Ошибка считывания», и программа завершает работу
       Serial.println("Ошибка считывания");
       digitalWrite(LED, 1);
    }else{
      digitalWrite(LED, 0);
    }
    Firebase.setString(fbdo, "/temp/", t);
    Firebase.setString(fbdo, "/humidity/", h);
    Serial.println(t);
    Serial.println(h);
  }

  // Обновление изображения на матрице
  if (currentMillis - previousImageUpdate >= imageInterval) {
    uint8_t activeUsers = 0; // Занимает 1 байт вместо 2 байт (int)
  // Получить количество активных пользователей из Firebase
    if (Firebase.getInt(fbdo, "/users/active_users")) {
      activeUsers = fbdo.intData(); // Преобразуем int в uint8_t
      Serial.print("Активных пользователей: ");
      Serial.println(activeUsers);
      digitalWrite(LED, 0);
    } else {
      Serial.print("Ошибка получения /active_users: ");
      Serial.println(fbdo.errorReason());
      digitalWrite(LED, 1);
    }

    String filename = getFilenameForUsers(activeUsers);
    displayImageFromSD(filename);
  }
}

String getFilenameForUsers(uint8_t activeUsers) { //выбор картинки по кол-во пользователей
  switch (activeUsers) {
    case 0: return "/0.bin";
    case 1: return "/1.bin";
    case 2: return "/2.bin";
    case 3: return "/3.bin";
    case 4: return "/4.bin";
    case 5: return "/5.bin";
    case 6: return "/6.bin";
    case 7: return "/7.bin";
    case 8: return "/8.bin";
    default: return "/6.bin";
  }
}

void displayImageFromSD(const String& filename) {
  File file = SD.open(filename);
  if (!file) {
    Serial.print("Ошибка открытия файла: ");
    Serial.println(filename);
    digitalWrite(LED, 1);
    return;
  }

  if (file.size() != 768) {
    Serial.println("Некорректный формат файла!");
    file.close();
    return;
  }

  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (file.available()) {
        uint8_t r = file.read();
        uint8_t g = file.read();
        uint8_t b = file.read();
        matrix.drawPixel(x, y, matrix.Color(r, g, b));
      }
    }
  }
  
  file.close();
  matrix.show();
  Serial.print("Изображение загружено: ");
  Serial.println(filename);
}
