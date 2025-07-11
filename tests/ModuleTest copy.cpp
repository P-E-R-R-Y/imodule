#include <gtest/gtest.h>
#include "../includes/IModule.hpp"

// Implémentation de test minimale
class TestIModule : public IModule {
public:
    TestIModule(const std::string& name, const std::string& type)
        : name_(name), type_(type) {}

    const std::string getType() const override { return type_; }
    const std::string getName() const override { return name_; }

private:
    std::string name_;
    std::string type_;
};

TEST(IModuleTest, InterfaceMethods) {
    TestIModule mod("TestName", "TestType");
    EXPECT_EQ(mod.getName(), "TestName");
    EXPECT_EQ(mod.getType(), "TestType");
}