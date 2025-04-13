#ifndef DATA_H
#define DATA_H

// структура настроек
struct Data {
  bool state = 1;                   // 0 выкл, 1 вкл
  bool lock = 0;                    // блокировка жестов
  byte mode = 0;                    // 0 цвет, 1 теплота, 2 огонь
  byte bright[3] = { 30, 30, 30 };  // яркость
  byte value[3] = { 0, 0, 0 };      // параметр эффекта (цвет...)
};

#endif // DATA_H
