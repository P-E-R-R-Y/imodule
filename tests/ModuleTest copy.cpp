#include <gtest/gtest.h>
#include "../includes/IModule.hpp"

bool test(bool *b) {
    static bool save = false;
    if (b != nullptr) {
        save = *b;
    }
    return save;
}

// Implémentation de test minimale
class TestIModule : public IModule {
public:
    TestIModule(const std::string& name, const std::string& type)
        : name_(name), type_(type) {}

    const std::string& getType() const override { return type_; }
    const std::string& getName() const override { return name_; }
    void update() override {
        bool value = true;
        test(&value);
        return; 
    }

private:
    std::string name_;
    std::string type_;
};

TEST(IModuleTest, InterfaceMethods) {
    TestIModule mod("TestName", "TestType");
    EXPECT_EQ(mod.getName(), "TestName");
    EXPECT_EQ(mod.getType(), "TestType");
    EXPECT_EQ(test(nullptr), false);
    mod.update();
    EXPECT_EQ(test(nullptr), true);
}