/**
 * @file IModule.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief Le contrat par defaut : ce que tout module sait dire de lui-meme.
 * @date 2026-08-04
 *
 * @addtogroup imodule
 * @{
 */

#ifndef IMODULE_HPP
#define IMODULE_HPP

class IModuleRegistry;

/**
 * @interface IModule
 * @brief Ce que tout module sait dire de lui-meme, et l'etat de sa dll.
 *
 * Deux choses seulement sont a ecrire : type() et name(). Le reste est
 * fourni - le compteur d'usage et le drapeau de fermeture sont des donnees
 * ordinaires, mises a jour par le manager d'un cote et par les detenteurs
 * de l'autre.
 *
 * LE COMPTEUR VIT ICI, dans la dll, et c'est voulu. Ce qui ne traverse pas
 * un dlopen, c'est du CODE - un destructeur, une vtable. Un entier, si.
 * C'est ce qui distingue ce mecanisme d'un shared_ptr, dont le deleter
 * s'executerait a une adresse demappee.
 */
class IModule {
    public:
        virtual ~IModule() = default;

        /** @brief Le contrat qu'il remplit : "graphic2", "audio", "game"... */
        virtual const char *type() const = 0;

        /** @brief Le fournisseur : "raylib", "sfml", "console"... */
        virtual const char *name() const = 0;

        /**
         * @brief Le registre, pose par le manager au chargement.
         *
         * Appele juste apres getModules(), avant que le module soit
         * atteignable par qui que ce soit. Jamais nul ensuite.
         *
         * @param registry
         */
        void bind(IModuleRegistry &registry) { _registry = &registry; }

        /**
         * @brief Je te tiens. Compteur +1.
         *
         * A appeler des qu'on garde un pointeur sur ce module ou sur un
         * objet qu'il a fabrique. Tant que le compteur n'est pas nul, sa
         * dll ne peut pas se fermer.
         */
        void acquire() { _uses++; }

        /**
         * @brief Je te lache. Compteur -1.
         *
         * A appeler APRES avoir detruit ce qu'on tenait de lui, jamais
         * avant : entre les deux, la dll pourrait se fermer.
         */
        void release() { if (_uses) _uses--; }

        /** @brief Combien de detenteurs, a cet instant. */
        unsigned uses() const { return _uses; }

        /**
         * @brief Ma dll est condamnee : si tu me tiens, lache-moi.
         *
         * LE SEUL SIGNAL du mecanisme. Un detenteur le teste a chaque tick ;
         * quand il passe a vrai, il detruit ce qu'il tient et appelle
         * release().
         *
         * Vrai ne veut pas dire ferme : tant que le compteur n'est pas nul,
         * la dll reste ouverte et tout ce qu'elle a fabrique reste valide.
         * C'est justement ce qui laisse le temps de lacher proprement.
         *
         * @return bool
         */
        bool mustClose() const { return _mustClose; }

        /**
         * @brief Plus personne ne me tient.
         *
         * LA SEULE CONDITION. Une dll dont tous les modules repondent vrai,
         * et qui est condamnee, peut etre fermee.
         *
         * @return bool
         */
        bool isClosed() const { return _uses == 0; }

        /**
         * @brief Condamne ce module. Appele par le manager, jamais par un
         *        module - un module n'a pas a condamner son voisin.
         */
        void condemn() { _mustClose = true; }

        /**
         * @brief Remet ce module a neuf. Appele par le manager a CHAQUE Load.
         *
         * Indispensable, et pas seulement par prudence : sur macOS, dlclose
         * ne decharge tres souvent PAS l'image. dyld la garde mappee, et le
         * dlopen suivant rend le meme handle - donc getModules() rend LE MEME
         * OBJET, a la meme adresse, avec l'etat qu'il avait avant.
         *
         * Sans cette remise a zero, une bibliotheque rechargee revient avec
         * son mustClose() encore vrai : ses detenteurs la lachent aussitot et
         * elle ne peut plus jamais servir.
         *
         * Un module suppose donc rien de sa propre fraicheur - c'est le
         * manager qui la lui donne.
         */
        void reset() { _uses = 0; _mustClose = false; }

    protected:
        /** @brief Pour aller chercher les autres modules. */
        IModuleRegistry *registry() const { return _registry; }

    private:
        IModuleRegistry *_registry = nullptr;

        /* Un entier nu, pas un atomic : sfml comme raylib exigent que le
         * rendu se fasse depuis un seul thread, donc la boucle qui lit et
         * ecrit ces deux champs est sequentielle. Le jour ou un chargement
         * partirait sur un thread de fond, std::atomic<unsigned> suffit et
         * ne change rien d'autre. */
        unsigned _uses = 0;
        bool _mustClose = false;
};

/** @} */

#endif // IMODULE_HPP
