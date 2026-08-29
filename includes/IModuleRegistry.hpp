/**
 * @file IModuleRegistry.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief Ce qu'un module a le droit de demander sur les autres.
 * @date 2026-08-11
 *
 * @addtogroup imodule
 * @{
 */

#ifndef IMODULE_REGISTRY_HPP
#define IMODULE_REGISTRY_HPP

#include <string>
#include <vector>

#include "IModule.hpp"

/**
 * @interface IModuleRegistry
 * @brief La table, en lecture seule.
 *
 * C'est ce que le manager pose sur chaque module au chargement. Il n'y a
 * ni Load ni Unload ici, et c'est tout l'interet : un module peut trouver
 * ses voisins, il ne peut pas fermer la bibliotheque dans laquelle il
 * tourne.
 *
 * Pas d'Acquire ni de Release non plus. Le compteur vit dans IModule, donc
 * on le prend en s'adressant a lui :
 *
 *     IModule *gfx = registry->Get("graphic2", "sfml");
 *     if (gfx) gfx->acquire();
 *     ...
 *     gfx->release();
 *
 * Passer par le registre n'ajouterait qu'un detour.
 *
 * LIVE, pas un instantane : redemande et tu vois ce qui a change. C'est ce
 * qui rend le remplacement a chaud possible - on relit au lieu de garder un
 * pointeur qui a ete libere dessous.
 */
class IModuleRegistry {

    public:
        virtual ~IModuleRegistry() = default;

        /**
         * @brief La case : ce contrat, chez ce fournisseur.
         *
         * @param type la valeur rendue par IModule::type()
         * @param key  le nom sous lequel la dll a ete chargee
         * @return IModule* nullptr si la case est vide - ce n'est pas une
         *         erreur, seulement une capacite que ce fournisseur n'a pas
         */
        virtual IModule *Get(const std::string &type, const std::string &key) = 0;

        /**
         * @brief La ligne : tous ceux qui remplissent ce contrat.
         *
         * @param type
         * @return std::vector<IModule *>
         */
        virtual std::vector<IModule *> GetAllByType(const std::string &type) = 0;

        /**
         * @brief La colonne : tout ce que ce fournisseur apporte.
         *
         * @param key
         * @return std::vector<IModule *>
         */
        virtual std::vector<IModule *> GetAllByKey(const std::string &key) = 0;

        /**
         * @brief Toute la table.
         *
         * @return std::vector<IModule *>
         */
        virtual std::vector<IModule *> GetAll() = 0;

        /**
         * @brief Le module EN SERVICE pour ce contrat, ou nullptr.
         *
         * Un seul a la fois, a tout instant. C'est ce qui remplace la
         * devinette : avant, un invite cherchait "celui qui a une fenetre" et
         * se trompait des qu'il y en avait deux.
         *
         * @param type
         * @return IModule*
         */
        virtual IModule *Current(const std::string &type) = 0;

        /**
         * @brief L'hote declare son choix pour ce contrat.
         *
         * Le TYPE est donne a part, et volontairement : un IGraphic3Module
         * declare "graphic3" alors qu'il remplit aussi "graphic2". L'hote
         * l'inscrit sous la capacite qu'il utilise, pas sous ce que le module
         * dit de lui-meme.
         *
         * @param type
         * @param module nullptr pour n'avoir plus rien en service
         */
        virtual void Select(const std::string &type, IModule *module) = 0;
};

/** @} */

#endif /* !IMODULE_REGISTRY_HPP */
