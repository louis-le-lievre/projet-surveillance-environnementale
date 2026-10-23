// Programme à téléverser dans le NODE (V.5.1)
// Si Arduino, choisir carte : NodeMCU 1.0


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>


//---- Point d'accès disponible 1
#define AP1_SSID "iPhoneQ"
#define AP1_PASS "griezmann91"


//---- Point d'accès disponible 2
#define AP2_SSID "Tel2"
#define AP2_PASS "123456"


//---- Point d'accès disponible 3
#define AP3_SSID "Personne2"
#define AP3_PASS "aaaaaaaa"


//===== REGLAGES_02 : Vitesse moniteur série
uint32_t speedSerial    = 115200;


//===== REGLAGES_03 : littleFs (permet d'héberger les fichiers)
#define ENABLE_LITTLE_FS


//===== Réglage de la fréquence de rafraîchissement (pas trop vite SVP)
unsigned long timerDelay = 2000; // Fréquence de rafraîchissement imposée par le Node


//========== OBJETS ET VARIABLES
//----- Création d'un objet server sur le port 80 pour répondre à la requête http://192.168.x.x
AsyncWebServer server(80);


//----- Création d'un objet Websocket
AsyncWebSocket ws("/ws");


//----- Création d'un objet WifiMulti pour gérer plusieurs point d'accès
ESP8266WiFiMulti wifiMulti;




//========== BUFFER et autres
//----- Chaînes JSON
String jsonStringMega    = "";  // Contiendra la chaîne JSON reçue de la MEGA
String jsonStringAll     = "";  // Contiendra la chaîne JSON complète envoyée à la page web


//======== FACILITATEURS (ne pas changer SVP)
// MACRO : Limiteur de fréquence
#define LIMIT_FREQ_BEGIN(varName, duration) static uint32_t varName = 0; if(millis()>varName+duration)
#define LIMIT_FREQ_END(varName) varName = millis()


// MACRO : Limiteur de répétition
#define LIMIT_REP_BEGIN(varName, nbrOfRep) static uint8_t varName = 0; if(varName<nbrOfRep)
#define LIMIT_REP_END(varName) varName++


// MACRO : Setup virtuel
#define SETUP_VIR_BEGIN(varName) static uint8_t varName = 0; if(varName<1)
#define SETUP_VIR_END(varName) varName++


//========== PROTOTYPES DE FONCTIONS (pour que VSCode soit contant)
String  getSensorReadings();
String  getSensorReadingsMega();
void    initWifiMulti();
void    notifyClients(String sensorReadings);
void    handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void    onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void    initWebSocket();
void    initFS();
void    manageMdns();




//======================================== SETUP() =======================================
void setup() {
  //---------- Initialisation des liaisons séries
  Serial.begin(speedSerial); while (!Serial); // Utiliser pour débogage & réception données de la MEGA
  Serial.println(); Serial.println("Serial:ok");


  //---------- Initialisation du wifi-multi (nouvelle méthode)
  wifiMulti.addAP(AP1_SSID, AP1_PASS);
  wifiMulti.addAP(AP2_SSID, AP2_PASS);
  wifiMulti.addAP(AP3_SSID, AP3_PASS);
  initWifiMulti();


  //---------- Initialisation du websocket
  initWebSocket();


  //---------- Initialisation du SPIFF (appelé maintenant Little FS sur les ESP8266)
#if defined ENABLE_LITTLE_FS
  initFS();


  //---- Initialisation du serveur
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");
  server.begin();
#endif
}


//======================================== LOOP() ========================================
void loop() {


  //========== OPTION : MDNS : ne fonctionne pas toujours - (il faut un navigateur compatible hélas)
#if defined ENABLE_MDNS
  manageMdns();
#endif


  //========== Récupération des données de la MEGA (capteurs distants)
  jsonStringMega = checkAndReadsMegaData();


  //========== Envoie les données complètes à la page web
  LIMIT_FREQ_BEGIN(sendingWebData, timerDelay) {
    String sensorReadings = getSensorReadings();
    Serial.print("NODE>");
    //sensorReadings = Serial.readStringUntil('\n');
    Serial.println(sensorReadings);
    notifyClients(sensorReadings);
    LIMIT_FREQ_END(sendingWebData);
  }


  //========== Gestion des clients
  ws.cleanupClients();


}


String getSensorReadings() {
  JSONVar jsonObjectAll;
 
  // DEBUG: Affiche ce qu'on reçoit
  Serial.print("jsonStringMega = ");
  Serial.println(jsonStringMega);
 
  // Vérifie si la chaîne n'est pas vide
  if (jsonStringMega.length() == 0 || jsonStringMega == "{}") {
    Serial.println("Aucune donnée Mega !");
    return "{}";
  }
 
  // Essaie de parser le JSON
  JSONVar jsonObjectMega = JSON.parse(jsonStringMega);
 
  // Vérifie si le parsing a réussi
  if (JSON.typeof(jsonObjectMega) == "undefined") {
    Serial.println("JSON invalide !");
    return "{}";
  }
 
  // Copie les données (avec vérification)
  if (JSON.typeof(jsonObjectMega["DATA-MEGA-1"]) != "undefined") {
    jsonObjectAll["DATA-MEGA-DISTANT-1"] = String((const char*)jsonObjectMega["DATA-MEGA-1"]);
  } else {
    jsonObjectAll["DATA-MEGA-DISTANT-1"] = "";
  }
 
  if (JSON.typeof(jsonObjectMega["DATA-MEGA-2"]) != "undefined") {
    jsonObjectAll["DATA-MEGA-DISTANT-2"] = String((const char*)jsonObjectMega["DATA-MEGA-2"]);
  } else {
    jsonObjectAll["DATA-MEGA-DISTANT-2"] = "";
  }
 
  if (JSON.typeof(jsonObjectMega["DATA-MEGA-3"]) != "undefined") {
    jsonObjectAll["DATA-MEGA-DISTANT-3"] = String((const char*)jsonObjectMega["DATA-MEGA-3"]);
  } else {
    jsonObjectAll["DATA-MEGA-DISTANT-3"] = "";
  }
 
  if (JSON.typeof(jsonObjectMega["DATA-MEGA-4"]) != "undefined") {
    jsonObjectAll["DATA-MEGA-DISTANT-4"] = String((const char*)jsonObjectMega["DATA-MEGA-4"]);
  } else {
    jsonObjectAll["DATA-MEGA-DISTANT-4"] = "";
  }
 
  if (JSON.typeof(jsonObjectMega["DATA-MEGA-5"]) != "undefined") {
    jsonObjectAll["DATA-MEGA-DISTANT-5"] = String((const char*)jsonObjectMega["DATA-MEGA-5"]);
  } else {
    jsonObjectAll["DATA-MEGA-DISTANT-5"] = "";
  }
 
  if (JSON.typeof(jsonObjectMega["DATA-MEGA-6"]) != "undefined") {
    jsonObjectAll["DATA-MEGA-DISTANT-6"] = String((const char*)jsonObjectMega["DATA-MEGA-6"]);
  } else {
    jsonObjectAll["DATA-MEGA-DISTANT-6"] = "";
  }
 
  if (JSON.typeof(jsonObjectMega["DATA-MEGA-7"]) != "undefined") {
    jsonObjectAll["DATA-MEGA-DISTANT-7"] = String((const char*)jsonObjectMega["DATA-MEGA-7"]);
  } else {
    jsonObjectAll["DATA-MEGA-DISTANT-7"] = "";
  }
 
  if (JSON.typeof(jsonObjectMega["DATA-MEGA-8"]) != "undefined") {
    jsonObjectAll["DATA-MEGA-DISTANT-8"] = String((const char*)jsonObjectMega["DATA-MEGA-8"]);
  } else {
    jsonObjectAll["DATA-MEGA-DISTANT-8"] = "";
  }
 
  String result = JSON.stringify(jsonObjectAll);
  Serial.print("NODE>");
  Serial.println(result);
 
  return result;
}


void initWifiMulti() {
  Serial.println("Tentative de connexion à un point d'accès disponible... ");


tryWifi:
  if (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
    goto tryWifi;
  }
  Serial.println('\n');
  Serial.print("Connecté à:\t\t\t");
  Serial.println(WiFi.SSID());
  Serial.print("Adresse IP:\t\t\t");
  Serial.println(WiFi.localIP());
  Serial.print("Force du signal(dBm):\t");
  Serial.println(WiFi.RSSI());
  Serial.print("Adresse MAC du Node:\t");
  Serial.println(WiFi.macAddress());
}


void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    //data[len] = 0;
    //String message = (char*)data;
    // Check if the message is "getReadings"
    //if (strcmp((char*)data, "getReadings") == 0) {
    //if it is, send current sensor readings
    String sensorReadings = getSensorReadings();
    //Serial.print(sensorReadings);
    notifyClients(sensorReadings);
    //}
  }
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}


void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


String checkAndReadsMegaData() {
  static String buffer = "";
  static String lastValidJson = "{}";
 
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
   
    // DEBUG: tout afficher
    Serial.print("📥 RAW: ");
    Serial.println(line);
   
    // FILTRE INTELLIGENT: cherche un JSON contenant "DATA-MEGA-1"
    if (line.indexOf("\"DATA-MEGA-1\"") != -1) {
      // Trouvé ! Extrait juste la partie JSON
      int start = line.indexOf('{');
      int end = line.lastIndexOf('}');
     
      if (start != -1 && end != -1 && end > start) {
        String jsonOnly = line.substring(start, end + 1);
        Serial.print("✅ MEGA>");
        Serial.println(jsonOnly);
        lastValidJson = jsonOnly;
        return jsonOnly;
      }
    }
  }
 
  // Si pas de nouvelles données, retourne le dernier JSON valide
  // Mais expire après 10 secondes
  static unsigned long lastValidTime = 0;
  static bool hasValidData = false;
 
  if (lastValidJson != "{}") {
    if (!hasValidData) {
      lastValidTime = millis();
      hasValidData = true;
    }
   
    // Garde les données valides pendant 10 secondes max
    if (millis() - lastValidTime < 10000) {
      return lastValidJson;
    }
  }
 
  // Timeout ou pas de données
  hasValidData = false;
  return "{}";
}


//============================== OPION : LITTLE-FS (gestion des fichiers web)
#if defined ENABLE_LITTLE_FS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS a rencontré un problème");
  }
  else {
    Serial.println("LittleFS est fonctionnel");
  }
}
#endif


//============================== OPTION : MDNS (uniquement utile si vous voulez accéder à la page par http://nomMdns.local)
#if defined ENABLE_MDNS
#include <ESP8266mDNS.h>
void manageMdns() {
  const char* hostName  = NAME_MDNS;
  SETUP_VIR_BEGIN(mDns) {
    if (MDNS.begin(hostName)) {
      Serial.print("mDNS est actif. ");
      Serial.print("Votre NODE répondra à http://");
      Serial.print(hostName);
      Serial.println(".local (si navigateur compatible)");
      //MDNS.addService("http", "tcp", mdnsPort);
    } else {
      Serial.println("Erreur lors de l'activation de mDNS.");
    }
    SETUP_VIR_END(mDns);
  }
  MDNS.update();
}
#endif
