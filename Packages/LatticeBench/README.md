# Bench: запуск бенчмарков

Инфраструктурный пакет (`id = "Bench"`) над `Lattice/Tools/BmRunner`.

```text
Bench
└── Benchmarks:bench           actions run [name], list, view <result>
```

Результаты хранятся в объекте как `Lattice::BasicTable` и не создают узлов дерева;
`run` печатает таблицу и её имя, `view <имя>` показывает сохранённый результат.

```toml
[Startup]
Bench = "on"
```

```sh
xmake build Bench.tests && xmake run Bench.tests
```
