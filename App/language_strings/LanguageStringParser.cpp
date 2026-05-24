#include "App/language_strings/LanguageStringParser.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <string_view>

#include "App/language_strings/GlobalStrings.h"

enum class CurrentAction { lhs, rhs };

void trimString(std::string_view& str) {
    if (str.starts_with(' ')) {
        str.remove_prefix(1);
    }
    if (str.ends_with(' ')) {
        str.remove_suffix(1);
    }
}

void trimLhs(std::string_view& str) { trimString(str); }

void trimRhs(std::string_view& str) {
    trimString(str);
    assert(str.starts_with('\"'));
    assert(str.ends_with('\"'));
    str.remove_prefix(1);
    str.remove_suffix(1);
}

void line_finished(std::string_view& lhs, std::string_view& rhs) {
    trimLhs(lhs);
    trimRhs(rhs);
    global_string_setters.at(std::string(lhs))(*global_strings, std::string(std::string(rhs)));
}

// Language file's structure:
// string="value",
// string2= "value2",
// string3 = "value3",
// string4 = "value with spaces",
// string5 =
//               "lots of spaces that are ignored",
// string6 = "string with some \"parentesis\" inside",
void LanguageStringParser::loadGlobalStringByPath(std::string path) {
    GlobalStrings* r = new GlobalStrings{};

    std::ifstream stream(path);
    assert(stream.is_open());

    // TODO: optimize (if needed)
    std::string_view lhs{};
    std::string lhs_s{};
    std::string_view rhs{};
    std::string rhs_s{};

    CurrentAction current_token_type = CurrentAction::lhs;
    for (char symbol; stream.get(symbol);) {
        switch (current_token_type) {
        case CurrentAction::lhs:
            if (symbol == '=') {
                lhs = {strdup(lhs_s.data()), lhs_s.length()};
                current_token_type = CurrentAction::rhs;
            }
            else {
                lhs_s.push_back(symbol);
            }
            break;
        case CurrentAction::rhs:
            if (symbol == ',') {
                rhs = { strdup(rhs_s.data()), rhs_s.length() };
		line_finished(lhs, rhs);
                current_token_type = CurrentAction::lhs;
            }
            else {
                rhs_s.push_back(symbol);
            }
            break;
        }
    }

    if (current_token_type == CurrentAction::rhs) {
      rhs = { strdup(rhs_s.data()), rhs_s.length() };
      line_finished(lhs, rhs);
    }
}

void LanguageStringParser::loadGlobalStringByLanguage(Language lang) {
    LanguageStringParser::loadGlobalStringByPath(LanguagePath::getPathByLanguage(lang));
}
