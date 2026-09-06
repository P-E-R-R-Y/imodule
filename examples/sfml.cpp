/* sfml : fournit graphic SEULEMENT, pas window - couverture partielle
 * volontaire - et revendique OpenGL, comme ray.cpp. */
#include "graphic.hpp"

namespace {
struct Graphic final : GraphicModule {
    const char *name() const override { return "sfml"; }
    const char *const *claims() const override {
        static const char *c[] = { "opengl", nullptr };
        return c;
    }
};
Graphic graphic;
IModule *list[] = { &graphic, nullptr };
} // namespace

extern "C" IModule **getModules() { return list; }
