/**
 * @file DummyRegistry.hpp
 * @brief Un registre de comptoir : une liste, et rien d'autre.
 *
 * Il sert a verifier CE QUE VOIT UN INVITE. Le vrai registre indexe par
 * table et ferme des dll ; celui-ci se contente de repondre aux six
 * questions du contrat, ce qui suffit a dire si le contrat est utilisable.
 */

#ifndef DUMMYREGISTRY_HPP_
#define DUMMYREGISTRY_HPP_

#include "IModuleRegistry.hpp"

#include <map>
#include <string>
#include <vector>

class DummyRegistry : public IModuleRegistry {

    public:
        /** @brief Ajoute un module sous cette cle de chargement. */
        void add(const std::string &key, IModule *module) {
            _held.push_back({key, module});
        }

        IModule *Get(const std::string &type, const std::string &key) override {
            for (const Cell &cell : _held)
                if (cell.key == key && type == cell.module->type())
                    return cell.module;
            return nullptr;
        }

        std::vector<IModule *> GetAllByType(const std::string &type) override {
            std::vector<IModule *> found;

            for (const Cell &cell : _held)
                if (type == cell.module->type())
                    found.push_back(cell.module);
            return found;
        }

        std::vector<IModule *> GetAllByKey(const std::string &key) override {
            std::vector<IModule *> found;

            for (const Cell &cell : _held)
                if (cell.key == key)
                    found.push_back(cell.module);
            return found;
        }

        std::vector<IModule *> GetAll() override {
            std::vector<IModule *> found;

            for (const Cell &cell : _held)
                found.push_back(cell.module);
            return found;
        }

        IModule *Current(const std::string &type) override {
            const auto found = _current.find(type);

            return found == _current.end() ? nullptr : found->second;
        }

        void Select(const std::string &type, IModule *module) override {
            if (module)
                _current[type] = module;
            else
                _current.erase(type);
        }

    private:
        struct Cell { std::string key; IModule *module; };

        std::vector<Cell> _held;
        std::map<std::string, IModule *> _current;
};

#endif /* !DUMMYREGISTRY_HPP_ */
