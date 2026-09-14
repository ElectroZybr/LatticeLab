#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/**
 @file Value.hpp
 @brief Представление значений, загружаемых из конфигурационных и табличных данных.

 Value используется парсерами и загрузчиками как промежуточное представление данных.

 Поддерживаемые типы:
 - строка
 - целое число
 - число с плавающей точкой
 - bool
 - glm::vec2
 - glm::vec3
 - glm::vec4
 - массив значений
 - таблица значений

 Value не предназначен для хранения основных runtime-данных.
 После загрузки данные должны преобразовываться в специализированные структуры.
*/


struct Value;

using Array = std::vector<Value>;
using Table = std::unordered_map<std::string, Value>;

struct Value : std::variant<std::string, int64_t, double, bool, 
    glm::vec2, glm::vec3, glm::vec4, Array, Table> {

    using variant::variant;

    template<typename T>
    bool is() const {
        return std::holds_alternative<T>(*this);
    }

    template<typename T>
    T get() const {
        if constexpr (std::is_same_v<T, float>)
            return static_cast<float>(std::get<double>(*this));
        else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, bool> ||
                        std::is_same_v<T, int64_t> || std::is_same_v<T, std::string>)
            return std::get<T>(*this);
        else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
            return static_cast<T>(std::get<int64_t>(*this));
        else
            return std::get<T>(*this);
    }

    template<typename T>
    T require() const {
        if (!is<T>())
            throw std::bad_variant_access{};

        return std::get<T>(*this);
    }

    template<typename T>
    T as() const {
        if constexpr (std::is_same_v<T, float>) {
            return static_cast<float>(require<double>());
        }
        else if constexpr (
            std::is_integral_v<T> &&
            !std::is_same_v<T, bool> &&
            !std::is_same_v<T, int64_t>
        ) {
            return static_cast<T>(require<int64_t>());
        }
        else {
            return require<T>();
        }
    }
};