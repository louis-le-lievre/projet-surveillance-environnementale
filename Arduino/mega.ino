#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <ArduinoJson.h>  


// --- CAPTEURS INTÉRIEURS (GYMNASE) ---
#define DHTPIN 7
#define DHTTYPE DHT11
#define MQ2_PIN A0
#define BRUIT_PIN A8
#define LED_GYMNASE A12


// --- CAPTEURS EXTÉRIEURS ---
#define LUM_PIN A4
#define LED_EXT 12  


// --- SEUILS ---
#define TEMP_SEUIL_INT 35
#define MQ2_SEUIL 1000
#define BRUIT_SEUIL 880
#define TEMP_SEUIL_EXT 30
#define LUM_SEUIL 700  


Adafruit_BMP280 bmp;
DHT dht(DHTPIN, DHTTYPE);


void setup() {
  Serial.begin(115200);   // Debug USB seulement
  Serial1.begin(115200);  // Communication avec NodeMCU
  Wire.begin();


  // Initialisation BMP280
  if (!bmp.begin(0x77)) {
    Serial.println("BMP280 non détecté !");
    while (1);
  }


  // Initialisation DHT11
  dht.begin();
 
  // Configuration des LEDs
  pinMode(LED_GYMNASE, OUTPUT);
  pinMode(LED_EXT, OUTPUT);
 
  digitalWrite(LED_GYMNASE, LOW);
  digitalWrite(LED_EXT, LOW);


  Serial.println("✅ Mega prêt - JSON envoyé sur Serial1");
  delay(1000);
}


void loop() {
  // ---------- LECTURE CAPTEURS ----------
  float tempInt = dht.readTemperature();
  float humInt = dht.readHumidity();
  int airQuality = analogRead(MQ2_PIN);
  int bruit = analogRead(BRUIT_PIN);
  float tempExt = bmp.readTemperature();
  float pression = bmp.readPressure() / 100.0F;
  int lum = analogRead(LUM_PIN);


  // ---------- VÉRIFICATION ----------
  if (isnan(tempInt) || isnan(humInt)) {
    Serial.println("❌ Erreur DHT11 !");
    delay(1000);
    return;
  }


  // ---------- ALERTES ----------
  bool alerteInt = (tempInt > TEMP_SEUIL_INT) || (airQuality > MQ2_SEUIL) || (bruit > BRUIT_SEUIL);
  bool alerteExt = (tempExt > TEMP_SEUIL_EXT) || (lum > LUM_SEUIL);
 
  digitalWrite(LED_GYMNASE, alerteInt ? HIGH : LOW);
  digitalWrite(LED_EXT, alerteExt ? HIGH : LOW);


  // ---------- GÉNÉRATION JSON ----------
  StaticJsonDocument<512> doc;
  doc["DATA-MEGA-1"] = String(tempExt) + " °C";
  doc["DATA-MEGA-2"] = String(lum);
  doc["DATA-MEGA-3"] = String(pression) + " hPa";
  doc["DATA-MEGA-4"] = alerteExt ? "Oui" : "Non";
  doc["DATA-MEGA-5"] = String(bruit);
  doc["DATA-MEGA-6"] = String(tempInt) + " °C et " + String(humInt) + " %";
  doc["DATA-MEGA-7"] = String(airQuality);
  doc["DATA-MEGA-8"] = alerteInt ? "Oui" : "Non";


  String jsonString;
  serializeJson(doc, jsonString);


  // ---------- ENVOI À LA NODE ----------
  // UNIQUEMENT LE JSON, RIEN D'AUTRE !
  Serial1.println(jsonString);
 
  // ---------- DEBUG USB ----------
  Serial.print("📤 JSON envoyé: ");
  Serial.println(jsonString);
 
  if (alerteInt) Serial.println("🚨 ALERTE GYMNASE");
  if (alerteExt) Serial.println("🚨 ALERTE EXTÉRIEUR");
 
  delay(2000);
}
