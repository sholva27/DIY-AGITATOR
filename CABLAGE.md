# Manuel de Câblage (Wiring Guide)

Ce document détaille le raccordement électrique pour le Magnetic Stirrer sur base ESP32-S3.

## 1. Schéma des Signaux

Le câblage des entrées/sorties logiques doit respecter les broches suivantes :

| Composant | Broche ESP32-S3 | Type | Note |
| :--- | :--- | :--- | :--- |
| **Lien Bioréacteur** | GPIO 4 (RX), 5 (TX) | UART | Liaison Serial1 |
| **Capteur Hall** | GPIO 6 | Entrée | Mesure directe du barreau |
| **Kill Switch** | GPIO 7 | Sortie | Sécurité MOSFET |
| **Écran OLED** | GPIO 8 (SDA), 9 (SCL)| I2C | Adresse 0x3C |
| **Encodeur EC11** | GPIO 10, 11, 12 | Entrées | CLK, DT, SW |
| **Ventilateur (Tacho)**| GPIO 13 | Entrée | **Pull-up 10k vers 3.3V requis** |
| **Ventilateur (PWM)** | GPIO 14 | Sortie | 25kHz / 3.3V |
| **Buzzer Actif** | GPIO 15 | Sortie | <20mA (sinon transistor) |
| **LED RGB** | GPIO 16, 17, 18 | Sorties | **Résistances 220Ω requises** |

### Précautions Signaux :
- **LED RGB :** Intercaler des résistances de 220 à 330 Ω sur chaque couleur (Rouge, Vert, Bleu).
- **Tacho :** Le signal tachymétrique des ventilateurs est souvent un collecteur ouvert. Une résistance de 10kΩ entre la broche GPIO 13 et le 3.3V de l'ESP32 est impérative.
- **Buzzer :** Direct sur GPIO 15 si buzzer piezo basse consommation. Pour un buzzer électromagnétique, utiliser un transistor (2N2222).

## 2. Alimentation et Masse (GND)

Le système utilise une alimentation principale de 12V DC.

### Architecture de puissance :
1.  **12V Direct :** Alimente le ventilateur PC.
2.  **Buck Converter (LM2596) :** Abaisse le 12V vers 5V pour alimenter l'ESP32-S3 (via broche 5V/VIN).
3.  **Régulateur interne ESP32 :** Fournit le 3.3V pour tous les modules (OLED, Encodeur, LED, Capteurs).

### Masse Commune (GND) :
Il est **impératif** que toutes les masses soient reliées ensemble :
- GND de l'alimentation 12V
- GND du Buck Converter
- GND de l'ESP32-S3
- GND du ventilateur et de tous les modules esclaves.

---
*Note : Le signal PWM 3.3V de l'ESP32 est directement compatible avec la plupart des ventilateurs PC 4-fils (norme Intel).*
