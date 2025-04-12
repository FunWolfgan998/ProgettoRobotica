/*
 * TODO: DIMINUZIONE TEMPO LETTURA COLORE, DROP, CALIBRAZIONE
*/
#include <Servo.h>
#include <Wire.h>//
#include <VL53L0X.h> // Sensore di prossimità
#include "Adafruit_TCS34725.h"
#include <EEPROM.h>

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_120MS, TCS34725_GAIN_1X);

#define trig_f 23
#define trig_l 25
#define trig_r 27

#define echo_f 22
#define echo_l 24
#define echo_r 26

#define mot_fl_1 5 // direzione
#define mot_fl_2 6

#define mot_fr_1 12
#define mot_fr_2 11

#define mot_bl_1 3
#define mot_bl_2 4

#define mot_br_1 10
#define mot_br_2 9

#define en_fl 7 // intensità rotazione
#define en_fr 13
#define en_bl 2
#define en_br 8

#define ledpin 53
#define servopin 51

#define btn_start 33
#define btn_red 35
#define btn_black 37
#define btn_white 39

struct SingleCalibration {
  uint16_t r, g, b, c, colorTemp, lux;
  SingleCalibration(uint16_t r = 0, uint16_t g = 0, uint16_t b = 0, uint16_t c = 0, uint16_t colorTemp = 0, uint16_t lux = 0)
    : r(r), g(g), b(b), c(c), colorTemp(colorTemp), lux(lux) {}
};

struct Calibration {
  SingleCalibration white;
  SingleCalibration black;
  SingleCalibration red;
  Calibration(SingleCalibration white = {}, SingleCalibration black = {}, SingleCalibration red = {})
    : white(white), black(black), red(red) {}
};

Servo servo;
bool walls[20][20];
int motors[4] = {0, 0, 0, 0};
int motSpeed = 70;
bool ignoreRed = false;
bool ignoreBlack = false;
int lastDistances[10] = {-800, -800, -800, -800, -800, -800, -800, -800, -800, -800};
Calibration calibration;
int numTurnL = 0;
int numTurnR = 0;

void initCalib() {
  /*saveCalib(2, SingleCalibration(117, 132, 1126, 461, 5494, 78));     //white
  saveCalib(1, SingleCalibration(16, 32, 36, 102, 6666, 15));        //black
  saveCalib(0, SingleCalibration(198, 59, 59, 328, 2526, 65522));    //red*/
  Calibration calibration = Calibration(
    SingleCalibration(464, 634, 636, 1898, 6629, 383), //white
    SingleCalibration(29, 37, 40, 113, 6800, 18), //black
    SingleCalibration(245, 93, 105, 438, 3005, 65527) //red
  );
}

void saveCalib(int index, SingleCalibration cfg) {
  int addr = index * sizeof(SingleCalibration); // calcola indirizzo
  EEPROM.put(addr, cfg);
}

SingleCalibration loadCalib(int index) {
  SingleCalibration cfg;
  int addr = index * sizeof(SingleCalibration); // calcola indirizzo
  EEPROM.get(addr, cfg);
  return cfg;
}

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
  
  initCalib();
  calibration.red = loadCalib(0);
  calibration.black = loadCalib(1);
  calibration.white = loadCalib(2);

  /*Serial.println(String(calibration.red.r));

  saveCalib(2, SingleCalibration(117, 132, 1126, 461, 5494, 78));     //white
  saveCalib(1, SingleCalibration(16, 32, 36, 102, 6666, 15));        //black
  saveCalib(0, SingleCalibration(198, 59, 59, 328, 2526, 65522)); 

  calibration.red = loadCalib(0);
  calibration.black = loadCalib(1);
  calibration.white = loadCalib(2);

  Serial.println(String(calibration.red.r));*/
  

  pinMode(btn_start, INPUT);
  pinMode(btn_red, INPUT);
  pinMode(btn_black, INPUT);
  pinMode(btn_white, INPUT);

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
  int n = 4;
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

void DROP () {
  Serial.println("LEDDDDDDDDD / DROPPP");
  delay(100);
  for (int i = 0; i<5; i++) {
    digitalWrite(ledpin, HIGH);
    delay(500);
    digitalWrite(ledpin, LOW);
    delay(500);
  }
  delay(100);
  turnServo();
  delay(100);
}

void printDist(bool dist) {

  if (dist){
    
    int dist1 = readDistance('f');
    int dist2 = readDistance('l');
    int dist3 = readDistance('r');
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
void printHistory() {
  Serial.print(" | "); Serial.print(lastDistances[0]); Serial.print(" "); Serial.print(lastDistances[1]);
  Serial.print(" "); Serial.print(lastDistances[2]);Serial.print(" "); Serial.print(lastDistances[3]);
  Serial.print(" "); Serial.print(lastDistances[4]); Serial.print(" "); Serial.print(lastDistances[5]);
  Serial.println();
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

void turnServo (){
  /*servo.attach(servopin);
  delay(50);
  servo.writeMicroseconds(100);
  delay(948);
  servo.detach();
  delay(50);*/
  Serial.println("DROPPING");
  int n = 9;
  for (int i = 0; i<n; i++){
    Serial.println("DROPPING");
    servo.attach(servopin);
    delay(20);
    servo.writeMicroseconds(100);
    delay(13); //qui
    servo.detach();
    delay(225);
  }
}

void setAllMotors(int v){
  setMotor('f', 'l', v);
  setMotor('f', 'r', v);
  setMotor('b', 'l', v);
  setMotor('b', 'r', v);
}

bool isInRange (int a, int b, float minpercb, float maxpercb){
  return (a > b*minpercb && a < b*maxpercb);
}

bool isCOLOR_white (uint16_t r, uint16_t g, uint16_t b){
  //nota che il R è pre qualche motivo più basso di un 30-40% con colore omogeneo in r g b e invece B è mediamente superiore di 15% ma dato che avviene sia nella calibrazione che a runtime ciò e trasparente (qui)
  return (isInRange(r, calibration.white.r, 0.7, 1.3) && isInRange(g, calibration.white.g, 0.7, 1.3) && isInRange(b, calibration.white.b, 0.7, 1.3));
}

bool isCOLOR_red (uint16_t r, uint16_t g, uint16_t b){
  //(((int)r)-((int)b)) > 130  && (((int)r)-((int)g))> 120
  //Serial.println(isInRange(r, calibration.red.r, 0.6, 1.4) && isInRange(g, calibration.red.g, 0.6, 1.4) && isInRange(b, calibration.red.b, 0.6, 1.4));
  return (isInRange(r, calibration.red.r, 0.6, 1.4) && isInRange(g, calibration.red.g, 0.6, 1.4) && isInRange(b, calibration.red.b, 0.6, 1.4));
}

bool isCOLOR_black (uint16_t r, uint16_t g, uint16_t b){
  //(int)lux < 190
  return (isInRange(r, calibration.black.r, 0.6, 1.4) && isInRange(g, calibration.black.g, 0.6, 1.4) && isInRange(b, calibration.black.b, 0.6, 1.4));
}

void loop() {
  /*while (true){
    Serial.println(String(digitalRead(btn_start)) + " " + String(digitalRead(btn_red)) + " " + String(digitalRead(btn_black)));
  }*/
  /*while (!digitalRead(btn_start)){
    uint16_t r, g, b, c, colorTemp, lux;
    getColor(r, g, b, c, colorTemp, lux, false);
    
    if (digitalRead(btn_red)){
      getColor(r, g, b, c, colorTemp, lux, true);
    } else if (digitalRead(btn_black)) {
      Serial.println("madonna troia");
      turnServo();
    }
    delay(100);
  }*/

  /*while (!digitalRead(btn_start)){
    uint16_t r, g, b, c, colorTemp, lux;
    getColor(r, g, b, c, colorTemp, lux, false);
    
    if (digitalRead(btn_red)){
      Serial.println(isCOLOR_red(r, g, b));
    } else if (digitalRead(btn_black)) {
      Serial.println("madonna troia");
      turnServo();
    }
    delay(100);
  }*/

  Serial.println("***CONFIGURATING***");

  while (!digitalRead(btn_start)){
    uint16_t r, g, b, c, colorTemp, lux;
    
    if (digitalRead(btn_red)){
      Serial.println("***RED CALIBRATION (HOLD FOR 200MS TO COMFIRM)(IF RESULTS APPEAR BELOW, THE CALIBRATION IS DONE - VALUES COULD BE OVERWRITTEN)***");
      delay(100);
      setAllMotors(motSpeed);
      delay(100);
      digitalWrite(ledpin, HIGH);
      while (digitalRead(btn_red)){
        getColor(r, g, b, c, colorTemp, lux, true);
        calibration.red = SingleCalibration(r, g, b, c, colorTemp, lux);
        Serial.println(String(calibration.red.r)); //da 16959
        if (digitalRead(btn_start)) { saveCalib(0, calibration.red); }
        delay(100);
      }
      digitalWrite(ledpin, LOW);
      setAllMotors(0);
    } else if (digitalRead(btn_black)) {
      Serial.println("***BLACK CALIBRATION (HOLD FOR 200MS TO COMFIRM)(IF RESULTS APPEAR BELOW, THE CALIBRATION IS DONE - VALUES COULD BE OVERWRITTEN)***");
      delay(100);
      setAllMotors(motSpeed);
      delay(100);
      digitalWrite(ledpin, HIGH);
      while (digitalRead(btn_black)){
        getColor(r, g, b, c, colorTemp, lux, true);
        calibration.black = SingleCalibration(r, g, b, c, colorTemp, lux);
        if (digitalRead(btn_start)) { saveCalib(1, calibration.black); }
        delay(100);
      }
      digitalWrite(ledpin, LOW);
      setAllMotors(0);
    } else if (digitalRead(btn_white)) {
      Serial.println("***WHITE CALIBRATION (HOLD FOR 200MS TO COMFIRM)(IF RESULTS APPEAR BELOW, THE CALIBRATION IS DONE - VALUES COULD BE OVERWRITTEN)***");
      delay(100);
      setAllMotors(motSpeed);
      delay(100);
      digitalWrite(ledpin, HIGH);
      while (digitalRead(btn_white)){
        getColor(r, g, b, c, colorTemp, lux, true);
        calibration.white = SingleCalibration(r, g, b, c, colorTemp, lux);
        if (digitalRead(btn_start)) { saveCalib(2, calibration.white); }
        delay(100);
      }
      digitalWrite(ledpin, LOW);
      setAllMotors(0);
    }
    delay(100);
  }

  delay(100);
  Serial.println("***STARTED***");
  setDefaultMotors();
  Serial.println(String(calibration.red.r));

  /*while (true) {
    //printDist (true);
    uint16_t r, g, b, c, colorTemp, lux;
    getColor(r, g, b, c, colorTemp, lux, true);
    Serial.println(isCOLOR_red(r,g,b));
  }*/

  /*while (true) {
    printDist (true);
    
  }*/

  while (true) {
    uint16_t r, g, b, c, colorTemp, lux;
    getColor(r, g, b, c, colorTemp, lux, true);
    printHistory();
    int avgdistf = avgDistance('f');
    
    if (isCOLOR_red(r, g, b) && !ignoreRed) { //ROSSO
      Serial.println("ROSSSOOOOOO");
      setAllMotors(0);
      ignoreRed = true;
      delay(100);
      setAllMotors(100);      delay(350);
      setAllMotors(0);
      delay(50);
      DROP();
      setDefaultMotors();
      avgdistf = avgDistance('f');
    }
    if (isCOLOR_black(r, g, b) && !ignoreBlack){
      Serial.println("NEROOOOO");
      setAllMotors(0);
      ignoreBlack = true;
      delay(100);
      setMotor('f', 'l', 100);
      setMotor('f', 'r', -100);
      setMotor('b', 'l', 100);
      setMotor('b', 'r', -100);
      delay(2300);
      setDefaultMotors();
      avgdistf = avgDistance('f');
    }
    if (isCOLOR_white(r, g, b)){
      ignoreRed = false;
      ignoreBlack = false;
      avgdistf = avgDistance('f');
    }
    bool correttivo = avgDifference(avgdistf) < 6;
    if ((avgdistf < 11 || avgdistf > 400 || correttivo) && true) { //qui curva e correttivo
      if (correttivo) {
        Serial.println("CORRETTIVOOO");
      } else {
          Serial.println("CURVAAAAA " + String(avgdistf));
      }
      setAllMotors(0);
      delay(100);
      setAllMotors(50);
      delay(1000); //while (avgDistance('f') > 2){ printData(false, true); delay(30); }
      setAllMotors(-50);
      delay(500);
      setAllMotors(0);
      delay(100);

      bool dir = avgDistance('r') > avgDistance('l');

      if (dir){
        if (numTurnL == 0){
          numTurnR += 1;
        } else {
          numTurnR = 1;
          numTurnL = 0;
        }
      } else {
        if (numTurnR == 0){
          numTurnL += 1;
        } else {
          numTurnL = 1;
          numTurnR = 0;
        }
      }

      if ((numTurnR == 4 || numTurnL == 4) && false){ // && true/false per far attivare o no
        dir = !dir;
        Serial.println("INVERTIII");
      }
      
      setMotor('f', 'l', 100 * (dir ? 1 : -1));
      setMotor('f', 'r', 100 * (dir ? -1 : 1));
      setMotor('b', 'l', 100 * (dir ? 1 : -1));
      setMotor('b', 'r', 100 * (dir ? -1 : 1));
      delay(1100); //vecchio: 1800, altro 1300
      setAllMotors(0);
      delay(100);
      setDefaultMotors();
      avgdistf = avgDistance('f');
    }
    updateDistances(avgdistf);
    delay(1);
  }
}
