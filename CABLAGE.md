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
- **LED RGB :** Intercaler des résistances de 220 à 330 Ω sur chaque couleur.
- **Tacho :** Résistance de 10kΩ entre GPIO 13 et le 3.3V de l'ESP32 est impérative.

## 2. Alimentation et Masse (GND)

Le système utilise une alimentation **Regulée 12V DC (Switching)**.

### Architecture de puissance :
1.  **12V via Kill Switch (MOSFET) :** Alimente le ventilateur PC. Cette ligne est physiquement coupée en mode OFF pour assurer l'arrêt total.
2.  **Buck Converter (LM2596) :** Abaisse le 12V permanent vers 5V pour l'ESP32-S3.
3.  **Masse Commune (GND) :** Toutes les masses (Alim 12V, Buck, ESP32, Modules) doivent être interconnectées.

### Note sur le ventilateur 4 fils :
Sur un ventilateur 4 fils, le 12V reste normalement présent en permanence et le contrôle se fait par le signal PWM. Cependant, beaucoup de ventilateurs conservent un régime minimal même à 0% de PWM. Le **Kill Switch** (piloté par le GPIO 7) permet de couper totalement l'alimentation 12V du ventilateur, garantissant un arrêt complet (0 RPM) et servant de sécurité en cas de perte du signal de commande.

### Dimensionnement de l'alimentation :
- **Logique :** L'ESP32-S3 et les modules consomment environ 0.3A en pic (Comms WiFi).
- **Ventilateur :** Vérifiez l'étiquette (ex: 0.20A). Prévoyez une marge de 1.5x pour l'appel de courant au démarrage.
- **Verdict :** Un adaptateur **12V / 2A (24W)** est recommandé pour la plupart des configurations. Un adaptateur de 1A ne suffira que pour les ventilateurs très basse consommation.

---
**Attention :** Un adaptateur sous-dimensionné ou non-régulé provoquera des "brown-outs" (chutes de tension) au démarrage du moteur, faisant redémarrer l'ESP32 en boucle.
