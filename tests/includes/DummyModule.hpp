/**
 * @file DummyModule.hpp
 * @brief Un module qui ne fait rien, pour eprouver ce que TOUT module a.
 *
 * IModule ne demande que deux methodes ; le reste - le compteur, le drapeau
 * de condamnation, les claims() - est fourni ou optionnel. C'est donc ce
 * "reste" qu'on teste, et il n'a besoin d'aucun vendor pour l'etre.
 */

#ifndef DUMMYMODULE_HPP_
#define DUMMYMODULE_HPP_

#include "IModule.hpp"

#include <initializer_list>
#include <vector>

class DummyModule : public IModule {

    public:
        DummyModule(const char *type, const char *name,
                    std::initializer_list<const char *> claims = {})
            : _type(type), _name(name), _claims(claims)
        {
            _claims.push_back(nullptr);
        }

        const char *type() const override { return _type; }
        const char *name() const override { return _name; }
        const char *const *claims() const override { return _claims.data(); }

        /** @brief Le manager qu'on lui a pose, vu de l'exterieur. */
        IModuleManager *seen() const { return registry(); }

    private:
        const char *_type;
        const char *_name;
        std::vector<const char *> _claims;
};

#endif /* !DUMMYMODULE_HPP_ */
