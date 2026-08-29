/**
 * @file TestModule.cpp
 * @brief Le protocole de vie d'un module, sans une seule dll.
 *
 * Tout ce que IModule apporte est du comptage et un drapeau. Ca se teste
 * donc a plat - et il FAUT que ce soit teste a plat, parce que les bugs
 * qu'on y a trouves ne venaient jamais du chargement mais toujours de
 * l'ordre des appels.
 */

#include <gtest/gtest.h>

#include "DummyModule.hpp"
#include "DummyRegistry.hpp"

/* ---- le compteur d'usage ----------------------------------------- */

TEST(ModuleUses, StartsFree)
{
    DummyModule module("graphic2", "faux");

    EXPECT_EQ(module.uses(), 0u);
    EXPECT_TRUE(module.isClosed());
    EXPECT_FALSE(module.mustClose());
}

TEST(ModuleUses, CountsHolders)
{
    DummyModule module("graphic2", "faux");

    module.acquire();
    module.acquire();
    EXPECT_EQ(module.uses(), 2u);
    EXPECT_FALSE(module.isClosed());

    module.release();
    EXPECT_EQ(module.uses(), 1u);
    EXPECT_FALSE(module.isClosed());

    module.release();
    EXPECT_EQ(module.uses(), 0u);
    EXPECT_TRUE(module.isClosed());
}

/**
 * @brief Relacher plus qu'on n'a pris ne descend pas sous zero.
 *
 * Un unsigned qui passe sous zero remonte a quatre milliards, et la dll ne
 * pourrait plus JAMAIS se fermer. Le plancher est ce qui rend un release()
 * en trop benin plutot que definitif.
 */
TEST(ModuleUses, NeverUnderflows)
{
    DummyModule module("graphic2", "faux");

    module.release();
    module.release();
    EXPECT_EQ(module.uses(), 0u);
    EXPECT_TRUE(module.isClosed());
}

/* ---- la condamnation --------------------------------------------- */

/**
 * @brief Condamne n'est pas ferme, et c'est tout l'interet.
 *
 * Le drapeau est le SIGNAL : "lache-moi". Le compteur est la CONDITION :
 * "plus personne ne me tient". Tant que le second n'est pas rempli, tout ce
 * que le module a fabrique reste valide - c'est ce delai qui laisse a un
 * detenteur le temps de rendre ses objets proprement.
 */
TEST(ModuleCondemn, StaysUsableWhileHeld)
{
    DummyModule module("graphic2", "faux");

    module.acquire();
    module.condemn();

    EXPECT_TRUE(module.mustClose());
    EXPECT_FALSE(module.isClosed());   //il tient encore

    module.release();
    EXPECT_TRUE(module.mustClose());
    EXPECT_TRUE(module.isClosed());    //maintenant on peut fermer
}

/**
 * @brief reset() rend un module a neuf, meme s'il a deja vecu.
 *
 * Indispensable et pas seulement par prudence : sur macOS dlclose ne
 * decharge tres souvent PAS l'image, donc le dlopen suivant rend le MEME
 * objet avec l'etat qu'il avait. Sans cette remise a zero, une
 * bibliotheque rechargee revient condamnee et ne peut plus jamais servir.
 */
TEST(ModuleCondemn, ResetMakesItReusable)
{
    DummyModule module("graphic2", "faux");

    module.acquire();
    module.condemn();
    ASSERT_TRUE(module.mustClose());

    module.reset();

    EXPECT_FALSE(module.mustClose());
    EXPECT_EQ(module.uses(), 0u);
    EXPECT_TRUE(module.isClosed());
}

/* ---- le registre pose par le manager ------------------------------ */

TEST(ModuleBind, RemembersItsRegistry)
{
    DummyModule module("graphic2", "faux");
    DummyRegistry registry;

    EXPECT_EQ(module.seen(), nullptr);
    module.bind(registry);
    EXPECT_EQ(module.seen(), &registry);
}

/* ---- ce que voit un invite ---------------------------------------- */

class RegistryView : public ::testing::Test {

    protected:
        void SetUp() override
        {
            registry.add("faux_impl", &graphic);
            registry.add("faux_impl", &audio);
            registry.add("autre_impl", &other);
        }

        DummyRegistry registry;
        DummyModule graphic{"graphic2", "faux"};
        DummyModule audio{"audio", "faux"};
        DummyModule other{"graphic2", "autre"};
};

TEST_F(RegistryView, FindsByTypeAndKey)
{
    EXPECT_EQ(registry.Get("graphic2", "faux_impl"), &graphic);
    EXPECT_EQ(registry.Get("audio", "faux_impl"), &audio);
    EXPECT_EQ(registry.Get("graphic2", "autre_impl"), &other);
    EXPECT_EQ(registry.Get("audio", "autre_impl"), nullptr);
}

TEST_F(RegistryView, ListsARow)
{
    EXPECT_EQ(registry.GetAllByType("graphic2").size(), 2u);
    EXPECT_EQ(registry.GetAllByType("audio").size(), 1u);
    EXPECT_TRUE(registry.GetAllByType("game").empty());
}

TEST_F(RegistryView, ListsAColumn)
{
    EXPECT_EQ(registry.GetAllByKey("faux_impl").size(), 2u);
    EXPECT_EQ(registry.GetAllByKey("autre_impl").size(), 1u);
    EXPECT_TRUE(registry.GetAllByKey("inconnu").empty());
}

TEST_F(RegistryView, ListsEverything)
{
    EXPECT_EQ(registry.GetAll().size(), 3u);
}

/**
 * @brief Personne en service tant que l'hote n'a pas choisi.
 *
 * C'est ce nullptr qui permet a un invite de LACHER sans se rebrancher :
 * pendant une bascule, l'hote met nullptr, l'invite le lit, rend ses objets
 * au vendor encore vivant, et attend. Sans cet etat, il sauterait aussitot
 * sur le suivant pendant que l'ancien respire encore.
 */
TEST_F(RegistryView, NothingIsCurrentUntilChosen)
{
    EXPECT_EQ(registry.Current("graphic2"), nullptr);

    registry.Select("graphic2", &graphic);
    EXPECT_EQ(registry.Current("graphic2"), &graphic);

    registry.Select("graphic2", nullptr);
    EXPECT_EQ(registry.Current("graphic2"), nullptr);
}

TEST_F(RegistryView, OneCurrentPerContract)
{
    registry.Select("graphic2", &graphic);
    registry.Select("audio", &audio);

    EXPECT_EQ(registry.Current("graphic2"), &graphic);
    EXPECT_EQ(registry.Current("audio"), &audio);

    /* Changer l'un ne touche pas l'autre : c'est ce qui permet de prendre
     * sa fenetre chez un vendor et ses sons chez un autre. */
    registry.Select("graphic2", &other);
    EXPECT_EQ(registry.Current("graphic2"), &other);
    EXPECT_EQ(registry.Current("audio"), &audio);
}

/**
 * @brief Une selection ne prend RIEN : c'est au detenteur d'acquerir.
 *
 * Select ne fait que declarer un choix. Si elle incrementait le compteur,
 * l'hote deviendrait detenteur sans le savoir et aucune dll ne pourrait
 * plus se fermer.
 */
TEST_F(RegistryView, SelectingDoesNotHold)
{
    registry.Select("graphic2", &graphic);

    EXPECT_EQ(graphic.uses(), 0u);
    EXPECT_TRUE(graphic.isClosed());
}
