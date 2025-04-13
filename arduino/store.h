#ifndef STOREMANAGER_H
#define STOREMANAGER_H

#include <Preferences.h>
#include "Data.h"  // Предполагается, что структура Data определена в Data.h

class Store {
private:
    Preferences prefs;          // Объект для работы с энергонезависимой памятью
    Data& data;                 // Ссылка на структуру данных
    const char* namespace_name; // Имя пространства имен для Preferences
    char version;               // Версия данных

public:
    // Конструктор
    Store(Data& data_ref, const char* ns = "my-app", char ver = 'a');

    // Инициализация и загрузка данных
    void begin(char ver = 'a');

    // Загрузка данных из Preferences
    void loadData();

    // Сохранение данных в Preferences
    void saveData();

    // Обновление данных
    void update();

    // сброс данных
    void reset();
};

#endif // STOREMANAGER_H