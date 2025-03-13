#include <Wire.h>//
#include <VL53L0X.h> // Sensore di prossimità
#include "Adafruit_TCS34725.h"

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

#define trig_f 22
#define trig_l 24
#define trig_r 26

#define echo_f 23
#define echo_l 25
#define echo_r 27

#define mot_fl_1 10 // direzione
#define mot_fl_2 9

#define mot_fr_1 11
#define mot_fr_2 12

#define mot_bl_1 3
#define mot_bl_2 4

#define mot_br_1 6
#define mot_br_2 5

#define en_fl 8 // intensità rotazione
#define en_fr 13
#define en_bl 2
#define en_br 7

enum Tile {
  normal,
  victim,
  empty,
  black
};

Tile tiles[20][20];
bool walls[20][20];


void setupSensor(VL53L0X &sensor, int shutdownPin, int address) {
  pinMode(shutdownPin, OUTPUT);
  digitalWrite(shutdownPin, LOW);  // Spegni il sensore
  delay(10);
  digitalWrite(shutdownPin, HIGH); // Riaccendi il sensore
  delay(10);

  if (!sensor.init()) {
    Serial.print("\nErrore inizializzazione sensore all'indirizzo 0x");
    Serial.println(address, HEX);
    while (1) {}
  }

  sensor.setAddress(address); // Imposta un indirizzo univoco
  sensor.startContinuous();   // Avvia la lettura continua
}

void setupColor(){
  if (tcs.begin()) {
    Serial.println("\nFound color sensor");
  } else {
    Serial.println("\nNo TCS34725 found ... check your connections");
    while (1);
  }
}


void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(mot_fl_1, OUTPUT);
  pinMode(mot_fl_2, OUTPUT);
  pinMode(mot_fr_1, OUTPUT);
  pinMode(mot_fr_2, OUTPUT);
  pinMode(mot_bl_1, OUTPUT);
  pinMode(mot_bl_2, OUTPUT);
  pinMode(mot_br_1, OUTPUT);
  pinMode(mot_br_2, OUTPUT);
  
  pinMode(en_fl, OUTPUT);
  pinMode(en_fr, OUTPUT);
  pinMode(en_bl, OUTPUT);
  pinMode(en_br, OUTPUT);

  pinMode(trig_f, OUTPUT);
  pinMode(trig_l, OUTPUT);
  pinMode(trig_r, OUTPUT);

  
  pinMode(echo_f, INPUT);
  pinMode(echo_l, INPUT);
  pinMode(echo_r, INPUT);

  delay(10);



  setupColor(); //condivide il bus i2c e non si può shitdownarlo
                // bisogna scollegare + e - del light sens x far andare gli altri
                //
                //quindi commmenta la funzione setupColor() se vuoi usare almeno gli altri sens
                //inoltre togli i + e - del sensore di color

  
}

void setWall(int x, int y, int value){
  walls[x][y] = value;
}


void setTile(int x, int y, Tile value){
  tiles[x][y] = value;
}


/*int readDistance (char c) {
  int trigPin = c=='f' ? trig_f : c=='l' ? trig_l : trig_r;
  int echoPin = c=='f' ? echo_f : c=='l' ? echo_l : echo_r;
  if (c != 'f' && c != 'l' && c != 'r') { Serial.println("Sensore scorretto"); }
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  int duration = pulseIn(echoPin, HIGH);
  int distance = (duration*.0343)/2;
  return distance;
}*/

int readDistance(char c) {
  int trigPin = c == 'f' ? trig_f : c == 'l' ? trig_l : trig_r;
  int echoPin = c == 'f' ? echo_f : c == 'l' ? echo_l : echo_r;
  if (c != 'f' && c != 'l' && c != 'r') {
    Serial.println("Sensore scorretto");
    return -1; // Aggiungi un ritorno di errore se il sensore è sbagliato
  }

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  int duration = pulseIn(echoPin, HIGH);
  if (duration == 0) { // Se non c'è eco, ritorna un errore
    return -1;
  }

  int distance = (duration * 0.0343) / 2;
  if (distance < 0 || distance > 4000) {  // Verifica distanze plausibili
    return -1; // Aggiungi una condizione di errore
  }

  return distance;
}


void loop() {
  bool color = false;
  bool dist = true;

  int dist1 = readDistance('f');
  int dist2 = readDistance('l');
  int dist3 = readDistance('r');
  
  uint16_t r, g, b, c, colorTemp, lux;
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature_dn40(r, g, b, c);
  lux = tcs.calculateLux(r, g, b);

  if (color){
      Serial.print("Color Temp: "); Serial.print(colorTemp, DEC); Serial.print(" K - ");
      Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" - ");
      Serial.print("R: "); Serial.print(r, DEC); Serial.print(" ");
      Serial.print("G: "); Serial.print(g, DEC); Serial.print(" ");
      Serial.print("B: "); Serial.print(b, DEC); Serial.print(" ");
      Serial.print("C: "); Serial.print(c, DEC); Serial.print(" ");
      Serial.println(" ");
  }
  if (dist){
    Serial.print("Sensore 1: ");
    Serial.print(dist1);
    Serial.print(" mm\t");

    Serial.print("Sensore 2: ");
    Serial.print(dist2);
    Serial.print(" mm\t");

    Serial.print("Sensore 3: ");
    Serial.print(dist3);
    Serial.println(" mm");
  }
  
  analogWrite(en_fl, 255);
  analogWrite(en_fr, 255);
  analogWrite(en_bl, 255);
  analogWrite(en_br, 255);

  digitalWrite(mot_fl_1, HIGH);
  digitalWrite(mot_fl_2, LOW);
  digitalWrite(mot_fr_1, HIGH);
  digitalWrite(mot_fr_2, LOW);
  digitalWrite(mot_bl_1, HIGH);
  digitalWrite(mot_bl_2, LOW);
  digitalWrite(mot_br_1, HIGH);
  digitalWrite(mot_br_2, LOW);

  delay(100);
}
