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

## v0.2.0 — proposed, none of this is written

- 🟡 `claims()` on `IModule` — the resources a module claims on the
  process, as opaque strings compared only by equality
- 🟡 `acquire()`/`release()`/`uses()`/`condemned()` moved onto
  `IModuleRegistry` — the table would stop taking the module's word for it
- 🟡 `Select()` removed from the guest view — a guest could no longer
  repoint the contract for everyone else
- 🟡 `reset()` removed — it only existed because state lived inside a dll
  that macOS often doesn't unmap
