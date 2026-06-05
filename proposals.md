# Propositions d'Amélioration (Project Improvements)

Voici plusieurs axes d'amélioration pour transformer ce mélangeur magnétique DIY en un outil de laboratoire professionnel.

## 1. Contrôle en Boucle Fermée (PID Control)
Actuellement, le code envoie un signal PWM constant.
- **Amélioration :** Implémenter un régulateur PID (Proportionnel, Intégral, Dérivé).
- **Avantage :** Le mélangeur maintiendra exactement le même RPM même si la viscosité du liquide change (ex: ajout de polymères ou changement de température).

## 2. Détection de Découplage Magnétique (IMPLÉMENTÉ)
Le "saut" du barreau magnétique est un problème courant.
- **Fonctionnalité :** Le code surveille désormais le RPM par rapport à la consigne. Si le RPM est nul alors que la puissance est élevée (>30%), le système passe en mode `DECOUPLED`.
- **Action :** Le ventilateur s'arrête par sécurité. L'utilisateur peut relancer manuellement.
- **Soft Start :** Une rampe progressive a été ajoutée pour éviter le décrochage lors des accélérations brusques.

## 3. Interface Web et Monitoring (IoT)
L'ESP32-S3 possède le WiFi.
- **Amélioration :** Créer un Dashboard web (via WebSerial ou une interface HTML/JavaScript stockée sur l'ESP32).
- **Avantage :** Visualiser les courbes de vitesse en temps réel sur un ordinateur ou smartphone et contrôler plusieurs mélangeurs simultanément.

## 4. Intégration de Capteurs Additionnels
- **Température (DS18B20) :** Placer une sonde dans le bioreacteur pour afficher la température sur l'écran OLED en plus de la vitesse.
- **Capteur de Courant (INA219) :** Mesurer la consommation du ventilateur pour détecter une surcharge mécanique ou un blocage. Le INA219 est préférable à l'ACS712 pour sa précision sur les faibles courants et son immunité aux champs magnétiques des aimants.

## 5. Améliorations Matérielles (Hardware)
- **Régulateur de tension à faible bruit :** Utiliser des condensateurs de filtrage plus importants pour éviter que le bruit du moteur ne perturbe l'écran OLED.
- **Boîtier Étanche :** Concevoir un boîtier imprimé en 3D avec des joints en silicone pour protéger l'électronique des éclaboussures de produits chimiques.

## 6. Modes de Mélange Avancés
- **Mode Pulsé :** Alterner entre haute et basse vitesse pour créer des turbulences spécifiques.
- **Minuteur avec Alarme (IMPLÉMENTÉ) :** Programmer une durée de mélange via l'encodeur (appui court pour basculer en mode Timer). Un buzzer (GPIO 15) retentit à la fin du compte à rebours ou en cas de découplage.
