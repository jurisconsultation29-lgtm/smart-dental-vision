#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

// --- DÉFINITION DES BROCHES ---
const int PIN_BUZZER         = 26;  
const int PIN_CAPTEUR_SALIVE = 36;  // VP / GPIO 36
const int PIN_INFRARED       = 4;   // GPIO 4 (selon la sérigraphie du module)
const int PIN_LED_1          = 33;  
const int PIN_LED_2          = 32;  

Adafruit_BME280 bme;

unsigned long dernierEnvoi = 0;
const long intervalleEnvoi = 2000; // Envoi toutes les 2 secondes

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialisation du bus I2C pour le BME280 (SDA=21, SCL=22)
  Wire.begin(21, 22);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  
  // Configuration de l'infrarouge en entrée avec résistance de rappel
  pinMode(PIN_INFRARED, INPUT_PULLUP); 

  // Initialisation du capteur BME280
  if (!bme.begin(0x76)) {
    bme.begin(0x77);
  }

  // S'assurer que le buzzer est éteint au démarrage
  digitalWrite(PIN_BUZZER, LOW);

  Serial.println("✅ Système Smart Dental / Santé démarré !");
}

void loop() {
  // Exécution à intervalles réguliers (toutes les 2 secondes)
  if (millis() - dernierEnvoi >= intervalleEnvoi) {
    dernierEnvoi = millis();

    // 1. Lecture de la température
    float temp = bme.readTemperature();
    if (isnan(temp)) temp = 0.0;

    // 2. Lecture et conversion du capteur de salive (0 à 100%)
    int brutSalive = analogRead(PIN_CAPTEUR_SALIVE);
    int tauxSalive = map(brutSalive, 4095, 0, 0, 100);
    if (tauxSalive < 0) tauxSalive = 0;
    if (tauxSalive > 100) tauxSalive = 100;

    // 3. Lecture de l'Infrarouge
    int etatIR = digitalRead(PIN_INFRARED);

    // --- 4. DIAGNOSTIC & GESTION DES ANOMALIES ---
    String diagnosticHaleine = "Sain";
    bool alerteAnomalie = false;

    // Exemple de règles métiers pour les anomalies :
    // - Température anormale (> 37.5°C) ET taux de salive très bas (< 20%) -> Risque pathologique / Halitose sévère
    if (temp > 37.5 && tauxSalive < 20) {
        diagnosticHaleine = "Risque Halitose / Sécheresse sévère";
        alerteAnomalie = true;
    } 
    else if (tauxSalive < 20) {
        diagnosticHaleine = "Sécheresse buccale (Attention)";
        // Pas forcément d'alerte sonore continue, juste un avertissement textuel, ou tu peux activer alerteAnomalie si tu le souhaites
    } 
    else if (temp > 37.5) {
        diagnosticHaleine = "Suspicion inflammation";
        alerteAnomalie = true;
    }

    // --- 5. GESTION DU BUZZER ---
    if (alerteAnomalie) {
        digitalWrite(PIN_BUZZER, HIGH); // Le buzzer sonne SEULEMENT s'il y a une anomalie
        digitalWrite(PIN_LED_1, HIGH);  // Allume une LED d'alerte
    } else {
        digitalWrite(PIN_BUZZER, LOW);  // Silencieux le reste du temps
        digitalWrite(PIN_LED_1, LOW);   // Éteint la LED d'alerte
    }

    // --- 6. AFFICHAGE JSON POUR TON APPLICATION ---
    Serial.print("{\"temp\":");
    Serial.print(temp);
    Serial.print(",\"salive\":");
    Serial.print(tauxSalive);
    Serial.print(",\"ir\":");
    Serial.print(etatIR);
    Serial.print(",\"diagnostic\":\"");
    Serial.print(diagnosticHaleine);
    Serial.println("\"}");
  }
}