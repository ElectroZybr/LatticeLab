#include "App/language_strings/GlobalStrings.h"

// i think there's a better way but whatever
std::string LanguagePath::getPathByLanguage(Language lang) {
  switch (lang) {
  case Language::en:
    return "./assets/translations/en.lang";
    break;
    
  case Language::ru:
    return "./assets/translations/ru.lang";
    break;
  }
}

std::string& CachedString::str() { return this->original_str; }

const char* CachedString::c_str() {
    if (!cached) {
        this->cached_c_str = this->original_str.c_str();
        cached = true;
    }
    return this->cached_c_str;
}
