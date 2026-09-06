#include "IModule.hpp"

struct GraphicModule : IModule {
    static constexpr const char *contract = "graphic";
    static constexpr const char *accepts[] = {"graphic", nullptr};

    const char *type() const override { return contract; }
};

struct WindowModule : IModule {
    static constexpr const char *contract = "window";
    static constexpr const char *accepts[] = {"window", nullptr};

    const char *type() const override { return contract; }
};
