# Project Milestones: PARAOS

## v1.1 Static Analysis Cleanup (Shipped: 2026-06-20)

**Delivered:** `*_clang_tidy` CMake presets теперь собираются на macOS без предупреждений; публичное API и поведение других платформ сохранены.

**Phases completed:** 5–8 (4 plans total)

**Key accomplishments:**

- Построены и классифицированы все предупреждения clang-tidy из пресетов `pc_debug_gcc_clang_tidy` и `freertos_debug_gcc_clang_tidy`.
- Исправлены предупреждения в core-заголовках и `port_unix/` с сохранением публичных сигнатур и семантики Linux-путей.
- Исправлены предупреждения в тестах и примерах (13 `.cpp`-файлов) с документированными inline-суппрессиями.
- Пройдена регрессионная защита: не-tidy-пресеты собираются, `port_win/` и `port_freertos/` не затронуты.

**Stats:**

- 18 файлов изменено
- +124 / −103 строк
- 4 phases, 4 plans, 14 tasks
- 1 день на выполнение вехи

**Git range:** `6d24cd5` → `f2c4ad1`

**Known deferred items at close:** 2 (macOS GitLab CI runner, macOS-specific build instructions) — см. `.planning/STATE.md` → Deferred Items.

**What's next:** v1.2 — CI/CD и документация для macOS, либо выбранная вместе с командой следующая веха.

---

## v1.0 macOS Support (Shipped: ранее)

**Delivered:** PARAOS собирается и работает на macOS; добавлена поддержка POSIX-совместимых примитивов через `port_unix/`.

See `.planning/milestones/` for full archive when available.
