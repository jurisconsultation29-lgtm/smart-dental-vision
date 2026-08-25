#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

// --- DÉFINITION DES BROCHES (Vérification et intégration du GPIO 5) ---
const int PIN_BUZZER         = 26;
const int PIN_CAPTEUR_SALIVE = 36; // VP / GPIO 36 (Analogique)
const int PIN_INFRARED       = 4;  // GPIO 4 (Numérique)
const int PIN_LED_1          = 33; // LED Verte (Prêt)
const int PIN_LED_2          = 32; // LED Rouge (Alerte)
const int PIN_CONTROLE_5     = 5;  // GPIO 5 (Intégré pour contrôle / module additionnel)

Adafruit_BME280 bme;

unsigned long dernierEnvoi = 0;
const long intervalleEnvoi = 2000; // Envoi toutes les 2 secondes

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialisation du bus I2C (SDA=21, SCL=22)
  Wire.begin(21, 22);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  pinMode(PIN_CONTROLE_5, OUTPUT);
  pinMode(PIN_INFRARED, INPUT_PULLUP);

  // Initialisation BME280
  if (!bme.begin(0x76)) {
    bme.begin(0x77);
  }

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_1, LOW);
  digitalWrite(PIN_LED_2, LOW);
  digitalWrite(PIN_CONTROLE_5, LOW);

  Serial.println("✅ Système Smart Dental / Contrôle ESP32 démarré (GPIO 5 inclus) !");
}

void loop() {
  // --- A. ÉCOUTE DES ORDRES DE L'APPLICATION WEB ---
  if (Serial.available() > 0) {
    char commande = Serial.read();
    
    switch (commande) {
      case 'V': // Vert / Prêt
        digitalWrite(PIN_LED_1, HIGH);
        digitalWrite(PIN_LED_2, LOW);
        digitalWrite(PIN_BUZZER, LOW);
        digitalWrite(PIN_CONTROLE_5, LOW);
        break;
      case 'R': // Alerte Rouge + Buzzer + Activation GPIO 5
        digitalWrite(PIN_LED_1, LOW);
        digitalWrite(PIN_LED_2, HIGH);
        digitalWrite(PIN_BUZZER, HIGH);
        digitalWrite(PIN_CONTROLE_5, HIGH);
        break;
      case '0': // Tout éteindre / Reset
        digitalWrite(PIN_LED_1, LOW);
        digitalWrite(PIN_LED_2, LOW);
        digitalWrite(PIN_BUZZER, LOW);
        digitalWrite(PIN_CONTROLE_5, LOW);
        break;
    }
  }

  // --- B. ENVOI DE LA TÉLÉMÉTRIE Périodique ---
  if (millis() - dernierEnvoi >= intervalleEnvoi) {
    dernierEnvoi = millis();

    float temp = bme.readTemperature();
    if (isnan(temp)) temp = 0.0;

    int brutSalive = analogRead(PIN_CAPTEUR_SALIVE);
    int tauxSalive = map(brutSalive, 4095, 0, 0, 100);
    if (tauxSalive < 0) tauxSalive = 0;
    if (tauxSalive > 100) tauxSalive = 100;

    int etatIR = digitalRead(PIN_INFRARED);
    int etatGPIO5 = digitalRead(PIN_CONTROLE_5);

    // Diagnostic local
    String diagnosticHaleine = "Sain";
    bool alerteAnomalie = false;

    if (temp > 37.5 || tauxSalive < 20) {
      diagnosticHaleine = "Risque Halitose / Sécheresse critique";
      alerteAnomalie = true;
    }

    // Gestion physique immédiate de l'alerte sur le kit (GPIO 5 activé sur alerte)
    if (alerteAnomalie) {
      digitalWrite(PIN_LED_2, HIGH);
      digitalWrite(PIN_BUZZER, HIGH);
      digitalWrite(PIN_CONTROLE_5, HIGH);
    } else {
      digitalWrite(PIN_LED_2, LOW);
      digitalWrite(PIN_BUZZER, LOW);
      digitalWrite(PIN_CONTROLE_5, LOW);
    }

    // Flux JSON envoyé vers le Web Serial (avec l'état du GPIO 5)
    Serial.print("{\"temp\":");
    Serial.print(temp);
    Serial.print(",\"salive\":");
    Serial.print(tauxSalive);
    Serial.print(",\"ir\":");
    Serial.print(etatIR);
    Serial.print(",\"gpio5\":");
    Serial.print(etatGPIO5);
    Serial.print(",\"diagnostic\":\"");
    Serial.print(diagnosticHaleine);
    Serial.println("\"}");
  }
}