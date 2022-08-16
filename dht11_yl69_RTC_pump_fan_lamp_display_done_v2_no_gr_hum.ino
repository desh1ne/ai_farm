#include <DHT.h>
#include <Wire.h> // Добавляем необходимые библиотеки
#include <LiquidCrystal_I2C.h>
//#include "DHT.h"
// библиотека для работы с часами реального времени
#include "TroykaRTC.h"
#define LEN_TIME 12 // размер массива для времени
#define DHTPIN A0 // к какому пину будет подключен сигнальный выход датчика

//выбор используемого датчика
#define DHTTYPE DHT11 // DHT 11
#define VODPOMPA_PIN 3 //контакт активации помпы
#define LEVEL_WATER_PIN 7 //контакт активации клапана
#define COPTER_PIN 4 //контакт активации вытяжной вентилляции
#define COPTER_IN_PIN 5 //контакт активации приточной вентилляции
#define LAMP_PIN 2 //контакт активации лампы
#define KLAPAN_PIN 6 //контакт активации клапана
#define LEVEL_SENSOR 8 //контакт активации клапана


//вода
int wlPin = A2; // датчик уровня воды
float water_level;
float water_level_percent = 0.0;
float temp_on = 28.0;
//почва
int rainPin = A1; // пин датчика влажности почвы
float g_h; //значения влажности почвы
//RTC
String time_hhmm;
String lamp_on = "23:00";
String lamp_off = "10:30";
char time_[LEN_TIME]; // массив для хранения текущего времени
//водяная помпа
//Время между проверками полива
#define time_copter_limit 1000 * 60
//Объявляем переменную, в которой будет храниться значение влажности
unsigned int hum = 0;
unsigned int level = 0;
int HUM_MIN = 30; // пороговое значение указывается тут
int LEVEL_MIN = 20;
//В этой переменной будем хранить временной промежуток
unsigned long time_copter = 0;
unsigned int vent_state = 0;
unsigned int diff_ss = 0;
bool state_in = false;
bool state_out = false;
byte degree[8] = // кодируем символ градуса
{
B00111,
B00101,
B00111,
B00000,
B00000,
B00000,
B00000,
};


DHT dht(DHTPIN, DHTTYPE); //инициализация датчика температуры
LiquidCrystal_I2C lcd(0x27,16,2); // Задаем адрес и размерность дисплея

RTC clock;// создаём объект для работы с часами реального времени


void setup()
{
  lcd.init(); // Инициализация lcd
  lcd.backlight(); // Включаем подсветку
  lcd.createChar(1, degree); // Создаем символ под номером 1
  Serial.begin(9600);
  dht.begin();
  clock.begin();
  pinMode(COPTER_PIN, OUTPUT);
  pinMode(COPTER_IN_PIN, OUTPUT);
  pinMode(LEVEL_WATER_PIN, OUTPUT);
  pinMode(VODPOMPA_PIN, OUTPUT);
  pinMode(LAMP_PIN, OUTPUT);  
  pinMode(KLAPAN_PIN, OUTPUT);
  pinMode(LEVEL_SENSOR, OUTPUT);
  //clock.set(20,20,00,06,07,2022,WEDNESDAY);
}


void water_pump(float humNow, String _time)
  {
  // Если значение показателя не равно предыдущему, то...
  if(humNow != hum) {
    //Сохраняем полученные сейчас значение
    hum= humNow;
  }
  int hh = _time.substring(0,2).toInt();
  int mm = _time.substring(3,5).toInt();
  int ss = _time.substring(6,8).toInt();
  if ( (((hh == 22) && (mm == 20) && ((ss >= 3) && (ss <= 45))) ) 
  //|| ( ((hh[ == 22) && (mm == 20) && ((ss >= 3) && (ss <= 28))) ) 
  || ( ((hh <= 10) && (mm == 20) && ((ss >= 2) && (ss <= 7))) ) )  //
  {
    digitalWrite(VODPOMPA_PIN, HIGH);
  }
  /*
  (hum >= HUM_MIN) && (hum - HUM_MIN < 20) &&
  (hum >= HUM_MIN + 20) && (hum - HUM_MIN < 45) &&
  }*/
  else {
    // Завершаем работу
    digitalWrite(VODPOMPA_PIN, LOW);
  }
}
  
  /*
//Задаем условия: если прошел заданный пользователь промежуток времени и //статус влаги в почве меньше необходимого, то...
  if ((time_water == 0 || millis() - time_water > time_water_limit) && (hum < HUM_MIN) ) {
    // Даем сигнал о начале работы водяной помпы
    digitalWrite(VODPOMPA_PIN, HIGH);
    //Объявляем потом, длящийся 2 секунды
    delay(10000);
    // Завершаем работу помпы
    digitalWrite(VODPOMPA_PIN, LOW);
    // Ставим в значение переменной Time текущее время и добавляем 3 минуты 
    time_water = millis();
    Serial.println(time_water);
  }
}*/

void copter_drive(float temp, String _time)
  {
  int hh = _time.substring(0,2).toInt();
  int mm = _time.substring(3,5).toInt();
  int ss = _time.substring(6,8).toInt();
  float _delta = random(275, 280) / 10;
  float delta_ = random(280, 285) / 10;
  if (((mm <= 15) || ((mm >= 30) && (mm < 45))) )  
    { 
    digitalWrite(COPTER_PIN, LOW);
    }
  else if (((temp >= temp_on) && ((mm >= 15) && (mm < 30)) || (mm >= 45) ) ) 
    {
    // включаем вентиллятор и меняем границу из случайного диапазона 27.5 - 27.9
    digitalWrite(COPTER_PIN, LOW);
    temp_on = _delta;
    }
  else {
    // Завершаем работу
    digitalWrite(COPTER_PIN, HIGH);
    temp_on = delta_;
       }
}

void copter_in(float temp, String _time)
  {
  bool state = false;
  int hh = _time.substring(0,2).toInt();
  int mm = _time.substring(3,5).toInt();
  if ((state == false) && ( (mm <= 15) || ((mm >= 30) && (mm < 45))) )   //
  {
    digitalWrite(COPTER_IN_PIN, LOW);
    state = true;
  }
  else {
    // Завершаем работу
    digitalWrite(COPTER_IN_PIN, HIGH);
    state = false;
    
  }
}


//заполнение водой до уровня датчика
void level_pump(float wl, String _time)
{
// Если значение показателя не равно предыдущему, то...
  if(wl != level) {
    //Сохраняем полученные сейчас значение
    level= wl;
  }
  int hh = _time.substring(0,2).toInt();
  int mm = _time.substring(3,5).toInt();
  int ss = _time.substring(6,8).toInt();
  
  if ( ((level < LEVEL_MIN) && ((hh == 1) && (mm > 0) && (mm <= 3) && ((ss >= 3) && (ss <= 33))) ) || 
  ( (level >= LEVEL_MIN) && (level - LEVEL_MIN < 60) && ((hh == 1) && (mm > 0) && (mm <= 3) && ((ss >= 3) && (ss <= 13))) ) 
     ) 
  {
    digitalWrite(LEVEL_WATER_PIN, HIGH);
  }
  else
    {
    digitalWrite(LEVEL_WATER_PIN, LOW);
    }
}

//клапан полива
void water_valve(float humNow, String _time)
  {
  // Если значение показателя не равно предыдущему, то...
  if(humNow != hum) {
    //Сохраняем полученные сейчас значение
    hum= humNow;
  }
  int hh = _time.substring(0,2).toInt();
  int mm = _time.substring(3,5).toInt();
  int ss = _time.substring(6,8).toInt();
  if ( ((hh == 22) && (mm == 20) && ((ss >= 0) && (ss <= 48)) )  ||
  ( ((hh <= 10) && (mm == 20) && ((ss >= 0) && (ss <= 11))) ) )  
  {
    digitalWrite(KLAPAN_PIN, HIGH);
  }
  else {
    // Завершаем работу
    digitalWrite(KLAPAN_PIN, LOW);
  }
}
//свет
void phytolamp(String _time)
  {
  int hh = _time.substring(0,2).toInt();
  int mm = _time.substring(3,5).toInt();
  if ( ((hh >= lamp_on.substring(0,2).toInt()) && (hh < 24) ) || (hh < lamp_off.substring(0,2).toInt()) || 
  ( (hh == lamp_off.substring(0,2).toInt() ) && (mm <= lamp_off.substring(3,5).toInt()) ) ) 
  {
    digitalWrite(LAMP_PIN, HIGH); //включаем лампу
  }
  else //if (_time == lamp_off) 
  {
    digitalWrite(LAMP_PIN, LOW);// Завершаем работу лампы
  }
}

//начало работы
void loop() {
  //digitalWrite(LAMP_PIN, HIGH);
  // Добавляем паузы в несколько секунд между измерениями
  delay(2000);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  
  // считываем входные данные с аналогового контакта 1:
  g_h = analogRead(rainPin);
  Serial.println(g_h);  
  float percentage_g_h = map(g_h, 800, 900, 0, 100);
  
  // Проверка, посылает ли датчик значения или нет
  if ((isnan(t)) || (isnan(h)) || (isnan(g_h))) {
  lcd.print("Failed");
  delay(1000);
  return;
  }

  Serial.println("WH");
  Serial.print(percentage_g_h);
  if(g_h > HUM_MIN){
    Serial.println(" - Doesn't need watering");  
    // " – Полив не нужен"
  }
  else {
    Serial.println(" - Time to water your plant");
    // " – Пора поливать!"
  }
  
  clock.read();// запрашиваем данные с часов
  clock.getTime(time_); // сохраняем текущее время, дату и день недели в переменные
  String _time_ = String(time_);
  time_hhmm = _time_.substring(0, 8);
  int hh = time_hhmm.substring(0,2).toInt();
  int mm = time_hhmm.substring(3,5).toInt();
  int ss = time_hhmm.substring(6,8).toInt();
  if ( ((hh >= 4) && (hh < 24) && (mm == 21) && ((ss >= 0) && (ss <= 10)) ) || ((hh == 1) && (mm > 0) && (mm <= 3) && ((ss >= 0) && (ss <= 40))) )
    {
      digitalWrite(LEVEL_SENSOR, HIGH);
      delay(10);
      // считываем входные данные с аналогового контакта 2:
      water_level = analogRead(wlPin);
      //Serial.println(water_level);
      float percentage_w_l = map(water_level, 25, 350, 0, 100);
      if (percentage_w_l != water_level_percent) {
        //Сохраняем полученные сейчас значение
         water_level_percent= percentage_w_l;
      }
      Serial.println(" level :");
      Serial.println(level);
    }
  else {
      digitalWrite(LEVEL_SENSOR, LOW);
  }
  
  copter_drive(t, time_hhmm); // вентиллятор OUT
  copter_in(t, time_hhmm); // вентиллятор IN
  phytolamp(time_hhmm); //включаем лампу
  water_pump(percentage_g_h, time_hhmm); //насос автополива
  water_valve(percentage_g_h, time_hhmm); //клапан автополива
  level_pump(water_level_percent, time_hhmm); //заполнение поливочного резервуара 
  // Выводим показания влажности и температуры и времени
  lcd.setCursor(0, 0); // Устанавливаем курсор в начало 1 строки
  lcd.print("L=    %"); // Выводим текст
  lcd.setCursor(2, 0); // Устанавливаем курсор на 7 символ
  lcd.print(water_level_percent, 1); // Выводим на экран значение уровня
  lcd.setCursor(0, 1); // Устанавливаем курсор в начало 2 строки
  lcd.print("t=    \1C "); // Выводим текст, \1 - значок градуса
  lcd.setCursor(2, 1); // Устанавливаем курсор на 7 символ
  lcd.print(t,1); // Выводим значение температуры
  //влажность почвы на экран:
  lcd.setCursor(9, 0); // Устанавливаем курсор в начало 1 строки
  lcd.print("W=    %"); // Выводим текст
  lcd.setCursor(11, 0); // Устанавливаем курсор на 7 символ
  lcd.print(percentage_g_h, 1); // Выводим на экран значение влажности почвы
  // время в правом нижнем углу дисплея
  lcd.setCursor(11, 1); // Устанавливаем курсор на 7 символ
  lcd.print(time_hhmm.substring(0, 5)); //время в формате HH:mm

}
