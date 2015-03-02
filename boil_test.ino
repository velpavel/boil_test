#include <OneWire.h>

#define BTN_PIN 3 //Пин с кнопкой
#define LED_PIN 13 //Пин со светодиодом 
#define TERMO_PIN 10 //Пин с теромометром
#define RANDOM_PIN A0 //Генератор ПСЧ. Любой аналог_рид пин к которому ничего не подключено.
#define BOIL_TEMP 99 //Температура при которой мы считаем, что вода кипит

OneWire ds(TERMO_PIN);  
unsigned long start_ms=0; //время начала эксперимента
boolean status_run=0; //режим работы. 0 - ожидание, 1 - снятие температуры.
long random_id=0; //случайно сгенерированный id эксперимента
boolean boil=false; //флаг закипания
int temperature=0;
  
void setup() {
  Serial.begin(38400);
  randomSeed(analogRead(RANDOM_PIN)); 
  pinMode(BTN_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
}
 
void loop() {
  
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
    }
    //вот такая некрасивая борьба с дребезгом сейчас
    delay(300);
  }
  if (status_run)
  {
    //инфа о температуре
    GetTemperature();
    
    //Светодиод горит при закипании (по умолчанию от 99 градусов).
    //Мигает с частотой 550мс при температуре ниже 1 градуса.
    //550-50 при 1-100
    //50 при больше 100, если это не закипание : )
    if (boil) digitalWrite(LED_PIN, HIGH);
    else{
      int delta=550;
      if (temperature<1) delta=1050;
      else if (temperature<101) delta=1050-temperature*10;
      else delta=50;
      Blink(delta);
    }
  }
}

void Blink(unsigned long interval){
  static unsigned long prevTime=0;
  if (millis()-prevTime>interval){
    prevTime=millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));    
  }
}

void GetTemperature(){
  static unsigned long prevTime=0;
  static bool start_reed_tempr=false;
  static byte data[2];
  
  //Спросим датчик о температуре
  if (!start_reed_tempr){
    ds.reset(); 
    ds.write(0xCC);
    ds.write(0x44);
    start_reed_tempr=true;
    prevTime=millis();    
  }
  //Даём датчику 750 мс на ответ
  else if (millis()-prevTime>750UL){
    start_reed_tempr=false;
    ds.reset();
    ds.write(0xCC);
    ds.write(0xBE);
    data[0] = ds.read(); 
    data[1] = ds.read();
    temperature = (data[1]<< 8)+data[0];
    temperature = temperature>>4;
    //печать в серийный порт
    PrintTemperature();
  }
}

//Печать всего в серийный порт
void PrintTemperature(){
  Serial.print("RID: ");
  Serial.print(random_id);
  Serial.print(" :MS: ");
  Serial.print(millis()-start_ms);
  Serial.print(" :T: ");
  Serial.print(temperature);
  if (temperature>=BOIL_TEMP && !boil) {Serial.print(" :boil: 1"); boil= true;}
  else if (boil && temperature<BOIL_TEMP) {Serial.print(" :boil: -1"); boil= false;}
  else {Serial.print(" :boil: 0");}
  Serial.println();
}
