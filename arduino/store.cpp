#include "store.h"

Store::Store(Data& data_ref, const char* ns, char ver)
  : data(data_ref), namespace_name(ns), version(ver) {}

void Store::begin(char ver) {
  version = ver;
  prefs.begin(namespace_name, false);  // Открываем Preferences для чтения/записи
  loadData();                          // Загружаем данные
  prefs.end();
}

void Store::loadData() {
  if (prefs.isKey("state")) {
    data.state = prefs.getBool("state", data.state);
  }
  if (prefs.isKey("lock")) {
    data.lock = prefs.getBool("lock", data.lock);
  }
  if (prefs.isKey("mode")) {
    data.mode = prefs.getUChar("mode", data.mode);
  }
  if (prefs.isKey("bright")) {
     prefs.getBytes("bright", data.bright, sizeof(data.bright));
  }
  if (prefs.isKey("value")) {
    prefs.getBytes("value", data.value, sizeof(data.value));
  }
}

void Store::saveData() {
  prefs.putBool("state", data.state);
  prefs.putBool("lock", data.lock);
  prefs.putUChar("mode", data.mode);
  prefs.putBytes("bright", data.bright, sizeof(data.bright));
  prefs.putBytes("value", data.value, sizeof(data.value));
}

void Store::update() {
  saveData();
}

void Store::reset() {
  prefs.clear();
}