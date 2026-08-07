# 🧩 P-E-R-R-Y imodule

A tiny plugin system for C++ : load shared libraries at runtime, discover
which contracts (`IModule` subclasses) each one provides, and read them back
by type, by vendor, or by row.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](#)

---

## ✨ Overview

`P-E-R-R-Y imodule` is a **header-only** library for a plugin architecture :
a `.so`/`.dylib`/`.dll` is loaded, it exposes one or more contracts through
`extern "C"` factory functions, and `ModuleManager` keeps track of who
provides what.

- `SharedLibrary` — RAII wrapper around a single dll : `dlopen`/`LoadLibrary`
  on construction, `dlclose`/`FreeLibrary` on destruction, `symbol<T>(name)`
  to resolve an entry point. Cross-platform, non-copyable.
- `IModule` — the minimal contract every module implements : `type()`,
  `name()`.
- `Stride<Identity, Columns...>` — a generic two-axis table : rows are a
  dense index, columns are a fixed set of types known at compile time (one
  `vector<optional<T>>` per type, no hashing), and each row also owns an
  `Identity` — whatever the caller wants a row to be keyed by. It knows
  nothing about dlls or modules; `ModuleManager` is just one user of it.
- `ModuleManager<Ts...>` — a `Stride<ModuleLibrary, Ts*...>` : a loaded dll
  **is** a row, each contract in `Ts...` **is** a column, and the row's
  `Identity` (`ModuleLibrary`) is the struct that owns the `SharedLibrary`.
  There is no separate index-to-library map to keep in sync : `Unload()`
  clearing the row also destroys the `unique_ptr<SharedLibrary>`, which
  closes the dll.

---

## 🧱 Features

- Load a dll, get every contract it provides discovered automatically via
  `T::entry` (an `extern "C"` factory function name each contract defines)
- Partial coverage is not an error : a vendor providing only some contracts
  just leaves the others unset
- Three ways to read a module back : `Get<T>(vendorName)`, `Get<T>(Entity)`,
  `GetAll<T>()` (every vendor of one contract), `GetAll(vendorName)` (every
  contract one vendor provides)
- `Unload(key)` : one call, forgets the modules then closes the dll — no
  ordering bug possible, because both happen from the same `Stride::remove`
- No `std::any`, no `type_index`, no runtime type check anywhere — `Ts...`
  is fixed at compile time
- **Unit-tested** with GoogleTest, including a full walkthrough against real
  dlls built by the test suite itself

---

## 🧩 Example Usage

```cpp

#include <cstdio>
#include "ModuleManager.hpp"
#include "IModule.hpp"

struct IGraphicModule : IModule {
    static constexpr const char *entry = "getGraphicModule";
    const char *type() const override { return "graphic"; }
};

struct IWindowModule : IModule {
    static constexpr const char *entry = "getWindowModule";
    const char *type() const override { return "window"; }
};

// -- ray.cpp, compiled as ray.so : provides both contracts --------------
// struct Graphic final : IGraphicModule { const char *name() const override { return "ray"; } };
// struct Window  final : IWindowModule  { const char *name() const override { return "ray"; } };
// extern "C" IGraphicModule *getGraphicModule() { return &graphic; }
// extern "C" IWindowModule  *getWindowModule()  { return &window; }

int main() {
    ModuleManager<IGraphicModule, IWindowModule> modules;

    modules.Load("./ray.so", "ray");
    modules.Load("./sfml.so", "sfml"); // sfml only provides IGraphicModule

    // GetAll<T>() : every vendor of one contract.
    std::printf("graphic : %zu vendor(s)\n", modules.GetAll<IGraphicModule>().size());

    // GetAll(vendor) : everything one vendor provides, any contract.
    std::printf("ray provides %zu module(s)\n", modules.GetAll("ray").size());

    // Get<T>(name) : one module, by vendor name.
    if (IGraphicModule *ray = modules.Get<IGraphicModule>("ray"))
        std::printf("Get<IGraphicModule>(\"ray\") -> %s\n", ray->name());

    // Key(name) -> Entity, then Get(e) / Get<T>(e) : the same row, by index.
    if (auto e = modules.Key("ray")) {
        std::printf("Get(e)->path() -> %s\n", modules.Get(*e)->path().c_str());
        if (IGraphicModule *ray = modules.Get<IGraphicModule>(*e))
            std::printf("Get<IGraphicModule>(e) -> %s\n", ray->name());
    }

    // ONE call : modules forgotten, THEN the dll closed.
    modules.Unload("ray");
    std::printf("after Unload(\"ray\") : %s\n",
                modules.Get<IGraphicModule>("ray") ? "still there" : "gone");
}
```
