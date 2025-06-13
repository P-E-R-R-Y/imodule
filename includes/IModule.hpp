
#include <string>

#ifndef IMODULE_HPP
#define IMODULE_HPP

class IModule {
    public:

        virtual ~IModule() = default;

        virtual const std::string getType() const = 0;

        virtual const std::string getName() const = 0;
};

#endif // MODULE_HPP