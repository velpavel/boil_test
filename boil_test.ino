#include <OneWire.h>

#define BTN_PIN 3
#define LED_PIN 13
OneWire ds(10);
unsigned long start_ms=0;
boolean status_run=0;
 
void setup() {
  Serial.begin(38400);
  randomSeed(analogRead(0)); 
  pinMode(BTN_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
}
 
void loop() {
  static long random_id=0;
  static boolean boil=false;
  if (digitalRead(BTN_PIN))
  {
    status_run = !status_run;
    if (status_run)
    {
      boil = false;
      start_ms = millis();
      random_id=random(64000);
      Serial.println("START");
    }
    else
    {
      digitalWrite(LED_PIN, LOW);
      Serial.println("STOP");
      delay(1000);
    }
  }
  if (status_run)
  {
    int tempr=ReadTemp();
    Serial.print("RID: ");
    Serial.print(random_id);
    Serial.print(" :MS: ");
    Serial.print(millis()-start_ms);
    Serial.print(" :T: ");
    Serial.print(tempr);
    if (tempr>=99 && !boil) {Serial.print(" :boil: 1"); boil= true;}
    else if (boil && tempr<99) {Serial.print(" :boil: -1"); boil= false;}
    else {Serial.print(" :boil: 0");}
    Serial.println();
    
    //Светодиод
    if (boil) {digitalWrite(LED_PIN, HIGH);}
    else {digitalWrite(LED_PIN, !digitalRead(LED_PIN));}
  }
}

int ReadTemp()
{
  byte data[2];
  ds.reset(); 
  ds.write(0xCC);
  ds.write(0x44);
  delay(750);
  ds.reset();
  ds.write(0xCC);
  ds.write(0xBE);
  data[0] = ds.read(); 
  data[1] = ds.read();
  int Temp = (data[1]<< 8)+data[0];
  Temp = Temp>>4;
  return Temp;
}
