/* raylib : fournit DEUX contrats depuis la meme dll, et revendique OpenGL -
 * comme sfml ci-dessous, pour eprouver le conflit de claims() entre deux
 * vrais vendors, charges par de vrais dlopen. */
#include "graphic.hpp"

namespace {
struct Graphic final : GraphicModule {
    const char *name() const override { return "ray"; }
    const char *const *claims() const override {
        static const char *c[] = { "opengl", nullptr };
        return c;
    }
};
struct Window final : WindowModule { const char *name() const override { return "ray"; } };
Graphic graphic;
Window window;
IModule *list[] = { &graphic, &window, nullptr };
} // namespace

extern "C" IModule **getModules() { return list; }
