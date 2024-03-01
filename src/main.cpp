#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>

#define CMPS12_ADDRESS 0x60 // Addresse du CMPS12 shifté à droite de un bit pour la libraire wire
#define ANGLE_8 1			// Registre pour lire l'angle de 8bit

unsigned char high_byte, low_byte, angle8;
char pitch, roll;
unsigned int angle16;
float effect_I = 0;
unsigned long temps_precedent = 0;
unsigned long temps_actuel;
unsigned long diff_temps = 0;
signed int diff_erreur = 0;

const float KI = 0.0000015;
const float KP = 1;
const int OFFSET = 1520;

Servo myservo; // Créer objet servo pour controler le servomoteur 

int servoPin = 13;

int val; // variable pour envoyer position au servomoteur

void setup()
{
	Serial.begin(115200);
	Wire.begin();
	// Allocation de tous les timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);			  // standard 50 hz servo
	myservo.attach(servoPin, 1480, 1520); // attache le servo à la pin 13
}

void loop()
{
	Wire.beginTransmission(CMPS12_ADDRESS); // Commence la communication avec le CMPS12
	Wire.write(ANGLE_8);					// Envoie le registre qu'on veut lire
	Wire.endTransmission();
	Wire.requestFrom(CMPS12_ADDRESS, 5); //Nous donnes les données qu'on veut lire du CMPS12

	while (Wire.available() < 5); // Attend pour que tout les bits soient recus

	angle8 = Wire.read(); // Lire les 5 bits
	high_byte = Wire.read();
	low_byte = Wire.read();
	pitch = Wire.read();
	roll = Wire.read();

	angle16 = high_byte; // Calculer l'angle de 16 bit
	angle16 <<= 8;
	angle16 += low_byte;

	uint16_t compass = angle16 / 10; //Calcul de l'angle sur 360 et afficher sur terminal
	Serial.print("Compass: "); 
	Serial.print(compass, DEC);

	signed int erreur = compass - 180; //Calcul de l'erreur et afficher sur terminal
	Serial.print("    Erreur: "); 
	Serial.print(erreur, DEC);

	temps_actuel = micros(); //Compter les microsecondes pour le calcul de différence de temps

	diff_temps = temps_actuel - temps_precedent; //Calcul et affichage sur terminal de la différence de temps
	Serial.print("    Diff Temps: "); 
	Serial.print(diff_temps, DEC);

	diff_erreur = diff_temps * erreur; //Calcul de la différence d'erreur en multipliant la diff de temps par l'erreur
	Serial.print("    Diff Erreur: "); 
	Serial.print(diff_erreur, DEC); // Affichage sur terminal

	effect_I = effect_I + diff_erreur; //Calcul et affichage sur terminal de l'effet I
	Serial.print("    Effect_I: "); 
	Serial.print(effect_I, DEC);
	if (effect_I > (100 / KI)){ //Si effet I est plus grand que 100
		effect_I = (100/ KI); //Effet I égal 100
	}
	else if (effect_I < (-100 / KI)){ //Si effet I est plus grand que -100
		effect_I = (-100 / KI); //Effet I égal -100
	}
	temps_precedent = temps_actuel; //Temps précedent prends valeur de temps actuel

	signed int effet_p = (erreur * KP); //Calcul de l'effet P
	unsigned int input = effet_p + KI * effect_I + OFFSET; //Calcul de la valeur qu'on envoie au servomoteur
	val = input;
	myservo.write(val); // Position du servomoteur selon la valeur qu'on lui input

	Serial.print("    KI * effect_I: "); //Affichage sur terminal de l'effet I fois KI
	Serial.print(KI * effect_I, DEC);

	Serial.print("    Effect P: "); //Affichage sur terminal de l'effet P
	Serial.print(effet_p, DEC);

	Serial.print("    input: "); //Affichage sur terminal de la valeur qu'on envoie au servomoteur
	Serial.println(input, DEC);
}