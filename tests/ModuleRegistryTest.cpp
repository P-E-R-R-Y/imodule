#include <gtest/gtest.h>
#include "../includes/IModule.hpp"

// Implémentation factice de IModule
class DummyModule : public IModule {
public:
    DummyModule(std::string name, std::string type)
        : name_(std::move(name)), type_(std::move(type)) {}

    const std::string getType() const override { return type_; }
    const std::string getName() const override { return name_; }

    bool hasRegistry() const { return registry != nullptr; }

private:
    std::string name_;
    std::string type_;
};

TEST(ModuleRegistryTest, AddAndGetByName) {
    ModuleRegistry registry;
    DummyModule mod("ModuleA", "typeX");

    registry.addModule(&mod);

    auto* found = registry.getModuleByName("ModuleA");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "ModuleA");
    EXPECT_EQ(found->getType(), "typeX");
    EXPECT_TRUE(mod.hasRegistry());
}

TEST(ModuleRegistryTest, GetByType) {
    ModuleRegistry registry;
    DummyModule m1("M1", "typeA");
    DummyModule m2("M2", "typeA");

    registry.addModule(&m1);
    registry.addModule(&m2);

    auto modules = registry.getModulesByType("typeA");
    EXPECT_EQ(modules.size(), 2);
}

TEST(ModuleRegistryTest, DuplicateNameThrows) {
    ModuleRegistry registry;
    DummyModule m1("SameName", "type1");
    DummyModule m2("SameName", "type2");

    registry.addModule(&m1);
    EXPECT_THROW(registry.addModule(&m2), std::runtime_error);
}

TEST(ModuleRegistryTest, GetUnknownReturnsNull) {
    ModuleRegistry registry;

    EXPECT_EQ(registry.getModuleByName("Unknown"), nullptr);
    EXPECT_TRUE(registry.getModulesByType("UnknownType").empty());
}