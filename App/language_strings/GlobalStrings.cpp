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
