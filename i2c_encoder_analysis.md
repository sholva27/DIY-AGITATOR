# Analyse : Module Codeur Rotatif IIC (I2C)

Vous avez demandé si le module **EC11 Codeur Rotatif IIC** est intéressant pour ce projet. Voici une analyse technique.

## 1. Qu'est-ce que c'est ?
Contrairement à un encodeur EC11 standard qui nécessite 3 broches GPIO (CLK, DT, SW) et des interruptions constantes sur l'ESP32, la version **IIC (I2C)** possède son propre petit microcontrôleur intégré (souvent un ATtiny ou un circuit dédié). Il gère la lecture des impulsions et communique la valeur finale via le bus I2C.

## 2. Avantages pour le Stirrer
- **Économie de Pins :** Il se branche sur les mêmes fils que l'écran OLED (SDA/SCL). Vous libérez ainsi 3 GPIO sur votre ESP32-S3.
- **Réduction de la charge CPU :** L'ESP32 n'a plus besoin de gérer les interruptions rapides de l'encodeur. Il lit simplement une valeur numérique quand il en a besoin.
- **Immunité au bruit :** Les encodeurs mécaniques souffrent souvent de "rebonds" (bruit électrique). Le module I2C filtre généralement ces rebonds de manière logicielle en interne, rendant le réglage de la vitesse beaucoup plus fluide.

## 3. Inconvénients
- **Complexité logicielle :** Nécessite une bibliothèque spécifique (ex: `I2C_Rotary_Encoder`) au lieu de simples `digitalRead`.
- **Vitesse de réponse :** Si le bus I2C est très chargé (ex: mise à jour constante de l'OLED), il peut y avoir une légère latence perçue lors d'une rotation très rapide.

## 4. Verdict
**C'est une amélioration très intéressante, surtout si vous prévoyez d'ajouter d'autres capteurs (température, courant) qui consomment des pins GPIO.**

C'est un choix "pro" qui simplifie le câblage (Daisy-chain avec l'écran OLED). Si vous en trouvez un chez **DZDUINO** ou **SESDZ**, c'est une excellente option pour le raffinement hardware.
