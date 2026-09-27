#pragma once

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Lattice {

/**
 @file Value.hpp
 @brief Представление значений, загружаемых из конфигурационных и табличных данных.

 Value используется парсерами и загрузчиками как промежуточное представление данных.

 Поддерживаемые типы:
 - string
 - int
 - double
 - bool
 - vec2
 - vec3
 - vec4
 - Array
 - Object

 Value не предназначен для хранения основных runtime-данных.
 После загрузки данные должны преобразовываться в специализированные структуры.
*/

struct Value;

using Array = std::vector<Value>;
using Object = std::unordered_map<std::string, Value>;

using StringPtr = std::shared_ptr<std::string>;
using ArrayPtr = std::shared_ptr<Array>;
using ObjectPtr = std::shared_ptr<Object>;

struct Value : std::variant<StringPtr, int64_t, double, bool, 
    glm::vec2, glm::vec3, glm::vec4, ArrayPtr, ObjectPtr> {

    using variant::variant;
    Value(std::string value) : variant(std::make_shared<std::string>(std::move(value))) {}
    Value(const char* value) : variant(std::make_shared<std::string>(value)) {}
    Value(Array value) : variant(std::make_shared<Array>(std::move(value))) {}
    Value(Object value) : variant(std::make_shared<Object>(std::move(value))) {}

    template<typename T>
    bool is() const {
        if constexpr (std::is_same_v<T, std::string>)
            return std::holds_alternative<StringPtr>(*this);
        else if constexpr (std::is_same_v<T, Array>)
            return std::holds_alternative<ArrayPtr>(*this);
        else if constexpr (std::is_same_v<T, Object>)
            return std::holds_alternative<ObjectPtr>(*this);
        else
            return std::holds_alternative<T>(*this);
    }

    template<typename T>
    T get() const {
        if constexpr (std::is_same_v<T, float>) 
            return static_cast<float>(std::get<double>(*this));
        else if constexpr (std::is_same_v<T, std::string>) 
            return *std::get<StringPtr>(*this);
        else if constexpr (std::is_same_v<T, Array>) 
            return *std::get<ArrayPtr>(*this);
        else if constexpr (std::is_same_v<T, Object>)
            return *std::get<ObjectPtr>(*this);
        else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, int64_t>) 
            return static_cast<T>(std::get<int64_t>(*this));
        else return std::get<T>(*this);
    }

    template<typename T>
    decltype(auto) require() const {
        if constexpr (std::is_same_v<T, std::string>)
            return static_cast<const std::string&>(*std::get<StringPtr>(*this));
        else if constexpr (std::is_same_v<T, Array>)
            return static_cast<const Array&>(*std::get<ArrayPtr>(*this));
        else if constexpr (std::is_same_v<T, Object>)
            return static_cast<const Object&>(*std::get<ObjectPtr>(*this));
        else {
            if (!is<T>()) throw std::bad_variant_access{};
            return std::get<T>(*this);
        }
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

    std::string toString() const {
        return std::visit([](const auto& value) -> std::string {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, StringPtr>)
                return value ? *value : "";

            else if constexpr (std::is_same_v<T, bool>)
                return value ? "true" : "false";

            else if constexpr (std::is_same_v<T, int64_t>)
                return std::to_string(value);

            else if constexpr (std::is_same_v<T, double>)
                return std::format("{}", value);

            else if constexpr (std::is_same_v<T, glm::vec2>)
                return std::format("[{}, {}]", value.x, value.y);

            else if constexpr (std::is_same_v<T, glm::vec3>)
                return std::format("[{}, {}, {}]", value.x, value.y, value.z);

            else if constexpr (std::is_same_v<T, glm::vec4>)
                return std::format("[{}, {}, {}, {}]", value.x, value.y, value.z, value.w);

            else if constexpr (std::is_same_v<T, ArrayPtr>) {
                if (!value) return "[]";

                std::string result = "[";
                for (size_t i = 0; i < value->size(); ++i) {
                    if (i) result += ", ";
                    result += (*value)[i].toString();
                }
                return result + "]";
            }

            else if constexpr (std::is_same_v<T, ObjectPtr>) {
                if (!value) return "{}";

                std::string result = "{";
                bool first = true;

                for (const auto& [key, item] : *value) {
                    if (!first) result += ", ";
                    first = false;

                    result += key;
                    result += ": ";
                    result += item.toString();
                }

                return result + "}";
            }
        }, static_cast<const variant&>(*this));
    }
};

/**
 * Converts a runtime parameter to and from the closed Value representation.
 *
 * Parameter types which are not part of Value can specialize this adapter
 * without extending the kernel's generic data type.
 */
template<typename T>
struct ParamAdapter {
    static Value get(const T& value) {
        return Value{value};
    }

    static void set(T& target, const Value& value) {
        target = value.get<T>();
    }
};

}
