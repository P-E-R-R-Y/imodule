# imodule — changelog

Markers: 🟢 added · 🔴 breaking · 🔵 fix · ⚪ internal or docs · 🟡 proposed
in the plan, no code written yet.

## v0.1.0

- 🟢 `IModule`: `type()`, `name()`, `bind()`, `registry()`, `acquire()`,
  `release()`, `uses()`, `mustClose()`, `isClosed()`, `condemn()`, `reset()`
- 🟢 `IModuleRegistry`: `Get`, `GetAllByType`, `GetAllByKey`, `GetAll`,
  `Current`, `Select`
- 🟢 `accepts[]`: the convention, declared by whichever interface needs it
  (`igraphic`)
- 🟢 13 tests against fake abstractions (`DummyModule`, `DummyRegistry`)

## v0.2.0

The manager moves in, and the guest view goes away.

- 🔴 `IModuleRegistry` removed. `IModuleManager` — until now in the
  `modulemanager` repo — lives here instead, and is what `IModule::bind()`
  hands over. `modulemanager` is retired.
- 🔴 `Current()` / `Select()` removed. Nothing arbitrates for anyone: a
  guest reads `GetAllByType()` and picks for itself.
- 🔴 `GetAllByType()` / `GetAllByKey()` / `GetAll()` return a `Stride::Span`
  instead of a `std::vector`. Empty cells come back as `nullptr` — that
  vendor simply does not provide that contract.
- 🟢 `claims()` on `IModule` — the resources it holds exclusively, as
  opaque strings. `acquire()` refuses up front on a collision.
- 🟢 `IModuleManager::add()` — a column with no library, for a module
  compiled into the host. Takes a `unique_ptr`: the caller gives it up.
- 🟢 `Stride<T>` — the table itself: one flat buffer, a row or a column
  read in place as a start, a step and a length.
- 🟢 `SharedLibrary` moved in from `modulemanager`.
- ⚪ 44 tests, including a real `dlopen` claims collision between two
  vendors.
