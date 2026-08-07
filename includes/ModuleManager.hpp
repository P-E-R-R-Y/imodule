#ifndef MODULE_MANAGER_HPP
#define MODULE_MANAGER_HPP

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "IModule.hpp"
#include "SharedLibrary.hpp"
#include "Stride.hpp"

/**
 * @brief A row's identity in ModuleManager's Stride : where the loaded
 *        SharedLibrary lives. remove() resetting this closes the dll.
 */
struct ModuleLibrary {
    std::unique_ptr<SharedLibrary> library;
};

/**
 * @class ModuleManager
 * @brief A Stride<ModuleLibrary, Ts...> : a loaded dll IS a row, a contract
 *        (Ts...) IS a column, the row's identity IS the dll itself. No
 *        index/library link to keep in sync by hand — Stride owns both.
 *
 *     ModuleManager<IGraphicModule, IWindowModule> modules;
 *
 *     modules.Load("./ray.so", "ray");
 *     modules.GetAll<IGraphicModule>();   // every vendor of this contract
 *     modules.GetAll("ray");              // everything "ray" provides
 *     modules.Get<IGraphicModule>("ray"); // one, by vendor name
 *     modules.Get<IGraphicModule>(e);     // one, by row
 *     modules.Get(e);                     // the SharedLibrary of this row
 *     modules.Unload("ray");              // clears the row, closes the dll
 */
template <typename... Ts>
class ModuleManager : public Stride<ModuleLibrary, Ts *...> {
        using Base = Stride<ModuleLibrary, Ts *...>;

    public:
        using typename Base::Entity;

        /** @brief Opens the dll, adds its row, fills its columns. */
        bool Load(const std::string &path, std::string key = "") {
            if (key.empty())
                key = path;
            if (_byKey.count(key))
                return false;
            auto lib = std::make_unique<SharedLibrary>(path);
            SharedLibrary &ref = *lib;
            Entity e = this->add(ModuleLibrary{std::move(lib)});
            (Discover<Ts>(ref, e), ...);
            _byKey.emplace(key, e);
            return true;
        }

        /** @brief Forgets this row's modules, THEN closes its dll. One call. */
        void Unload(const std::string &key) {
            auto it = _byKey.find(key);
            if (it == _byKey.end())
                return;
            (Forget<Ts>(it->second), ...);
            this->remove(it->second); /* clears columns AND identity -> closes the dll */
            _byKey.erase(it);
        }

        /** @brief A module of T DISPONIBLE by vendor name, or nullptr. Table a cote. */
        template <typename T>
        T *Get(const std::string &name) const {
            auto &byName = std::get<ByName<T>>(_byName);
            auto it = byName.find(name);
            return it == byName.end() ? nullptr : it->second;
        }

        /** @brief The T module of this exact row, or nullptr. */
        template <typename T>
        T *Get(Entity e) {
            auto &slot = this->template at<T *>(e);
            return slot ? *slot : nullptr;
        }

        /** @brief The SharedLibrary of this exact row, or nullptr. */
        SharedLibrary *Get(Entity e) {
            auto &id = this->identity(e);
            return id ? id->library.get() : nullptr;
        }

        /** @brief The row of this key, so it can be handed to Get(Entity) / Get<T>(Entity). */
        std::optional<Entity> Key(const std::string &key) const {
            auto it = _byKey.find(key);
            return it == _byKey.end() ? std::nullopt : std::optional<Entity>(it->second);
        }

        /** @brief Every vendor of T. */
        template <typename T>
        std::vector<T *> GetAll() {
            std::vector<T *> out;
            for (auto &slot : this->template column<T *>())
                if (slot)
                    out.push_back(*slot);
            return out;
        }

        /** @brief Every module (any contract) this vendor provides. */
        std::vector<IModule *> GetAll(const std::string &vendorName) const {
            auto it = _byVendor.find(vendorName);
            return it == _byVendor.end() ? std::vector<IModule *>{} : it->second;
        }

    private:
        template <typename T>
        using ByName = std::unordered_map<std::string, T *>;

        template <typename T>
        void Discover(SharedLibrary &lib, Entity e) {
            auto entry = lib.symbol<T *(*)()>(T::entry);
            if (!entry)
                return;
            T *module = entry();
            this->template set<T *>(e, module);

            const std::string name = module->name();
            std::get<ByName<T>>(_byName)[name] = module;
            _byVendor[name].push_back(module);
        }

        template <typename T>
        void Forget(Entity e) {
            auto &slot = this->template at<T *>(e);
            if (!slot)
                return;
            T *module = *slot;
            const std::string name = module->name();
            std::get<ByName<T>>(_byName).erase(name);

            auto &vec = _byVendor[name];
            vec.erase(std::remove(vec.begin(), vec.end(), static_cast<IModule *>(module)), vec.end());
            if (vec.empty())
                _byVendor.erase(name);
        }

        std::tuple<ByName<Ts>...> _byName{};
        std::unordered_map<std::string, std::vector<IModule *>> _byVendor;
        std::unordered_map<std::string, Entity> _byKey;
};

#endif /* !MODULE_MANAGER_HPP */
