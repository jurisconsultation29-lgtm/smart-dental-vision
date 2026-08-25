#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

// --- DÉFINITION DES BROCHES ---
const int PIN_BUZZER         = 26;  // Buzzer
const int PIN_CAPTEUR_SALIVE = 36;  // Capteur de salive (GPIO 36 / VP)
const int PIN_INFRARED       = 4;  // Capteur Infrarouge (GPIO 34)
const int PIN_LED_1          = 33;  // LED Alerte
const int PIN_LED_2          = 32;  // LED Normal

Adafruit_BME280 bme;

unsigned long dernierEnvoi = 0;
const long intervalleEnvoi = 2000; // Envoi toutes les 2 secondes

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialisation du bus I2C pour le BME280 (SDA=21, SCL=22)
  Wire.begin(21, 22);

  // Configuration des broches entrées/sorties
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  pinMode(PIN_INFRARED, INPUT);

  // Initialisation du capteur BME280 (test des deux adresses i2c courantes: 0x76 ou 0x77)
  if (!bme.begin(0x76)) {
    if (!bme.begin(0x77)) {
      Serial.println("❌ BME280 introuvable ! Vérifie le câblage I2C.");
    }
  }

  Serial.println("✅ Système SmartDental initialisé avec succès !");
}

void loop() {
  // Gestion de l'intervalle d'envoi (non bloquant)
  if (millis() - dernierEnvoi >= intervalleEnvoi) {
    dernierEnvoi = millis();

    // 1. Lecture de la température du BME280
    float temp = bme.readTemperature();
    if (isnan(temp)) temp = 0.0;

    // 2. Lecture et conversion de la salive (GPIO 36)
    int brutSalive = analogRead(PIN_CAPTEUR_SALIVE);
    int tauxSalive = map(brutSalive, 4095, 0, 0, 100);
    if (tauxSalive < 0) tauxSalive = 0;
    if (tauxSalive > 100) tauxSalive = 100;

    // 3. Lecture de l'infrarouge avec inversion logique (GPIO 34)
    int etatInfrarouge = !digitalRead(PIN_INFRARED); 

    // 4. Gestion de la logique d'alerte (Buzzer et LEDs)
    // Seuil d'alerte : si salive > 30% OU si l'infrarouge détecte un obstacle (1)
    if (tauxSalive > 30 || etatInfrarouge == 1) {
      digitalWrite(PIN_LED_1, HIGH);  // LED Alerte ON
      digitalWrite(PIN_LED_2, LOW);   // LED Normal OFF
      digitalWrite(PIN_BUZZER, HIGH); // Buzzer ON
    } else {
      digitalWrite(PIN_LED_1, LOW);   // LED Alerte OFF
      digitalWrite(PIN_LED_2, HIGH);  // LED Normal ON
      digitalWrite(PIN_BUZZER, LOW);  // Buzzer OFF
    }

    // 5. Affichage des données au format JSON dans le Moniteur Série
    Serial.print("{\"temp\":");
    Serial.print(temp);
    Serial.print(",\"salive\":");
    Serial.print(tauxSalive);
    Serial.print(",\"ir\":");
    Serial.print(etatInfrarouge);
    Serial.println("}");
  }
}