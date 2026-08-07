#include <gtest/gtest.h>

#include "ModuleManager.hpp"
#include "graphic.hpp"

struct ModuleManagerTest : ::testing::Test {
    ModuleManager<IGraphicModule, IWindowModule> modules;
};

TEST_F(ModuleManagerTest, LoadDiscoversModules) {
    modules.Load(RAY_PATH);
    EXPECT_NE(modules.Get<IGraphicModule>("ray"), nullptr);
}

TEST_F(ModuleManagerTest, LoadingSameKeyTwiceIsRefused) {
    EXPECT_TRUE(modules.Load(RAY_PATH));
    EXPECT_FALSE(modules.Load(RAY_PATH));
}

TEST_F(ModuleManagerTest, LoadWithCustomKey) {
    modules.Load(RAY_PATH, "ray");
    EXPECT_NE(modules.Get<IGraphicModule>("ray"), nullptr);
}

TEST_F(ModuleManagerTest, GetByUnknownNameIsNull) {
    modules.Load(RAY_PATH);
    EXPECT_EQ(modules.Get<IGraphicModule>("vulkan"), nullptr);
}

TEST_F(ModuleManagerTest, PartialCoverageIsNotAnError) {
    modules.Load(SFML_PATH); /* ne fournit pas window */
    EXPECT_NE(modules.Get<IGraphicModule>("sfml"), nullptr);
    EXPECT_EQ(modules.Get<IWindowModule>("sfml"), nullptr);
}

TEST_F(ModuleManagerTest, GetAllByTypeListsEveryVendor) {
    modules.Load(RAY_PATH);
    modules.Load(SFML_PATH);
    EXPECT_EQ(modules.GetAll<IGraphicModule>().size(), 2u);
}

TEST_F(ModuleManagerTest, GetAllByVendorNameListsEveryContractItProvides) {
    modules.Load(RAY_PATH); /* graphic + window */
    EXPECT_EQ(modules.GetAll("ray").size(), 2u);
}

TEST_F(ModuleManagerTest, GetAllByVendorNameIsEmptyForUnknownVendor) {
    modules.Load(RAY_PATH);
    EXPECT_TRUE(modules.GetAll("vulkan").empty());
}

TEST_F(ModuleManagerTest, UnloadRemovesModulesThenClosesTheLibraryInOneCall) {
    modules.Load(SFML_PATH);

    modules.Unload(SFML_PATH);

    EXPECT_EQ(modules.Get<IGraphicModule>("sfml"), nullptr);
    EXPECT_TRUE(modules.GetAll("sfml").empty());
    EXPECT_TRUE(modules.GetAll<IGraphicModule>().empty());
}

TEST_F(ModuleManagerTest, UnloadingOneVendorLeavesTheOtherIntact) {
    modules.Load(RAY_PATH);
    modules.Load(SFML_PATH);

    modules.Unload(RAY_PATH);

    EXPECT_EQ(modules.Get<IGraphicModule>("ray"), nullptr);
    EXPECT_NE(modules.Get<IGraphicModule>("sfml"), nullptr);
}

TEST_F(ModuleManagerTest, UnloadByCustomKey) {
    modules.Load(RAY_PATH, "ray");
    modules.Unload("ray");
    EXPECT_EQ(modules.Get<IGraphicModule>("ray"), nullptr);
}

TEST_F(ModuleManagerTest, GetByEntityReturnsTheModuleOfThatRow) {
    modules.Load(RAY_PATH, "ray");
    auto e = modules.Key("ray");
    ASSERT_TRUE(e.has_value());
    EXPECT_NE(modules.Get<IGraphicModule>(*e), nullptr);
    EXPECT_EQ(modules.Get<IGraphicModule>(*e), modules.Get<IGraphicModule>("ray"));
}

TEST_F(ModuleManagerTest, GetByEntityReturnsTheSharedLibraryOfThatRow) {
    modules.Load(RAY_PATH, "ray");
    auto e = modules.Key("ray");
    ASSERT_TRUE(e.has_value());
    ASSERT_NE(modules.Get(*e), nullptr);
    EXPECT_EQ(modules.Get(*e)->path(), RAY_PATH);
}

TEST_F(ModuleManagerTest, KeyIsGoneAfterUnload) {
    modules.Load(RAY_PATH, "ray");
    modules.Unload("ray");
    EXPECT_FALSE(modules.Key("ray").has_value());
}
