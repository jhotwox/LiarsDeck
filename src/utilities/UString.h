#pragma once
#include <Arduino.h>

void ignoreTildes(String& str) {
  str.replace("á", "a");
  str.replace("Á", "a");
  str.replace("é", "e");
  str.replace("É", "e");
  str.replace("í", "i");
  str.replace("Í", "i");
  str.replace("ó", "o");
  str.replace("Ó", "o");
  str.replace("ú", "u");
  str.replace("Ú", "u");
  // str.replace("ñ", "n");
}

void formatText(String& str) {
  str.toLowerCase();
  ignoreTildes(str);
}

inline bool equalsIgnoreCase(const String& a, const String& b) {
  String aCopy = a;
  String bCopy = b;
  formatText(aCopy);
  formatText(bCopy);
  return aCopy == bCopy;
}
