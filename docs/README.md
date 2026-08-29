# imodule

version: 1.0.0

> The contract every loadable module answers to.

One interface, one file. A module says what it **is** and **who** it is —
nothing else. Everything about finding, loading and keeping track of those
modules lives in [modulemanager](https://github.com/P-E-R-R-Y/modulemanager).

```cpp
class IModule {
    public:
        virtual ~IModule() = default;
        virtual const char *type() const = 0;   // "graphic", "audio", ...
        virtual const char *name() const = 0;   // "raylib", "sfml", ...
};
```

Contracts extend it and add a unique `entry` symbol, which is the only
thing a loader ever looks up:

```cpp
class IGraphic2Module : public IModule {
    static constexpr const char *entry = "getGraphic2Module";
};
```

Symbol present, capability present. A vendor that cannot do 3D simply does
not export `getGraphic3Module`, and nothing breaks.
