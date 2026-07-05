# Liste de Courses (Shopping List) - Algérie 🇩🇿

Voici une sélection de composants pour le projet Magnetic Stirrer, vérifiés sur **dzduino.com** et **sesdz.com**.

## 1. Microcontrôleur (ESP32-S3)
Le S3 est idéal pour le PID haute fréquence.
- **DZDUINO :** [Carte de développement ESP32-S3 avec écran LCD tactile](https://www.dzduino.com/carte-de-developpement-esp32-s3-avec-ecran-lcd-tactile-rond-de-1-28-pouce-taille-compacte-accelerometre-et-capteur-gyroscopique) (En stock) - *Note: Inclut déjà un écran.*
- **DZDUINO :** [ESP32 OLED 0.96 Pouce + Batterie](https://www.dzduino.com/esp32-oled-0-96-pouce-carte-de-developpement-batterie) (En stock) - *Excellent choix "tout-en-un" car l'écran OLED est déjà soudé.*

## 2. Affichage (OLED 0.96" I2C)
- **DZDUINO :** [Module OLED 0.96 inch 128X64](https://www.dzduino.com/module-daffichage-oled-0-96-inch-4pin-128x64-jaune-bleu) (Vérifier stock régulièrement).

## 3. Capteur de Courant (Optionnel - Pour le Log)
- **DZDUINO :** [Module INA219](https://www.dzduino.com/ina219-module-de-capteur-de-courant) (Vérifier stock).
- *Pro Alternative :* Rechercher **INA226** (16-bit) pour une précision accrue sur les courants faibles (<200mA).
- *Important :* Privilégier un ventilateur PC **4-fils (PWM)** pour une mesure de courant stable côté 12V.

## 4. Alimentation (Step-Down / Buck Converter)
Il faut abaisser le 12V du ventilateur vers le 5V de l'ESP32.
- **DZDUINO :** [Module Step Down 6-24V vers USB/5V](https://www.dzduino.com/module-2-usb-dc-dc-step-down-6-24v) (En stock).
- **DZDUINO :** [LM2596S DC-DC Buck Converter](https://www.dzduino.com/dc-dc-converter-module-dalimentation-réglable-step-down-module-fr) (Vérifier stock).
- **Alternative Ultra-Compacte :** **Module S09 (Buck-Boost)**.
    - *Avantages :* Très petite taille (19x14mm), régulation automatique (accepte 3-15V pour sortir 5V stable).
    - *Attention :* Courant max de **600mA**. Suffisant pour l'ESP32 et l'OLED, mais trop faible pour alimenter le ventilateur lui-même (qui doit rester en direct sur le 12V).

## 5. Contrôle de Température (Bioréacteur)
- **SESDZ :** [Contrôleur STC-3028](https://www.sesdz.com/regulateur-temperature-humidite-incubateur-STC-3028) (En stock) - *Pour réguler la température globale du bioreacteur.*
- **SESDZ :** [Sonde DS18B20](https://www.sesdz.com/Régulateur-controleur-température/Mesure) (En stock).

## 6. Accessoires & Outillage
- **SESDZ :** [Pince à dénuder GSFixtop](https://www.sesdz.com/pince-a-denuder-les-fils-1-1-6-2-2-6-3-2mm-175-x-130-mm-gsfixtop-10602) (En stock).
- **DZDUINO :** Pour les aimants néodyme et les ventilateurs PC (4-fils recommandés), ces pièces sont souvent disponibles chez les vendeurs de matériel informatique ou de réparation d'imprimantes 3D.

**Conseil :** Les stocks varient très vite sur ces sites. N'hésitez pas à les appeler directement pour confirmer les arrivages.
