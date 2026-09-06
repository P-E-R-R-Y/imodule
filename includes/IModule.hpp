/**
 * @file IModule.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief Ce qu'un module dit de lui-meme, et l'etat de sa bibliotheque.
 * @date 2026-08-04
 *
 * @addtogroup imodule
 * @{
 */

#ifndef IMODULE_HPP
#define IMODULE_HPP

class IModuleManager;

/**
 * @interface IModule
 * @brief Ce que tout module chargeable sait dire de lui-meme.
 *
 * A ecrire : type(), name(), et claims() s'il tient une ressource
 * exclusive. Le reste - compteur de detenteurs, drapeau de fermeture - est
 * fourni.
 */
class IModule {
    public:
        virtual ~IModule() = default;

        /** @brief Le contrat qu'il remplit : "graphic2", "audio", "game"... */
        virtual const char *type() const = 0;

        /** @brief Le fournisseur : "raylib", "sfml", "console"... */
        virtual const char *name() const = 0;

        /**
         * @brief Les ressources qu'il tient pour lui seul : "opengl"...
         *
         * Tableau termine par nullptr, vide par defaut. Deux modules qui
         * revendiquent la meme ressource ne peuvent pas etre tenus en meme
         * temps.
         *
         * @return const char *const*
         */
        virtual const char *const *claims() const {
            static const char *none[] = { nullptr };
            return none;
        }

        /** @brief Le manager, pose au chargement. Jamais nul ensuite. */
        void bind(IModuleManager &manager) { _manager = &manager; }

        /**
         * @brief Se declarer detenteur.
         *
         * @return false si une de ses claims() est deja prise ailleurs ;
         *         rien n'est alors modifie
         */
        bool acquire();

        /** @brief Rendre ce qu'on tenait. A appeler apres l'avoir detruit. */
        void release();

        /** @brief Combien de detenteurs, a cet instant. */
        unsigned uses() const { return _uses; }

        /**
         * @brief Sa bibliotheque est condamnee : les detenteurs doivent
         *        lacher.
         *
         * Condamne n'est pas ferme : tant qu'un detenteur reste, tout ce
         * que ce module a fabrique reste valide.
         *
         * @return bool
         */
        bool mustClose() const { return _mustClose; }

        /** @brief Plus aucun detenteur. */
        bool isClosed() const { return _uses == 0; }

        /** @brief Le condamner. Appele par le manager, jamais par un module. */
        void condemn() { _mustClose = true; }

        /**
         * @brief Le rendre a neuf : ni detenteur, ni condamnation.
         *
         * Appele a chaque chargement - un dlopen peut rendre un objet qui a
         * deja vecu.
         */
        void reset() { _uses = 0; _mustClose = false; }

    protected:
        /** @brief Pour aller chercher les autres modules. */
        IModuleManager *registry() const { return _manager; }

    private:
        friend class IModuleManager;

        IModuleManager *_manager = nullptr;

        /* Un entier nu, pas un atomic : la boucle qui lit et ecrit ces deux
         * champs est sequentielle, les vendors graphiques l'exigent. */
        unsigned _uses = 0;
        bool _mustClose = false;
};

/** @} */

#endif // IMODULE_HPP
