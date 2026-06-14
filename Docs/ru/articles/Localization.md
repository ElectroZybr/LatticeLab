> Язык: RU
> Canonical: да
> Статус EN: не создан
# Локализация

Эта статья описывает, как в проекте устроена локализация интерфейса, где лежат переводы и как правильно добавить новую строку.

## Роль

Текущая локализация покрывает прежде всего GUI-слой приложения.

Схема простая:

- строки интерфейса хранятся в `assets/translate.csv`
- на этапе конфигурации и сборки из CSV генерируется таблица переводов
- в коде UI строки берутся через `_tr`

## Код

- [assets/translate.csv](../../../assets/translate.csv)
- [App/localization/i18n.h](../../../App/localization/i18n.h)
- [cmake/embedded_translate.cmake](../../../cmake/embedded_translate.cmake)
- [cmake/generate_translations.cmake](../../../cmake/generate_translations.cmake)
- [App/GUI/interface/panels/settings/SettingsPanel.cpp](../../../App/GUI/interface/panels/settings/SettingsPanel.cpp)

## Поток

Упрощенно локализация работает так:

```text
assets/translate.csv
  -> cmake/generate_translations.cmake
  -> generated/translate/translate.h
  -> App/localization/i18n.h
  -> "some_key"_tr
  -> строка в UI
```

## Таблица Переводов

Основной источник переводов:

- `assets/translate.csv`

Этот файл можно редактировать как обычный текст, а можно открывать в табличных редакторах вроде `Microsoft Excel`, `LibreOffice Calc` или `OnlyOffice`, если так удобнее работать с большим количеством строк.

Формат сейчас такой:

```text
key;en;ru
imgui_simulation;Simulation;Симуляция
imgui_render;Render;Рендер
```

Колонки:

- `key` — стабильный текстовый ключ
- `en` — английская строка
- `ru` — русская строка

## Генерация

Во время сборки `cmake/embedded_translate.cmake` запускает `cmake/generate_translations.cmake`, который генерирует:

- `generated/translate/translate.h`

Дальше этот заголовок подключается в:

- `App/localization/i18n.h`

После этого код уже работает с compile-time таблицей переводов.

Важная деталь: генератор сам сортирует строки по ключу при генерации `kTranslations`. То есть вручную поддерживать CSV в отсортированном виде желательно для читаемости, но корректность lookup на этом не держится.

## Доступ Из Кода

В коде используется `App/localization/i18n.h`.

Основные элементы:

- `i18n::Lang`
- `i18n::current()`
- `i18n::setCurrent(...)`
- `i18n::toggle()`
- `i18n::tr(...)`
- литерал `"some_key"_tr`

Типичный пример:

```cpp
using i18n::operator""_tr;

ImGui::SeparatorText("imgui_simulation"_tr.data());
```

Если ключ найден, возвращается строка текущего языка. Если ключ не найден, в UI вернется сам ключ. Это удобно для быстрого обнаружения пропущенных переводов.

## Переключение Языка

Текущее состояние языка хранится в `i18n::currentLanguage`.

Сейчас переключение делается через:

- `i18n::toggle()`

Практически оно используется в `SettingsPanel.cpp`, где кнопка языка меняет `Ru` и `En`.

## Как Добавить Перевод

Если нужно добавить новую переводимую строку:

1. Придумать стабильный ключ в стиле существующих.
2. Добавить строку в `assets/translate.csv`.
3. Переконфигурировать или пересобрать проект, чтобы заново сгенерировался `generated/translate/translate.h`.
4. Использовать ключ в коде через `"your_key"_tr`.

Минимальный пример:

```text
imgui_new_option;New option;Новая опция
```

И в коде:

```cpp
ImGui::TextUnformatted("imgui_new_option"_tr.data());
```

## Практические Правила

- ключ должен описывать смысл, а не конкретный текст
- лучше держать единый префикс по контексту, например `imgui_`, `integrator_`, `capture_`
- если строка участвует в `ImGui`-идентификаторе, suffix вроде `##id` тоже должен лежать в переводе как часть строки
- если строка используется с форматированием, placeholder'ы должны совпадать между языками

## Связи

- [README.md](../README.md)
- [App.md](./App.md)
- [GUI.md](./GUI.md)
- [Glossary.md](./Glossary.md)
