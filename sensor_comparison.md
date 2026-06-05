# Comparaison des Capteurs de Courant : ACS712 vs INA219

Pour le projet de mélangeur magnétique (contrôle de ventilateur PC), voici une analyse comparative entre le module **ACS712 (30A)** et le **INA219**.

## 1. ACS712 (Capteur à Effet Hall)
- **Principe :** Mesure le champ magnétique généré par le courant traversant une piste interne.
- **Interface :** Sortie Analogique (Tension).
- **Avantages :**
    - Isolation galvanique totale entre le circuit de puissance (12V fan) et le circuit de commande (ESP32).
    - Très simple à câbler.
- **Inconvénients :**
    - **Précision médiocre pour les faibles courants :** Le modèle 30A a une sensibilité de 66mV/A. Un ventilateur de PC consomme environ 0.2A à 0.5A, ce qui ne produit qu'une variation de 13mV à 33mV. C'est très difficile à mesurer précisément avec l'ADC de l'ESP32 qui est assez bruité.
    - Sensible aux champs magnétiques externes (problématique à côté des aimants du mélangeur !).
    - Sortie 5V (nécessite un pont diviseur pour l'entrée 3.3V de l'ESP32).

## 2. INA219 (Capteur Shunt I2C)
- **Principe :** Mesure la chute de tension aux bornes d'une résistance de précision (shunt).
- **Interface :** I2C (Numérique).
- **Avantages :**
    - **Haute Précision :** Peut mesurer de très faibles courants avec une excellente résolution.
    - **Mesure de Tension :** Mesure aussi la tension du bus (VCC du ventilateur).
    - **Numérique :** Pas de bruit ADC, communication directe en I2C avec l'ESP32.
    - **Paramétrable :** On peut modifier le gain pour s'adapter précisément à la consommation du ventilateur.
- **Inconvénients :**
    - Pas d'isolation galvanique (masse commune nécessaire).
    - Un peu plus complexe à configurer (bibliothèque logicielle requise).

## Verdict pour le Projet
**Le gagnant est le INA219.**

**Pourquoi ?**
Le ventilateur de PC consomme trop peu de courant pour que l'ACS712 (surtout en version 30A) soit efficace. L'ACS712 30A est conçu pour de gros moteurs ou des chargeurs de batterie. De plus, la proximité des aimants du mélangeur fausserait les mesures de l'ACS712 (effet Hall).

Le INA219 vous permettra de détecter si le ventilateur est bloqué ou si le barreau magnétique force trop, même avec une consommation de seulement 150mA.
