# Propositions Créatives

## 1. Éclairage du Vortex
Utilisation de NeoPixels (WS2812B) pour éclairer le vortex.

## 2. Maintenance Prédictive (ML)
Détection de l'usure des roulements via analyse vibratoire.
*Note : Nécessite un accéléromètre externe I2C (ex: MPU6050), car l'ESP32-S3 n'en possède pas en interne.*

## 3. Titration Automatique
Ajout d'une pompe péristaltique pilotée par l'ESP32 pour des ajustements de pH en temps réel.

## 4. Détection Acoustique du Découplage
Utilisation d'un microphone I2S (ex: INMP441) pour détecter le bruit caractéristique du barreau magnétique lorsqu'il décroche ("clattering"). Cela permettrait une sécurité redondante au capteur Hall.

## 5. Analyse de Santé de l'Alimentation
Utilisation des données de l'INA219/INA226 pour détecter une usure des roulements du ventilateur (augmentation progressive de la consommation de base) ou des collisions entre les aimants.

## 6. Interface Tactile (Haptic)
Ajout d'un retour haptique via le buzzer ou un vibreur lors du passage des crans de l'encodeur, permettant un réglage précis sans regarder l'écran.

## 7. Suite d'Automatisation de Laboratoire
Intégration avec **Home Assistant** ou **Node-RED** via MQTT pour programmer des cycles d'agitation complexes (ex: rampes de vitesse sur 24h) synchronisés avec d'autres capteurs du laboratoire.
