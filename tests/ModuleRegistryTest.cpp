#include <gtest/gtest.h>
#include "../includes/IModule.hpp"

// Implémentation factice de IModule
class DummyModule : public IModule {
public:
    DummyModule(std::string name, std::string type)
        : name_(std::move(name)), type_(std::move(type)) {}

    const std::string getType() const override { return type_; }
    const std::string getName() const override { return name_; }
    void update() override { };

    bool hasRegistry() const { return registry != nullptr; }

private:
    std::string name_;
    std::string type_;
};

TEST(ModuleRegistryTest, AddAndGetActive) {
    ModuleRegistry registry;
    DummyModule mod("ModuleA", "typeX");
    DummyModule mod2("ModuleB", "typeX");

    registry.add(&mod);
    registry.add(&mod2);

    auto* found = registry.getActive("typeX");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "ModuleA");
    EXPECT_EQ(found->getType(), "typeX");
    EXPECT_TRUE(mod.hasRegistry());
}
TEST(ModuleRegistryTest, Get) {
    ModuleRegistry registry;
    DummyModule m1("M1", "typeA");
    DummyModule m2("M2", "typeA");

    registry.add(&m1);
    registry.add(&m2);

    auto modules = registry.get("typeA");
    EXPECT_EQ(modules.size(), 2);
}

TEST(ModuleRegistryTest, GetUnknown) {
    ModuleRegistry registry;

    EXPECT_EQ(registry.getActive("UnknownType"), nullptr);
    EXPECT_TRUE(registry.get("UnknownType").empty());
}