//Программа для измерения тепературы. Данные передаются в Serial.
//
//!!!Внимание!!! используется EEPROM 411, 412, 413, 414, 415

#include <OneWire.h>    //Для термометра
#include <EEPROM.h>
#include <Wire.h>       //Для часов
#include <TimeLib.h>    //Для часов 
#include <DS1307RTC.h>  //Для часов
#include <SPI.h>        //Для карты памяти
#include <SD.h>         //Для карты памяти

#define CHECH_EEPROM_ADRESS 411 //адрес EEPROM в по которому должно хранится значение, указывающие на то, что все нужные значения в EEPROM заполнены
#define CHECH_EEPROM_VALUE 166 //Значение, которые должно быть по адресу CHECH_EEPROM_ADRESS
#define DEVICE_ID_EEPROM 412 //адрес EEPROM с ID данного измерительного устройства
#define EXPERIMENT_ID_EEPROM_HIGH 413 //адрес EEPROM со старшим байтом ID эксперимента
#define EXPERIMENT_ID_EEPROM_LOW 414 //адрес EEPROM с младшим байтом ID эксперимента
//#define BOIL_TEMP_EEPROM 415 //в будущем тут будет хранится температура закипания

#define BTN_PIN 5 //Пин с кнопкой. Старт-Стоп.
#define LED_PIN A1 //Пин со светодиодом. Используется как индикатор температуры воды.
#define BUZZER_PIN 3 //Пин с пищалкой. Используется как сигнал закипания. Её функция Tone блокирует ШИМ на 3 и 11 пинах
//Пищалка блокирует ШИМ на 3ем и на 11 портах
#define TERMO_PIN 9 //Пин с теромометром. На 10м пине не работает из-за конфликта с SD картой
#define RANDOM_PIN A0 //Генератор ПСЧ. Любой аналог_рид пин к которому ничего не подключено.
//! A4 -SDA часы
//! A5 -SCL часы
#define SD_CS_PIN 10
//! 11 - SD MOSI
//! 12 - SD MISO
//! 13 - SD CLK/SCK

#define BOIL_TEMP 97 //Температура при которой мы считаем, что вода кипит.
#define HELP_STRING "s - start/stop; l - display last results; h - help; d - remove sum file; ! - remove detail file"

OneWire ds(TERMO_PIN);
unsigned long start_ms = 0; //время начала эксперимента
boolean status_run = 0; //режим работы. 0 - ожидание, 1 - снятие температуры.
boolean boil = false; //флаг закипания
int temperature = 0;
int device_id;
unsigned int exper_id;

File myFile;  //объект для файла на SD карте.
#define temper_file_name "temper.csv"
#define temper_sum_file_name "tsum.txt"
boolean sd_ok=false;

void setup() {  
  Serial.begin(9600);
  randomSeed(analogRead(RANDOM_PIN));
  pinMode(BTN_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  if (EEPROM.read(CHECH_EEPROM_ADRESS) == CHECH_EEPROM_VALUE) {
    device_id = EEPROM.read(DEVICE_ID_EEPROM);
    exper_id = EEPROM.read(EXPERIMENT_ID_EEPROM_HIGH) * 255 + EEPROM.read(EXPERIMENT_ID_EEPROM_LOW);

    Serial.println("EEPROM values were read");
  }
  else {
    device_id = random(255);
    EEPROM.write(DEVICE_ID_EEPROM, device_id);
    exper_id = 0;
    EEPROM.write(EXPERIMENT_ID_EEPROM_HIGH, 0);
    EEPROM.write(EXPERIMENT_ID_EEPROM_LOW, 0);
    EEPROM.write(CHECH_EEPROM_ADRESS, CHECH_EEPROM_VALUE);

    Serial.println("EEPROM values were NULL. Generated new.");
  }
  //Проверим наличие карты памяти.
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD init failed");
    sd_ok=false;
    tone(BUZZER_PIN, 3000, 200);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(200);
    tone(BUZZER_PIN, 3000, 200);
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
  }
  else{sd_ok=true; Serial.println("SD init ok");}
  //проверим/создадим файл для температуры
  if (sd_ok){
    if (!SD.exists(temper_file_name)){
      //temper_file_name
      Sd_print("Date;RID;MS;Temperature;boil", temper_file_name);
      Serial.println("Created new file");
    }
  }
  Serial.println(HELP_STRING);
  Serial.println("Ready!");
}

void loop() {
  //Сменим режим работы, если нажата кнопка
  char incomingByte='_';
  bool need_change_mode = false;
  if (Serial.available() > 0){
    incomingByte = Serial.read();
    if (incomingByte == 's' || incomingByte == 's' ) { need_change_mode=true; }
    if (incomingByte == 'h' || incomingByte == 'H' || incomingByte == '?' ) { Serial.println(HELP_STRING); }
    if (incomingByte == 'l' || incomingByte == 'L' ) { SD_read_last_sum(); }
    if (incomingByte == 'd' || incomingByte == 'D' ) { SD.remove(temper_sum_file_name); Serial.println("Sum file removed"); }
    if (incomingByte == '!') { SD.remove(temper_file_name); Serial.println("Detail file removed"); }
  }
  if (BtnPressed(BTN_PIN) || need_change_mode)
  {
    status_run = !status_run;
    if (status_run)
    {
      boil = false;
      start_ms = millis();                                                                                                                                                                                                                                                             
      exper_id += 1;
      WriteExperID(exper_id);
      Serial.println("START");
    }
    else
    {
      Serial.println("STOP");
      digitalWrite(LED_PIN, LOW);
      GetTemperature(true);
    }
  }
  if (status_run)
  {
    //инфа о температуре
    GetTemperature(false);

    //Светодиод горит при закипании (по умолчанию от 99 градусов).
    //Мигает с частотой 550мс при температуре ниже 1 градуса.
    //550-50 при 1-100
    //50 при больше 100, если это не закипание : )
    if (boil) digitalWrite(LED_PIN, HIGH);
    else {
      int delta = 550;
      if (temperature < 1) delta = 1050;
      else if (temperature < 101) delta = 1050 - temperature * 10;
      else delta = 50;
      Blink(delta);
    }
  }
}

void Blink(unsigned long interval) {
  static unsigned long prevTime = 0;
  if (millis() - prevTime > interval) {
    prevTime = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}

void GetTemperature(boolean finish) {
  static unsigned long prevTime = 0;
  static bool start_reed_tempr = false;
  static byte data[2];
  

  if (finish) {
    start_reed_tempr = false;
    return;
  }

  //Спросим датчик о температуре
  if (!start_reed_tempr) {
    ds.reset();
    ds.write(0xCC);
    ds.write(0x44);
    start_reed_tempr = true;
    prevTime = millis();
  }
  //Даём датчику 750 мс на ответ
  else if (millis() - prevTime > 750UL) {
    start_reed_tempr = false;
    ds.reset();
    ds.write(0xCC);
    ds.write(0xBE);
    data[0] = ds.read();
    data[1] = ds.read();
    temperature = (data[1] << 8) + data[0];
    temperature = temperature >> 4;
    //печать в серийный порт и на SD карту
    PrintTemperature();
  }
}

//Печать всего в серийный порт
void PrintTemperature() {
  String print_serial="";
  String print_SD=printdate()+";";
  print_serial+="RID: "+String(device_id)+"_"+String(exper_id);
  print_SD+=String(device_id)+"_"+String(exper_id)+";";
  unsigned long boil_time=millis() - start_ms;
  print_serial+=" :MS: "+String(boil_time);
  print_SD+=String(boil_time)+";";
  print_serial+=" :T: "+String(temperature);
  print_SD+=String(temperature)+";";
  
  if (temperature >= BOIL_TEMP && !boil) {
    print_serial+=" :boil: 1";
    print_SD+="1"; 
    boil = true;
    tone(BUZZER_PIN, 4000, 1000);
    String sd_sum_str = printdate()+" "+String(exper_id)+" "+String(boil_time/1000/60)+":"+String(boil_time/1000%60);
    Sd_print(sd_sum_str, temper_sum_file_name);
  }
  else if (boil && temperature < BOIL_TEMP) {
    print_serial+=" :boil: -1";
    print_SD+="-1"; 
    boil = false;
  }
  else {
    print_serial+=" :boil: 0";
    print_SD+="0"; 
  }
  Serial.println(print_serial);
  Sd_print(print_SD, temper_file_name);
}

void Sd_print(String str, String file_path){
  if (sd_ok){
    myFile=SD.open(file_path, FILE_WRITE);
    myFile.println(str);
    myFile.close();
  }
}

void SD_read_last_sum(){
  if (sd_ok){
    if (SD.exists(temper_sum_file_name)){
      myFile = SD.open(temper_sum_file_name);
      while (myFile.available()) {
        Serial.write(myFile.read());
      }
      myFile.close();
    }
    else {Serial.println("Sum file doesn't exist.");}
  }
  else {Serial.println("SD init failed");}
}

String printdate(){
  tmElements_t tm;
  RTC.read(tm);
  String date_str="";
  date_str+=String(tmYearToCalendar(tm.Year));
  date_str+=String(tm.Month/10);
  date_str+=String(tm.Month%10);
  date_str+=String(tm.Day/10);
  date_str+=String(tm.Day%10);
  date_str+=String(" ");
  date_str+=String(tm.Hour/10);
  date_str+=String(tm.Hour%10);
  date_str+=String(":");
  date_str+=String(tm.Minute/10);
  date_str+=String(tm.Minute%10);
  date_str+=String(":");
  date_str+=String(tm.Second/10);
  date_str+=String(tm.Second%10);

  return date_str;
}

//Чтение нажатия кнопки
boolean BtnPressed(byte btn_pin) {
  static boolean last_btn_status = LOW;
  boolean btn_status = digitalRead(btn_pin);
  boolean result = false;
  //борьба с дребезгом
  if (btn_status != last_btn_status) {
    delay(5);
    btn_status = digitalRead(btn_pin);
  }
  if (last_btn_status == LOW && btn_status == HIGH) result = true;
  last_btn_status = btn_status;
  return result;
}

//Запись номера эксперимента в EEPROM
void WriteExperID(int exper_id) {
  if (EEPROM.read(EXPERIMENT_ID_EEPROM_LOW) != exper_id % 255) EEPROM.write(EXPERIMENT_ID_EEPROM_LOW, exper_id % 255);
  if (EEPROM.read(EXPERIMENT_ID_EEPROM_HIGH) != exper_id / 255) EEPROM.write(EXPERIMENT_ID_EEPROM_HIGH, exper_id / 255);
}
