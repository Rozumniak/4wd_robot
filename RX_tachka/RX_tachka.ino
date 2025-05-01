// ЦЕ СКЕТЧ ПРИЙМАЧА!!!

//--------------------- НАЛАШТУВАННЯ ----------------------
#define CH_NUM 0x6e   // номер каналу (має збігатися з передавачем)
//--------------------- НАЛАШТУВАННЯ ----------------------

//--------------------- ДЛЯ РОЗРОБНИКІВ -----------------------
// РІВЕНЬ ПОТУЖНОСТІ ПЕРЕДАВАЧА
// На вибір RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
#define SIG_POWER RF24_PA_MAX

// ШВИДКІСТЬ ОБМІНУ
// На вибір RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
// має бути однакова на приймачі та передавачі!
// при найнижчій швидкості маємо найвищу чутливість і дальність!!
// УВАГА!!! enableAckPayload НЕ ПРАЦЮЄ НА ШВИДКОСТІ 250 kbps!
#define SIG_SPEED RF24_1MBPS
//--------------------- ДЛЯ РОЗРОБНИКІВ -----------------------

//--------------------- БІБЛІОТЕКИ ----------------------
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "GyverMotor.h"
#include <Servo.h> 

GMotor motorL(DRIVER2WIRE, 2, 3);
GMotor motorR(DRIVER2WIRE, 4, 5);

const int trigPin = 7;
const int echoPin = 8;

long duration;      // час проходження звукової хвилі
int distance;       // відстань, виміряна за допомогою звукової хвилі

int i = 90;         // початкове положення серво (вперед)

Servo myServo;      // серводвигун, названий "RadarServo"

//RF24 radio(9, 10);  // "створити" модуль на піннах 9 і 10 для НАНО/УНО
RF24 radio(9, 53); // для MEGA
//--------------------- БІБЛІОТЕКИ ----------------------

//--------------------- ЗМІННІ ----------------------
byte pipeNo;
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; // можливі адреси "труб"

int recieved_data[2];   // масив прийнятих даних
int telemetry[2];       // масив телеметричних даних (те, що відправляємо передавачу)
//--------------------- ЗМІННІ ----------------------

int calculateDistance() {   // функція для обчислення відстані за допомогою ультразвукового датчика
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);
  
  // встановити trigPin у HIGH на 10 мікросекунд
  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH); // зчитуємо тривалість сигналу
  distance = duration * 0.034 / 2;   // ділимо на 2 — туди й назад
  
  return distance; // повертаємо значення відстані
}  

void setup() {
  Serial.begin(9600);
  radioSetup();
  motorL.setMode(AUTO);
  motorR.setMode(AUTO);

  pinMode(trigPin, OUTPUT);       // встановлюємо trigPin як вихід
  pinMode(echoPin, INPUT);        // встановлюємо echoPin як вхід

  myServo.attach(6);              // підключаємо серводвигун до піну
  myServo.write(90);              // встановлюємо серво у вихідне (пряме) положення
}

void loop() {
  for (int i = 0; i <= 180; i++) {
    distance = calculateDistance(); // визначення відстані за допомогою функції

    myServo.write(i); // поворот серводвигуна на заданий кут
    delay(30);        // затримка для стабілізації руху серво
  }

  for (int i = 180; i >= 0; i--) {
    distance = calculateDistance(); // визначення відстані за допомогою функції

    myServo.write(i); // поворот серводвигуна на заданий кут
    delay(30);        // затримка для стабілізації руху серво
  }

  if (radio.available()) {
    while (radio.available(&pipeNo)) {        // слухаємо ефір
      
      radio.read(&recieved_data, sizeof(recieved_data));  // читаємо вхідний сигнал

      int X = 255 - recieved_data[0] / 2;
      int Y = 255 - recieved_data[1] / 2;

      distance = calculateDistance();
      delay(5);
      Serial.println(distance);

      if (distance <= 15) {
        int dutyR = -80;
        int dutyL = -80;
        motorR.setSpeed(dutyR);
        motorL.setSpeed(dutyL);
      } else {
        int dutyR = Y + X;
        int dutyL = Y - X;
        motorR.setSpeed(dutyR);
        motorL.setSpeed(dutyL);
      }

      // формуємо пакет телеметричних даних (напруга АКБ, швидкість, температура...)
      telemetry[0] = i; 
      telemetry[1] = distance; 
      //Serial.println(telemetry[1]);

      // надсилаємо телеметричний пакет
      radio.writeAckPayload(pipeNo, &telemetry, sizeof(telemetry));
    }
  }
}

void radioSetup() {             // налаштування радіо
  radio.begin();                // активувати модуль
  radio.setAutoAck(1);          // режим підтвердження прийому, 1 - увімкнено, 0 - вимкнено
  radio.setRetries(0, 15);      // (затримка між спробами, кількість спроб)
  radio.enableAckPayload();     // дозволити надсилання даних у відповідь на вхідний сигнал
  radio.setPayloadSize(32);     // розмір пакета в байтах
  radio.openReadingPipe(1, address[0]); // слухаємо трубу 0
  radio.setChannel(CH_NUM);     // обираємо канал (де немає завад)
  radio.setPALevel(SIG_POWER);  // рівень потужності передавача
  radio.setDataRate(SIG_SPEED); // швидкість обміну
  // має бути однакова на приймачі та передавачі!
  // при найнижчій швидкості маємо найвищу чутливість і дальність!!

  radio.powerUp();         // запуск модуля
  radio.startListening();  // починаємо слухати ефір, ми — приймач
}
