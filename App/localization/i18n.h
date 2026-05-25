#include <string_view>

#include "generated/translate/translate.h"

namespace i18n {
    enum class Lang { En, Ru };

    class Table {
    public:
        [[nodiscard]] static constexpr std::string_view get(std::string_view key, Lang lang) {
            auto it = std::lower_bound(detail::kTranslations.begin(), detail::kTranslations.end(), key,
                                       [](const detail::Row& row, std::string_view k) { return std::string_view(row.key) < k; });

            if (it != detail::kTranslations.end() && it->key == key) {
                return lang == Lang::Ru ? it->ru : it->en;
            }

            return key;
        }
    };

    constexpr std::string_view tr(std::string_view key, Lang lang = Lang::Ru) noexcept { return Table::get(key, lang); }
}
