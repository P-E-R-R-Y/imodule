# imodule

🔌 **P-E-R-R-Y imodule**

The contract every loadable module answers to, and the table that holds them.

[![Build](https://github.com/P-E-R-R-Y/imodule/actions/workflows/cmake.yml/badge.svg)](https://github.com/P-E-R-R-Y/imodule/actions)
[![Docs](https://img.shields.io/badge/docs-doxygen-blue.svg)](https://p-e-r-r-y.github.io/imodule)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

---

## ✨ Overview

`P-E-R-R-Y imodule` is a **header-only** library for programs whose parts are
discovered at runtime rather than linked at build time.

It holds two things:

- `IModule` — what a module says about itself: its contract, its name, and the
  hardware it needs exclusively.
- `IModuleManager` — a `type × key` table of every module currently loaded,
  filled by `dlopen` or by hand, and emptied without ever blocking.

Core goals:
- 🔎 **Anonymous loading** — one exported symbol, `getModules()`, whatever the
  contract
- 🧩 **No central arbiter** — every guest reads the table and picks for itself
- ⛔ **Real exclusivity only** — `claims()` refuses two vendors on the same
  device, nothing else is forbidden
- ⏳ **Closing never waits** — a condemned library closes once nobody holds it
- ✅ **Unit-tested** with GoogleTest
- 🚀 **Header-only**, one dependency (`dl`)

---

## 🧱 Features

- One row per contract, one column per origin — read either way, in place
- `Load()` for a shared library, `add()` for a module compiled into the host
- `acquire()` / `release()` — refused up front when a `claims()` collides
- `Unload()` condemns, `Reconcile()` closes what nobody holds anymore
- `Get<T>()` / `GetAllByType<T>()` hand modules back with their real type

---

## 🧩 Example Usage

```cpp
#include "IModuleManager.hpp"

/* ---- the vendor side : one library, any number of modules -------- */

class SfmlGraphic : public IModule {
    public:
        const char *type() const override { return "graphic2"; }
        const char *name() const override { return "sfml"; }

        /* Exclusive : no other OpenGL vendor can be held at the same time. */
        const char *const *claims() const override {
            static const char *claimed[] = {"opengl", nullptr};
            return claimed;
        }
};

extern "C" IModule **getModules() {
    static SfmlGraphic graphic;
    static IModule *list[] = {&graphic, nullptr};
    return list;
}

/* ---- the host side ----------------------------------------------- */

int main() {
    IModuleManager modules;

    modules.Load("./sfml_impl.dylib", "sfml");   // a shared library
    modules.add("built-in", std::make_unique<SfmlGraphic>());   // or compiled in

    /* One cell : this contract, from that vendor. */
    IModule *sfml = modules.Get("graphic2", "sfml");

    /* One row : every vendor answering the contract. Empty cells come back
     * as nullptr — that vendor simply does not provide it. */
    for (IModule *module : modules.GetAllByType("graphic2")) {
        if (!module || !module->acquire())
            continue;   // refused : something already claims "opengl"

        //... use it, then hand it back
        module->release();
        break;
    }

    /* Retiring a library : condemn it, then close it once it is free. */
    modules.Unload("sfml");
    while (modules.Reconcile() == 0)
        ;   // in a real program : once per tick, never a wait loop

    return 0;
}
```
