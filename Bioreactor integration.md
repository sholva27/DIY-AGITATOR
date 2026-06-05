# Bioreactor Integration: Stirrer, PI, and FLUOreacteur

Ce document détaille comment intégrer ce mélangeur magnétique (Magnetic Stirrer) avec les projets **Bioreacteur_ESP32_PI** et **NADAH_FLUOreacteur_ESP32**.

## 1. Architecture de Communication

Pour relier ces systèmes, l'ESP32-S3 du mélangeur agit comme un **nœud d'exécution** piloté par le contrôleur principal du bioréacteur.

### Liaison sans fil : ESP-NOW
Idéal pour le **NADAH_FLUOreacteur_ESP32** où l'espace et les câbles sont limités.
- **Protocole :** Envoi de structures `struct_message { int speed; }`.
- **Avantage :** Latence ultra-faible (<1ms) pour ajuster la vitesse en fonction des mesures de fluorescence en temps réel.

### Liaison filaire : Serial1 (GPIO 44/43)
Recommandé pour le **Bioreacteur_ESP32_PI** pour une fiabilité maximale en environnement industriel/laboratoire.
- **Protocole :** Commandes texte simples (ex: `50\n`) envoyées depuis l'ESP32 maître.
- **Usage :** Le contrôleur PI peut ajuster la vitesse de rotation pour optimiser le transfert d'oxygène (kLa) sans interférence WiFi.

---

## 2. Intégration spécifique aux projets

### Avec Bioreacteur_ESP32_PI
- **Asservissement kLa :** Utiliser la vitesse du mélangeur comme variable de sortie du régulateur PI. Si le taux d'oxygène dissous chute, le PI augmente automatiquement le PWM du mélangeur via Serial1.
- **Surveillance :** Le mélangeur renvoie le RPM réel au Bioreacteur_PI pour confirmer que l'agitation est effective.

### Avec NADAH_FLUOreacteur_ESP32
- **Synchronisation Optique :** Stopper ou réduire l'agitation pendant les mesures de fluorescence (si les aimants ou les bulles interfèrent avec le signal optique).
- **Mode Pulsé :** Créer des cycles d'agitation synchronisés avec les flashs d'excitation du fluoréacteur pour homogénéiser l'échantillon entre deux mesures.

---

## 3. Propositions d'Intégrations Futures

### A. Protocole de Mesure Automatisé
Implémenter une séquence "Stir-Wait-Measure" partagée entre les trois systèmes via ESP-NOW :
1. Le **Stirrer** mélange à 80% pendant 10 secondes.
2. Le **Stirrer** s'arrête (Status: IDLE).
3. Le **FLUOreacteur** effectue la mesure optique.
4. Le **Bioreacteur_PI** enregistre la donnée et ajuste la consigne.

### B. Sécurité Centralisée
Si le mélangeur détecte un **Découplage Magnétique** (DECOUPLED) :
- Envoyer immédiatement une alerte "E-STOP" à tous les autres modules.
- Arrêter le chauffage et l'injection de gaz dans le Bioreacteur_PI pour éviter d'endommager la culture.

### C. Dashboard Unifié
Utiliser l'ESP32-S3 du mélangeur (qui possède un écran OLED) comme terminal d'affichage pour les trois projets. Il pourrait afficher :
- Sa propre vitesse.
- Le pH/O2 du Bioreacteur_PI.
- La valeur RFU (Relative Fluorescence Units) du NADAH_FLUOreacteur.
