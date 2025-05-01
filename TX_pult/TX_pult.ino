// ЦЕ СКЕТЧ ПЕРЕДАВАЧА!!!

//--------------------- НАЛАШТУВАННЯ ----------------------
#define CH_NUM 0x60   // номер каналу (має збігатися з приймачем)
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
//RF24 radio(9, 10); // "створити" модуль на піннах 9 і 10 для Uno
RF24 radio(9, 53); // для Mega
//--------------------- БІБЛІОТЕКИ ----------------------

//--------------------- ЗМІННІ ----------------------
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; // можливі номери труб
byte X = 0;  // потенціометр на 0 аналоговому піні
byte Y = 1;  // повзунковий потенціометр на 1 аналоговому піні
byte light_button = 3; //кнопка перемикання фар на 3 цифровому піні

int transmit_data[3];     // масив даних для передачі
int telemetry[2];         // масив прийнятих від приймача телеметричних даних
byte rssi;
int trnsmtd_pack = 1, failed_pack;
unsigned long RSSI_timer;

boolean butt_flag = 0;
boolean button;
boolean led = 0;
//--------------------- ЗМІННІ ----------------------

void setup() {
  Serial.begin(9600); // відкриваємо порт для зв’язку з ПК
  radioSetup();
  pinMode(light_button, INPUT_PULLUP); // налаштувати пін кнопки
}

int applyDeadzone(int value, int center, int tolerance) {
  if (value >= (center - tolerance) && value <= (center + tolerance)) {
    return center;  // Якщо значення в межах мертвої зони, повертаємо центральне значення
  }
  return value;  // Якщо значення поза мертвою зоною, повертаємо його без змін
}

void loop() {
  // Зчитування положення джойстика
  int xVal = map(applyDeadzone(analogRead(X), 512, 35), 0, 1023, 1023, 0);
  int yVal = map(applyDeadzone(analogRead(Y), 512, 35), 0, 1023, 1023, 0); 

  // Застосовуємо мертву зону
  transmit_data[0] = xVal;
  transmit_data[1] = yVal;
  Serial.println(xVal);
  Serial.println(yVal);

  button = !digitalRead(3);

  if (button == 1 && butt_flag == 0){
    butt_flag = 1;
    led = !led;
    transmit_data[2] = led;
  }
  if (button == 0 && butt_flag == 1){
    butt_flag = 0;
  }

  // передача пакета transmit_data
  if (radio.write(&transmit_data, sizeof(transmit_data))) {
    trnsmtd_pack++;
    if (!radio.available()) {   // якщо отримано порожню відповідь
    } else {
      while (radio.available() ) {                  // якщо у відповіді щось є
        radio.read(&telemetry, sizeof(telemetry));  // зчитуємо
        // отримали заповнений даними масив telemetry у відповіді від приймача
        //Serial.print(telemetry[0]); // Вивід кута до шлюзу
        //Serial.print(","); // Роздільник для обробки в середовищі аналізу даних
        //Serial.print(telemetry[1]); // Вивід відстані до шлюзу
        //Serial.print("."); // Роздільник для обробки в середовищі аналізу даних
      }
    }
  } else {
    failed_pack++;
  }

  if (millis() - RSSI_timer > 1000) {    // таймер RSSI
    // розрахунок якості зв’язку (0 - 100%) на основі кількості помилок і успішних передач
    rssi = (1 - ((float)failed_pack / trnsmtd_pack)) * 100;

    // скидання значень
    failed_pack = 0;
    trnsmtd_pack = 0;
    RSSI_timer = millis();
  }
}

void radioSetup() {
  radio.begin();              // активувати модуль
  radio.setAutoAck(1);        // режим підтвердження прийому, 1 - увімкнено, 0 - вимкнено
  radio.setRetries(0, 15);    // (час між спробами достукатися, кількість спроб)
  radio.enableAckPayload();   // дозволити надсилання даних у відповідь на вхідний сигнал
  radio.setPayloadSize(32);   // розмір пакету в байтах
  radio.openWritingPipe(address[0]);   // ми - труба 0, відкриваємо канал для передачі
  radio.setChannel(CH_NUM);            // вибираємо канал (де немає шумів!)
  radio.setPALevel(SIG_POWER);         // рівень потужності передавача
  radio.setDataRate(SIG_SPEED);        // швидкість обміну
  // має бути однакова на приймачі та передавачі!
  // при найнижчій швидкості маємо найвищу чутливість і дальність!!

  radio.powerUp();         // запуск модуля
  radio.stopListening();   // не слухаємо ефір, ми - передавач
}
