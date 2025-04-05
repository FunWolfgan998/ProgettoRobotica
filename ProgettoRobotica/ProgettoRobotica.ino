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

#define ledpin 29

enum Tile {
  normal,
  victim,
  empty,
  black
};

Tile tiles[20][20];
bool walls[20][20];
int motors[4] = {0, 0, 0, 0};
int motSpeed = 70;
bool ignoreRed = false;
bool ignoreBlack = false;
int lastDistances[6] = {-800, -800, -800, -800, -800, -800};

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


int findMotorIndex (char fb, char lr) {
  int index = -1;
  if (lr == 'l' && fb == 'f'){
    index = 0;
  } else if (lr == 'r' && fb == 'f') {
    index = 1;
  } else if (lr == 'l' && fb == 'b') {
    index = 2;
  } else if (lr == 'r' && fb == 'b') {
    index = 3;
  }
  return index;
}

void findMotorPins(int target[3], char fb, char lr) {
  int enable;
  int pin1;
  int pin2;
  if (lr == 'l' && fb == 'f') {
    enable = en_fl;
    pin1 = mot_fl_1;
    pin2 = mot_fl_2;
  } else if (lr == 'r' && fb == 'f') {
    enable = en_fr;
    pin1 = mot_fr_1;
    pin2 = mot_fr_2;
  } else if (lr == 'l' && fb == 'b') {
    enable = en_bl;
    pin1 = mot_bl_1;
    pin2 = mot_bl_2;
  } else if (lr == 'r' && fb == 'b') {
    enable = en_br;
    pin1 = mot_br_1;
    pin2 = mot_br_2;
  } else {
    Serial.println("Motore errato");
    return;
  }
  target[0] = enable;
  target[1] = pin1;
  target[2] = pin2;
}


void setMotor (char fb, char lr, int intensity) {
  int index = findMotorIndex(fb, lr);
  int pins[3];
  findMotorPins(pins, fb, lr);
  if (intensity == motors[index]){
    return;
  } else if ((intensity < 0 && motors[index] < 0) || (intensity > 0 && motors[index] > 0)) {
    analogWrite(pins[0], abs(intensity));
  } else {
    if (intensity >= 0) {
      digitalWrite(pins[1], HIGH);
      digitalWrite(pins[2], LOW);
    } else {
      digitalWrite(pins[1], LOW);
      digitalWrite(pins[2], HIGH);
    }
    analogWrite(pins[0], abs(intensity));
  }
  motors[index] = intensity;
}

int readDistance (char c) {
  int _trig = c == 'f' ? trig_f : c == 'l' ? trig_l : trig_r;
  int _echo = c == 'f' ? echo_f : c == 'l' ? echo_l : echo_r;
  if (c != 'f' && c != 'l' && c != 'r') {
    Serial.println("Sensore scorretto");
    return -404; // Aggiungi un ritorno di errore se il sensore è sbagliato
  }
  int distance=0;
  long duration=0;
  digitalWrite (_trig, LOW);
  delayMicroseconds (2);
  digitalWrite(_trig, HIGH);
  delayMicroseconds (10) ;
  digitalWrite (_trig, LOW);
  duration = pulseIn (_echo, 1);
  distance = ((duration+10) *0.0343)/2;
  return distance;
}

int avgDistance (char c) {
  int n = 5;
  int sum = 0;
  for (int i = 0; i<n; i++) {
    sum += readDistance(c);
  }
  return sum/n;
}

double avgDifference(int value) {
  int size = sizeof(lastDistances)/sizeof(int);
  double sum = 0;
  for (int i = 0; i < size; i++) {
    sum += abs(lastDistances[i] - value);
  }
  return sum * 1.0 / (size);
}

void updateDistances(int newDistance) {
  int size = sizeof(lastDistances)/sizeof(int);
  for (int i = size-1; i > 0; i--) {
    lastDistances[i] = lastDistances[i - 1];
  }
  lastDistances[0] = newDistance;
}

void LED () {
  Serial.println("LEDDDDDDDDD");
  delay(200);
  for (int i = 0; i<6; i++) {
    digitalWrite(ledpin, HIGH);
    delay(500);
    digitalWrite(ledpin, LOW);
    delay(500);
  }
  delay(200);
}

void printDist(bool dist) {
  int dist1 = readDistance('f');
  int dist2 = readDistance('l');
  int dist3 = readDistance('r');

  if (dist){
    Serial.print("Sensore 1: ");
    Serial.print(dist1);
    Serial.print(" cm\t");

    Serial.print("Sensore 2: ");
    Serial.print(dist2);
    Serial.print(" cm\t");

    Serial.print("Sensore 3: ");
    Serial.print(dist3);
    Serial.print(" cm");

    Serial.print(" | "); Serial.print(lastDistances[0]); Serial.print(" "); Serial.print(lastDistances[1]);
    Serial.print(" "); Serial.print(lastDistances[2]);Serial.print(" "); Serial.print(lastDistances[3]);
    Serial.print(" "); Serial.print(lastDistances[4]); Serial.print(" "); Serial.print(lastDistances[5]);
    Serial.println();
  }
}

void getColor(uint16_t &r, uint16_t &g, uint16_t &b, uint16_t &c, uint16_t &colorTemp, uint16_t &lux) {
    getColor(r, g, b, c, colorTemp, lux, false);
}

void getColor(uint16_t &r, uint16_t &g, uint16_t &b, uint16_t &c, uint16_t &colorTemp, uint16_t &lux, bool print) {
    tcs.getRawData(&r, &g, &b, &c);
    colorTemp = tcs.calculateColorTemperature_dn40(r, g, b, c);
    lux = tcs.calculateLux(r, g, b);

    if (print) {
        Serial.print("Color Temp: "); Serial.print(colorTemp, DEC); Serial.print(" K - ");
        Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" - ");
        Serial.print("R: "); Serial.print(r, DEC); Serial.print(" ");
        Serial.print("G: "); Serial.print(g, DEC); Serial.print(" ");
        Serial.print("B: "); Serial.print(b, DEC); Serial.print(" ");
        Serial.print("C: "); Serial.print(c, DEC); Serial.print(" ");
        Serial.println();
    }
}

void setDefaultMotors(){
  setMotor('f', 'l', motSpeed);
  setMotor('f', 'r', motSpeed);
  setMotor('b', 'l', motSpeed);
  setMotor('b', 'r', motSpeed);
}


void loop() {
  setDefaultMotors();

  while (true) {
    printDist (false);
    uint16_t r, g, b, c, colorTemp, lux;
    getColor(r, g, b, c, colorTemp, lux, true);
    
    if ((((int)r)-((int)b)) > 130  && (((int)r)-((int)g))> 120 && !ignoreRed) { //ROSSO
      setMotor('f', 'l', 0);
      setMotor('f', 'r', 0);
      setMotor('b', 'l', 0);
      setMotor('b', 'r', 0);
      ignoreRed = true;
      delay(100);
      setMotor('f', 'l', -100);
      setMotor('f', 'r', -100);
      setMotor('b', 'l', -100);
      setMotor('b', 'r', -10);
      delay(600);
      setMotor('f', 'l', 0);
      setMotor('f', 'r', 0);
      setMotor('b', 'l', 0);
      setMotor('b', 'r', 0);
      delay(50);
      LED();
      setDefaultMotors();
    }
    if ((int)lux < 190 && !ignoreBlack){
      Serial.println("NEROOOOO");
      setMotor('f', 'l', 0);
      setMotor('f', 'r', 0);
      setMotor('b', 'l', 0);
      setMotor('b', 'r', 0);
      ignoreBlack = true;
      delay(200);
      setMotor('f', 'l', 100);
      setMotor('f', 'r', -100);
      setMotor('b', 'l', 100);
      setMotor('b', 'r', -100);
      delay(2300);
      setDefaultMotors();
    }
    if (r>1800 && g>3500 && b>3500){
      ignoreRed = false;
      ignoreBlack = false;
    }
    if (avgDistance('f') < 15 || avgDistance('f') > 400 || avgDifference(avgDistance('f')) < 4) {
      if (avgDifference(avgDistance('f')) < 4) { Serial.println("CORRETTIVOOO"); } else { Serial.println("CURVAAAAA"); }
      setMotor('f', 'l', 0);
      setMotor('f', 'r', 0);
      setMotor('b', 'l', 0);
      setMotor('b', 'r', 0);
      delay(500);
      setMotor('f', 'l', 50);
      setMotor('f', 'r', 50);
      setMotor('b', 'l', 50);
      setMotor('b', 'r', 50);
      delay(1000); //while (avgDistance('f') > 2){ printData(false, true); delay(30); }
      setMotor('f', 'l', -50);
      setMotor('f', 'r', -50);
      setMotor('b', 'l', -50);
      setMotor('b', 'r', -50);
      delay(500);
      setMotor('f', 'l', 0);
      setMotor('f', 'r', 0);
      setMotor('b', 'l', 0);
      setMotor('b', 'r', 0);
      delay(500);
      bool dir = avgDistance('r') > avgDistance('l');
      setMotor('f', 'l', 100 * (dir ? 1 : -1));
      setMotor('f', 'r', 100 * (dir ? -1 : 1));
      setMotor('b', 'l', 100 * (dir ? 1 : -1));
      setMotor('b', 'r', 100 * (dir ? -1 : 1));
      delay(1100); //vecchio: 1800, altro 1300
      setMotor('f', 'l', 0);
      setMotor('f', 'r', 0);
      setMotor('b', 'l', 0);
      setMotor('b', 'r', 0);
      delay(500);
      setDefaultMotors();
    }
    updateDistances(avgDistance('f'));
    delay(1);
  }
  delay(5000);
}
