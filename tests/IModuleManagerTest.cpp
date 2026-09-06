#include <gtest/gtest.h>

#include "IModuleManager.hpp"
#include "graphic.hpp"

#include <stdexcept>

namespace {
/* Un Span rend les trous a nullptr : size() compte les cases, filled()
 * compte ce qui s'y trouve vraiment. */
size_t filled(IModuleManager::Span span) {
    size_t count = 0;

    for (IModule *module : span)
        if (module)
            count++;
    return count;
}
} // namespace

/**
 * @brief Un scenario coherent, deux vraies dll (ray/sfml, construites comme
 *        cibles MODULE par CMake) : charger, lire des deux facons,
 *        decharger, et verifier le conflit reel de claims() entre vendors.
 */
struct IModuleManagerTest : ::testing::Test {
    IModuleManager modules;
};

TEST_F(IModuleManagerTest, LoadDiscoversModules) {
    ASSERT_TRUE(modules.Load(RAY_PATH, "ray"));
    EXPECT_NE(modules.Get("graphic", "ray"), nullptr);
    EXPECT_NE(modules.Get("window", "ray"), nullptr);
}

TEST_F(IModuleManagerTest, LoadingSameKeyTwiceIsRefused) {
    EXPECT_TRUE(modules.Load(RAY_PATH, "ray"));
    EXPECT_FALSE(modules.Load(RAY_PATH, "ray"));
}

/** @brief Un chemin invalide remonte l'echec de SharedLibrary. */
TEST_F(IModuleManagerTest, LoadThrowsOnAMissingLibrary) {
    EXPECT_THROW(modules.Load("./nexiste_pas.dylib", "fantome"), std::runtime_error);
    EXPECT_TRUE(modules.GetAllByKey("fantome").empty());
}

/** @brief La meme dll sous deux cles est deux colonnes. */
TEST_F(IModuleManagerTest, SameLibraryUnderTwoKeysIsTwoColumns) {
    ASSERT_TRUE(modules.Load(RAY_PATH, "ray"));
    ASSERT_TRUE(modules.Load(RAY_PATH, "ray-bis"));

    EXPECT_EQ(filled(modules.GetAllByType("graphic")), 2u);
    EXPECT_EQ(modules.GetKeys().size(), 2u);
}

TEST_F(IModuleManagerTest, GetAllByTypeListsTheRow) {
    modules.Load(RAY_PATH, "ray");
    modules.Load(SFML_PATH, "sfml");

    /* size() compte les cases de la ligne, trous compris : deux colonnes
     * chargees, donc deux cases, meme si sfml n'a pas de window. */
    EXPECT_EQ(modules.GetAllByType("graphic").size(), 2u);
    EXPECT_EQ(modules.GetAllByType("window").size(), 2u);
    EXPECT_EQ(filled(modules.GetAllByType("window")), 1u); // sfml n'en fournit pas
}

TEST_F(IModuleManagerTest, GetAllByKeyListsTheColumn) {
    modules.Load(RAY_PATH, "ray");

    EXPECT_EQ(filled(modules.GetAllByKey("ray")), 2u);
    EXPECT_TRUE(modules.GetAllByKey("sfml").empty());   // jamais chargee
}

TEST_F(IModuleManagerTest, UnloadThenReconcileClosesAFreeColumn) {
    modules.Load(RAY_PATH, "ray");
    modules.Load(SFML_PATH, "sfml");

    modules.Unload("ray");
    EXPECT_EQ(modules.Reconcile(), 1u);

    EXPECT_EQ(modules.Get("graphic", "ray"), nullptr);
    EXPECT_NE(modules.Get("graphic", "sfml"), nullptr);
}

TEST_F(IModuleManagerTest, ReconcileWaitsWhileAModuleIsHeld) {
    modules.Load(RAY_PATH, "ray");
    IModule *graphic = modules.Get("graphic", "ray");
    ASSERT_TRUE(graphic->acquire());

    modules.Unload("ray");
    EXPECT_EQ(modules.Reconcile(), 0u); // encore tenu, on repasse au tick suivant

    graphic->release();
    EXPECT_EQ(modules.Reconcile(), 1u);
}

/* ---- les memes questions, posees avec un type ---------------------- */

TEST_F(IModuleManagerTest, TypedGetReturnsTheContract) {
    modules.Load(RAY_PATH, "ray");
    modules.Load(SFML_PATH, "sfml");

    EXPECT_NE(modules.Get<GraphicModule>("ray"), nullptr);
    EXPECT_NE(modules.Get<WindowModule>("ray"), nullptr);
    EXPECT_EQ(modules.Get<WindowModule>("sfml"), nullptr);   // sfml n'en a pas
    EXPECT_EQ(modules.Get<GraphicModule>("inconnu"), nullptr);
}

/** @brief La version typee agrege les lignes et saute les cases vides. */
TEST_F(IModuleManagerTest, TypedGetAllByTypeSkipsTheHoles) {
    modules.Load(RAY_PATH, "ray");
    modules.Load(SFML_PATH, "sfml");

    EXPECT_EQ(modules.GetAllByType<GraphicModule>().size(), 2u);
    EXPECT_EQ(modules.GetAllByType<WindowModule>().size(), 1u);
}

TEST_F(IModuleManagerTest, ListsTypesAndKeys) {
    modules.Load(RAY_PATH, "ray");
    modules.Load(SFML_PATH, "sfml");

    EXPECT_EQ(modules.GetTypes().size(), 2u);   // graphic, window
    EXPECT_EQ(modules.GetKeys().size(), 2u);    // ray, sfml
}

/* ---- claims : conflit reel entre deux vendors OpenGL --------------- */

TEST_F(IModuleManagerTest, ConflictingClaimAcrossVendorsRefusesAcquire) {
    modules.Load(RAY_PATH, "ray");
    modules.Load(SFML_PATH, "sfml");

    IModule *ray = modules.Get("graphic", "ray");
    IModule *sfml = modules.Get("graphic", "sfml");

    ASSERT_TRUE(ray->acquire());
    EXPECT_FALSE(sfml->acquire());

    ray->release();
    EXPECT_TRUE(sfml->acquire());
}
