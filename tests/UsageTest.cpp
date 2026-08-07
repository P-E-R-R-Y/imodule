#include <gtest/gtest.h>

#include "ModuleManager.hpp"
#include "graphic.hpp"

/**
 * @brief One coherent scenario, real dlls (ray/sfml, built as MODULE
 *        targets by CMake), exercising every public method in the order a
 *        real caller would : load two vendors, read them back all three
 *        ways (by type, by vendor, by row), then unload one and check the
 *        other is untouched.
 */
struct UsageTest : ::testing::Test {
    ModuleManager<IGraphicModule, IWindowModule> modules;
};

TEST_F(UsageTest, FullWalkthroughMatchesRealUsage) {
    ASSERT_TRUE(modules.Load(RAY_PATH, "ray"));
    ASSERT_TRUE(modules.Load(SFML_PATH, "sfml"));

    /* GetAll<T>() : every vendor of one contract. */
    EXPECT_EQ(modules.GetAll<IGraphicModule>().size(), 2u);
    EXPECT_EQ(modules.GetAll<IWindowModule>().size(), 1u); /* sfml doesn't provide it */

    /* GetAll(vendor) : everything one vendor provides. */
    EXPECT_EQ(modules.GetAll("ray").size(), 2u);
    EXPECT_EQ(modules.GetAll("sfml").size(), 1u);

    /* Get<T>(name) : one, by vendor name. */
    IGraphicModule *rayGraphic = modules.Get<IGraphicModule>("ray");
    ASSERT_NE(rayGraphic, nullptr);
    EXPECT_STREQ(rayGraphic->name(), "ray");
    EXPECT_EQ(modules.Get<IWindowModule>("sfml"), nullptr);

    /* Key(name) -> Entity, then Get(e) / Get<T>(e) : the same row, by index. */
    auto ray = modules.Key("ray");
    ASSERT_TRUE(ray.has_value());
    ASSERT_NE(modules.Get(*ray), nullptr);
    EXPECT_EQ(modules.Get(*ray)->path(), RAY_PATH);
    EXPECT_EQ(modules.Get<IGraphicModule>(*ray), rayGraphic);

    /* Unload("ray") : its modules AND its dll go away, sfml is untouched. */
    modules.Unload("ray");
    EXPECT_FALSE(modules.Key("ray").has_value());
    EXPECT_EQ(modules.Get<IGraphicModule>("ray"), nullptr);
    EXPECT_EQ(modules.GetAll<IGraphicModule>().size(), 1u);
    EXPECT_NE(modules.Get<IGraphicModule>("sfml"), nullptr);
}
