#include <Wire.h>//
#include <VL53L0X.h> // Sensore di prossimità
#include "Adafruit_TCS34725.h"

VL53L0X sens_f;//front
VL53L0X sens_l;//left
VL53L0X sens_r;//right

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

#define SHDN_f 9 //sto qui è quello non saldato e bisogna tenerlo xkè faccia contatto bene
#define SHDN_l 11
#define SHDN_r 53

#define mot_fl_1 4 // direzione
#define mot_fl_2 3

#define mot_fr_1 7
#define mot_fr_2 6

#define mot_bl_1 29
#define mot_bl_2 31

#define mot_br_1 33
#define mot_br_2 35

#define en_fl 41 // intensità rotazione
#define en_fr 43
#define en_bl 42
#define en_br 40

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

  pinMode(SHDN_f, OUTPUT);
  pinMode(SHDN_l, OUTPUT);
  pinMode(SHDN_r, OUTPUT);

  
  digitalWrite(SHDN_f, LOW);
  digitalWrite(SHDN_l, LOW);
  digitalWrite(SHDN_r, LOW);
  delay(10);

  setupSensor(sens_f, SHDN_f, 0x30);
  setupSensor(sens_l, SHDN_l, 0x31);
  setupSensor(sens_r, SHDN_r, 0x32);


  /*delay(10);
  digitalWrite(SHDN_f, LOW);
  digitalWrite(SHDN_l, LOW);
  digitalWrite(SHDN_r, LOW);*/

  //setupColor(); //condivide il bus i2c e non si può shitdownarlo
                // bisogna scollegare + e - del light sens x far andare gli altri
                //
                //quindi commmenta la funzione setupColor() se vuoi usare almeno gli altri sens
                //inoltre togli i + e - del sensore di color

  sens_f.startContinuous();
  sens_l.startContinuous();
  sens_r.startContinuous();
  
}

void setWall(int x, int y, int value){
  walls[x][y] = value;
}


void setTile(int x, int y, Tile value){
  tiles[x][y] = value;
}



void loop() {
  bool color = false;
  bool dist = true;

  int dist1 = sens_f.readRangeContinuousMillimeters();
  int dist2 = sens_l.readRangeContinuousMillimeters();
  int dist3 = sens_r.readRangeContinuousMillimeters();
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
