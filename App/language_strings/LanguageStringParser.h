#pragma once
#include "App/language_strings/GlobalStrings.h"

class LanguageStringParser {
public:
  static void loadGlobalStringByPath(std::string path);
  static void loadGlobalStringByLanguage(Language lang);
};
