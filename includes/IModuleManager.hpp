/**
 * @file IModuleManager.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief La table de tous les modules charges.
 *
 * @addtogroup imodule
 * @{
 */

#ifndef IMODULE_MANAGER_HPP
#define IMODULE_MANAGER_HPP

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "IModule.hpp"
#include "SharedLibrary.hpp"
#include "Stride.hpp"

/**
 * @class IModuleManager
 * @brief Une ligne par contrat, une colonne par origine.
 *
 *                | raylib             | sfml              | asio
 *     -----------|--------------------|-------------------|-------------------
 *      graphic   | RayGraphicModule   | SfmlGraphicModule |
 *      audio     | RayAudioModule     | SfmlAudioModule   |
 *      network   |                    |                   | AsioNetworkModule
 *
 * Une colonne vient d'une bibliotheque (Load()) ou de l'hote lui-meme
 * (add()). Elle se lit par contrat, par origine, ou case par case.
 *
 * Aucun choix n'est impose : chaque invite lit la table et tente
 * d'acquerir ce qu'il veut. La seule chose qui puisse le refuser est une
 * ressource materielle deja prise (IModule::claims()).
 *
 * C'est ce que bind() pose sur chaque module : de la ou il est, un module
 * peut trouver ses voisins, jamais fermer une bibliotheque.
 */
class IModuleManager {

    public:
        /** @brief Une ligne ou une colonne de la table. Contient les cases vides. */
        using Span = typename Stride<IModule *>::Span;

        /**
         * @brief Le seul symbole qu'une bibliotheque doit exporter.
         *
         * @code
         * extern "C" IModule **getModules() {
         *     static SfmlGraphicModule graphic;
         *     static SfmlAudioModule audio;
         *     static IModule *list[] = { &graphic, &audio, nullptr };
         *     return list;
         * }
         * @endcode
         *
         * Un IModule ** termine par nullptr : deux pointeurs traversent un
         * dlopen sans rien supposer de l'ABI d'en face.
         */
        static constexpr const char *entry = "getModules";

        ~IModuleManager() {
            for (const auto &[key, library] : _libraries)
                if (!library)
                    for (IModule *module : _table.column(key))
                        delete module;
            _table = Stride<IModule *>{};
            _libraries.clear();
        }

        /**
         * @brief Ouvre une bibliotheque et ajoute les modules qu'elle exporte.
         *
         * @param path
         * @param key le nom de la colonne
         * @return false si la cle est prise, si getModules() manque, ou si
         *         la bibliotheque ne fournit rien
         */
        bool Load(const std::string &path, const std::string &key) {
            if (_table.hasColumn(key))
                return false;

            auto library = std::make_unique<SharedLibrary>(path);
            auto get = library->symbol<IModule **(*)()>(entry);

            if (!get)
                return false;

            IModule **modules = get();

            if (!modules || !*modules)
                return false;

            std::vector<IModule *> raws;
            for (IModule **it = modules; *it; it++)
                raws.push_back(*it);

            placeColumn(key, raws);
            _libraries.emplace(key, std::move(library));
            return true;
        }

        /**
         * @brief Ajoute une colonne sans bibliotheque. Le manager prend
         *        possession des modules.
         *
         * Pour un module compile dans l'hote. L'appelant lache son
         * unique_ptr : il ne peut plus le detruire lui-meme.
         *
         * @param key     le nom de la colonne
         * @param modules au moins un
         * @return false si la cle est prise ou si modules est vide
         */
        bool add(const std::string &key, std::vector<std::unique_ptr<IModule>> modules) {
            if (_table.hasColumn(key))
                return false;
            if (modules.empty())
                return false;

            std::vector<IModule *> raws;
            raws.reserve(modules.size());
            for (auto &module : modules)
                raws.push_back(module.release());

            placeColumn(key, raws);
            _libraries.emplace(key, nullptr);
            return true;
        }

        /** @brief Comme add(key, vector<...>), pour un seul module. */
        bool add(const std::string &key, std::unique_ptr<IModule> module) {
            std::vector<std::unique_ptr<IModule>> modules;

            modules.push_back(std::move(module));
            return add(key, std::move(modules));
        }

        /**
         * @brief Une case : ce contrat, chez ce fournisseur.
         *
         * @param type
         * @param key
         * @return IModule* nullptr si ce fournisseur ne remplit pas ce contrat
         */
        IModule *Get(const std::string &type, const std::string &key) {
            return _table.at(type, key);
        }

        /**
         * @brief La ligne : une case par fournisseur, pour ce contrat.
         *
         * Les fournisseurs qui ne le remplissent pas rendent nullptr.
         *
         *     for (IModule *m : modules.GetAllByType("graphic2"))
         *         if (m && m->acquire()) { ...; break; }
         *
         * @param type
         * @return Span
         */
        Span GetAllByType(const std::string &type) {
            return _table.row(type);
        }

        /**
         * @brief La colonne : une case par contrat, pour ce fournisseur.
         *
         * Les contrats qu'il ne remplit pas rendent nullptr.
         *
         * @param key
         * @return Span
         */
        Span GetAllByKey(const std::string &key) {
            return _table.column(key);
        }

        /** @brief Toute la table, cases vides comprises. */
        Span GetAll() {
            return _table.all();
        }

        /**
         * @brief Declarer un detenteur de plus sur ce module.
         *
         * @param module
         * @return false si une de ses claims() est deja prise
         */
        bool acquire(IModule *module) {
            if (!takeClaims(module))
                return false;
            module->_uses++;
            return true;
        }

        /**
         * @brief Un detenteur de moins. Libere ses claims() au dernier.
         *
         * @param module
         */
        void release(IModule *module) {
            if (module->_uses)
                module->_uses--;
            if (module->isClosed())
                freeClaims(module);
        }

        /**
         * @brief Condamne une colonne : ses detenteurs doivent lacher.
         *
         * Ne ferme rien. Tout ce que ces modules ont fabrique reste valide
         * jusqu'a Reconcile().
         *
         * @param key
         */
        void Unload(const std::string &key) {
            for (IModule *module : _table.column(key))
                if (module)
                    module->condemn();
        }

        /**
         * @brief Ferme les colonnes condamnees que plus personne ne tient.
         *
         * A appeler a chaque tick. Une colonne encore tenue est laissee
         * telle quelle : on repassera.
         *
         * @return combien ont ete fermees
         */
        size_t Reconcile() {
            return _table.eraseColumnsIf([this](const std::string &key, Span column) {
                if (!isCondemned(column) || !isFree(column))
                    return false;

                const auto library = _libraries.find(key);

                if (library != _libraries.end() && !library->second)
                    for (IModule *module : column)
                        delete module;

                _libraries.erase(key);
                return true;
            });
        }

        /**
         * @brief Ce contrat chez ce fournisseur, avec son vrai type.
         *
         * Parcourt T::accepts : demander un IGraphic2Module trouve aussi un
         * vendor qui ne declare que "graphic3".
         *
         * @param key
         * @return T* nullptr si absent
         */
        template <typename T>
        T *Get(const std::string &key) {
            for (const char *const *type = T::accepts; *type; type++)
                if (IModule *module = Get(*type, key))
                    return static_cast<T *>(module);
            return nullptr;
        }

        /**
         * @brief Tous ceux qui remplissent ce contrat, avec leur vrai type.
         *
         * Rend un vecteur, et non un Span : plusieurs lignes (T::accepts)
         * sont agregees en une.
         *
         * @return std::vector<T *>
         */
        template <typename T>
        std::vector<T *> GetAllByType() {
            std::vector<T *> found;

            for (const char *const *type = T::accepts; *type; type++)
                for (IModule *module : GetAllByType(*type))
                    if (module)
                        found.push_back(static_cast<T *>(module));
            return found;
        }

        /** @brief Les contrats presents, dans l'ordre de decouverte. */
        const std::vector<std::string> &GetTypes() const { return _table.rows(); }

        /** @brief Les origines chargees, dans l'ordre d'arrivee. */
        const std::vector<std::string> &GetKeys() const { return _table.columnNames(); }

    private:
        /** @brief Range une colonne : bind(), reset(), mise en table. */
        void placeColumn(const std::string &key, const std::vector<IModule *> &modules) {
            _table.addColumn(key);

            for (IModule *module : modules) {
                /* Le manager avant la mise en table : le module doit pouvoir
                 * chercher ses voisins des qu'il est atteignable. */
                module->bind(*this);
                module->reset();
                _table.set(module->type(), key, module);
            }
        }

        /**
         * @brief Marque les claims() de ce module, si toutes sont libres.
         *
         * @return false si l'une est prise ; rien n'est alors marque
         */
        bool takeClaims(IModule *module) {
            for (const char *const *c = module->claims(); *c; c++)
                if (_claimed.count(*c))
                    return false;
            for (const char *const *c = module->claims(); *c; c++)
                _claimed.insert(*c);
            return true;
        }

        /** @brief Rend ses claims() disponibles. */
        void freeClaims(IModule *module) {
            for (const char *const *c = module->claims(); *c; c++)
                _claimed.erase(*c);
        }

        /** @brief Au moins un module de cette colonne est condamne. */
        static bool isCondemned(Span column) {
            for (IModule *module : column)
                if (module && module->mustClose())
                    return true;
            return false;
        }

        /** @brief Plus aucun module de cette colonne n'est tenu. */
        static bool isFree(Span column) {
            for (IModule *module : column)
                if (module && !module->isClosed())
                    return false;
            return true;
        }

        Stride<IModule *> _table;

        /* Par cle, comme la table : la bibliotheque a fermer, ou nullptr
         * quand la colonne vient d'add() et que les modules sont a nous. */
        std::unordered_map<std::string, std::unique_ptr<SharedLibrary>> _libraries;

        std::set<std::string> _claimed;   ///< ressources materielles prises
};

/* Definis ici : IModule ne connait IModuleManager que par declaration
 * anticipee. */
inline bool IModule::acquire() { return _manager->acquire(this); }
inline void IModule::release() { _manager->release(this); }

/** @} */

#endif /* !IMODULE_MANAGER_HPP */
