# StdIo: реализация и композиция

StdIo — пакет с одной точкой входа `extern "C" void compose(NodeBuild root)`.
Только `compose.cpp` создаёт дерево, объявляет предоставляемые API, публикует
общий IO и размещает сериализатор конфигурации. Парсеры и файловый сервис — обычные C++ классы.

```text
StdIo
├── YamlParser:yaml        предоставляет ParserAPI
├── TomlParser:toml        предоставляет ParserAPI
├── TsvParser:tsv          предоставляет ParserAPI
├── IOSubsystem:io         использует три парсера; общий кандидат Context
└── Serializer:config      конфигурация runtime поверх IO (модуль Serialization); load/save на корне ветки
```

```cpp
extern "C" void compose(NodeBuild root) {
    root.emplace<YamlParser, ParserAPI>("yaml");
    root.emplace<TomlParser, ParserAPI>("toml");
    root.emplace<TsvParser, ParserAPI>("tsv");
    root.emplaceShared<IOSubsystem>("io", root.children<ParserAPI>());
}
```

`children` передаётся как `std::vector<ParserAPI*>` и записывает зависимости IO
от конкретных парсеров.
Удаление необходимого парсера завершает IO и его зависимых потребителей.

## Использование вне runtime

```cpp
YamlParser yaml;
TomlParser toml;
TsvParser tsv;
IOSubsystem io{{&yaml, &toml, &tsv}};
auto document = io.read("settings.yaml");
io.write("copy.yaml", document);
```

Парсеры должны жить дольше IO. Сервис не создаёт и не владеет ими, не хранит
builders, NodeId или runtime handles. `ParserAPI` — обычный полиморфный контракт
с виртуальным деструктором. Неизвестное расширение и несколько подходящих
парсеров дают `Lattice::Exception<Owner>` (header-only, производный от `std::runtime_error`).

`Document` использует общий тип данных `Lattice::Value` из заголовков библиотеки.
Это представление данных; NodeSystem, регистрация типов и запущенный runtime для
чтения/записи не нужны. Ошибки и логирование не привязаны к Lattice.

## Граница с предметными загрузчиками

IO читает и записывает Document. Он не сканирует registry в поиске `LoaderAPI` и
не создаёт чужие загрузчики. `LoaderAPI` тоже обычный C++ интерфейс. Предметная
ветка создаёт свой loader, передаёт зависимости и решает, какой раздел документа
ему загрузить.

Например, Shell создаёт KeybindsLoader рядом с ActionRouter и экспортирует свой
`load`: он читает keybinds через общий IO. Serializer сам читает свой YAML-файл.
У StdIo нет startup-действия, обходящего весь каталог Config и вызывающего все
зарегистрированные загрузчики.

## Файлы и сборка

| Назначение | Файл / target |
|---|---|
| Объекты, предоставляемые API, общий IO | `compose.cpp` / `StdIo` |
| Публичные обычные контракты | `StdIo/ParserAPI.hpp`, `StdIo/LoaderAPI.hpp`, `StdIo/IOSubsystem.hpp` |
| Реализации парсеров и сервиса | `StdIo/*.cpp` / `StdIo.core` |
| Использование без NodeSystem и загрузки плагинов | `tests/StdIo_tests.cpp` / `StdIo.tests` |

```sh
xmake f -y --lab_packages=StdIo,Serialization
xmake build StdIo.tests
xmake run StdIo.tests
xmake build Serialization.tests
xmake run Serialization.tests
```

YAML (`.yaml`, `.yml`) — стандартный формат Serialization; TOML также читает и
записывает документы. TSV — только чтение. Формат данных и правила сохранения —
в [документации Serialization](../Serialization/README.md).
