//Программа для измерения тепературы. Данные передаются в Serial.
//
//!!!Внимание!!! используется EEPROM 411, 412, 413, 414, 415

#include <OneWire.h>
#include <EEPROM.h>

#define CHECH_EEPROM_ADRESS 411 //адрес EEPROM в по которому должно хранится значение, указывающие на то, что все нужные значения в EEPROM заполнены
#define CHECH_EEPROM_VALUE 166 //Значение, которые должно быть по адресу CHECH_EEPROM_ADRESS
#define DEVICE_ID_EEPROM 412 //адрес EEPROM с ID данного измерительного устройства
#define EXPERIMENT_ID_EEPROM_HIGH 413 //адрес EEPROM со старшим байтом ID эксперимента
#define EXPERIMENT_ID_EEPROM_LOW 414 //адрес EEPROM с младшим байтом ID эксперимента
//#define BOIL_TEMP_EEPROM 415 //в будущем тут будет хранится температура закипания

#define BTN_PIN 4 //Пин с кнопкой. Старт-Стоп.
#define LED_PIN 13 //Пин со светодиодом. Используется как индикатор температуры воды.
#define BUZZER_PIN 3 //Пин с пищалкой. Используется как сигнал закипания. Её функция Tone блокирует ШИМ на 3 и 11 пинах
//Пищалка блокирует ШИМ на 3ем и на 11 портах
#define TERMO_PIN 10 //Пин с теромометром.
#define RANDOM_PIN A0 //Генератор ПСЧ. Любой аналог_рид пин к которому ничего не подключено.

#define BOIL_TEMP 97 //Температура при которой мы считаем, что вода кипит.

OneWire ds(TERMO_PIN);
unsigned long start_ms = 0; //время начала эксперимента
boolean status_run = 0; //режим работы. 0 - ожидание, 1 - снятие температуры.
boolean boil = false; //флаг закипания
int temperature = 0;
int device_id;
unsigned int exper_id;

void setup() {
  Serial.begin(38400);
  randomSeed(analogRead(RANDOM_PIN));
  pinMode(BTN_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
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
}

void loop() {
  //Сменим режим работы, если нажата кнопка
  if (BtnPressed(BTN_PIN))
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
    //печать в серийный порт
    PrintTemperature();
  }
}

//Печать всего в серийный порт
void PrintTemperature() {
  Serial.print("RID: ");
  Serial.print(device_id);
  Serial.print("_");
  Serial.print(exper_id);
  Serial.print(" :MS: ");
  Serial.print(millis() - start_ms);
  Serial.print(" :T: ");
  Serial.print(temperature);
  if (temperature >= BOIL_TEMP && !boil) {
    Serial.print(" :boil: 1");
    boil = true;
    tone(BUZZER_PIN, 4000, 1000);
  }
  else if (boil && temperature < BOIL_TEMP) {
    Serial.print(" :boil: -1");
    boil = false;
  }
  else {
    Serial.print(" :boil: 0");
  }
  Serial.println();
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

