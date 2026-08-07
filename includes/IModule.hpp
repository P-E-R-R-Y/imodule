/**
 * @file IModule.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief Le contrat par defaut : ce que tout module sait dire de lui-meme.
 * @date 2026-08-04
 */

#ifndef IMODULE_HPP
#define IMODULE_HPP

/**
 * @interface IModule
 * @brief Ce que tout module sait dire de lui-meme : ce qu'il EST, et QUI il est.
*/
class IModule {
    public:
        virtual ~IModule() = default;

        /** @brief Le contrat rempli : "graphic", "audio", "game"... */
        virtual const char *type() const = 0;

        /** @brief Le fournisseur : "ray", "sfml", "console"... */
        virtual const char *name() const = 0;
};

#endif // IMODULE_HPP