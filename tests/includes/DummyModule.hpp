/**
 * @file DummyModule.hpp
 * @brief Un module qui ne fait rien, pour eprouver ce que TOUT module a.
 *
 * IModule ne demande que deux methodes ; le reste - le compteur, le drapeau
 * de condamnation - est fourni. C'est donc ce "reste" qu'on teste, et il
 * n'a besoin d'aucun vendor pour l'etre.
 */

#ifndef DUMMYMODULE_HPP_
#define DUMMYMODULE_HPP_

#include "IModule.hpp"

class DummyModule : public IModule {

    public:
        DummyModule(const char *type, const char *name) : _type(type), _name(name) {}

        const char *type() const override { return _type; }
        const char *name() const override { return _name; }

        /** @brief Le registre qu'on lui a pose, vu de l'exterieur. */
        IModuleRegistry *seen() const { return registry(); }

    private:
        const char *_type;
        const char *_name;
};

#endif /* !DUMMYMODULE_HPP_ */
