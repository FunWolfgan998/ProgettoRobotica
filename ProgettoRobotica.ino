#include <Wire.h>//
#include <VL53L0X.h> // Sensore di prossimità

VL53L0X sens_f;//front
VL53L0X sens_l;//left
VL53L0X sens_r;//right

#define SHDN_f 50
#define SHDN_l 48
#define SHDN_r 46

#define mot_fl_1 2 // direzione
#define mot_fl_2 3

#define mot_fr_1 4
#define mot_fr_2 5

#define mot_bl_1 6
#define mot_bl_2 7

#define mot_br_1 8
#define mot_br_2 9

#define en_fl 10 // intensità rotazione
#define en_fr 11
#define en_bl 12
#define en_br 13

void setupSensor (int shutdown, VL53L0X sensor, int address) {
  digitalWrite(shutdown, HIGH);
  delay(10);
  if (!sensor.init()){
    Serial.println("Error sensor: " + address);
    while (1) {}
  }
  sensor.setAddress(address);
}

void setup() {
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

  Serial.begin(9600);
  Wire.begin();
  
  digitalWrite(SHDN_f, LOW);
  digitalWrite(SHDN_l, LOW);
  digitalWrite(SHDN_r, LOW);
  delay(10);

  setupSensor(SHDN_f, sens_f, 0x30);
  setupSensor(SHDN_l, sens_l, 0x30);
  setupSensor(SHDN_r, sens_r, 0x30);

  sens_1.startContinuous();
  sens_2.startContinuous();
  sens_3.startContinuous();
  
}

void loop() {
  int dist1 = sens_1.readRangeContinuousMillimeters();
  int dist2 = sens_2.readRangeContinuousMillimeters();
  int dist3 = sens_3.readRangeContinuousMillimeters();

  analogWrite(en_fl, 255);
  analogWrite(en_fr, 255);
  analogWrite(en_bl, 255);
  analogWrite(en_br, 255);

}
