/**
 * @file TestModule.cpp
 * @brief Le protocole de vie d'un module, sans une seule dll.
 */

#include <gtest/gtest.h>

#include "DummyModule.hpp"
#include "IModuleManager.hpp"

#include <memory>

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

/* Signale sa propre destruction : c'est ce qui rend la possession
 * observable depuis un test. */
class CountedModule : public IModule {
    public:
        explicit CountedModule(int &destroyed) : _destroyed(destroyed) {}
        ~CountedModule() override { _destroyed++; }

        const char *type() const override { return "graphic2"; }
        const char *name() const override { return "compte"; }

    private:
        int &_destroyed;
};

} // namespace

/* ---- add() : ce que le manager prend, il le rend ------------------ */

TEST(ModuleAdd, RefusesAnEmptyList)
{
    IModuleManager manager;

    EXPECT_FALSE(manager.add("vide", std::vector<std::unique_ptr<IModule>>{}));
}

TEST(ModuleAdd, RefusesAKeyAlreadyTaken)
{
    IModuleManager manager;

    EXPECT_TRUE(manager.add("faux_impl", std::make_unique<DummyModule>("graphic2", "un")));
    EXPECT_FALSE(manager.add("faux_impl", std::make_unique<DummyModule>("audio", "deux")));
}

/** @brief Le manager detruit ce qu'il a pris, quand il meurt. */
TEST(ModuleAdd, DestroysWhatItTookOnDestruction)
{
    int destroyed = 0;

    {
        IModuleManager manager;

        manager.add("compte", std::make_unique<CountedModule>(destroyed));
        EXPECT_EQ(destroyed, 0);
    }
    EXPECT_EQ(destroyed, 1);
}

/** @brief Et quand la colonne est condamnee puis fermee. */
TEST(ModuleAdd, DestroysWhatItTookOnReconcile)
{
    int destroyed = 0;
    IModuleManager manager;

    manager.add("compte", std::make_unique<CountedModule>(destroyed));

    manager.Unload("compte");
    ASSERT_EQ(manager.Reconcile(), 1u);
    EXPECT_EQ(destroyed, 1);
}

/** @brief Tant qu'un detenteur reste, rien n'est detruit. */
TEST(ModuleAdd, KeepsWhatIsStillHeld)
{
    int destroyed = 0;
    IModuleManager manager;

    manager.add("compte", std::make_unique<CountedModule>(destroyed));

    IModule *module = manager.Get("graphic2", "compte");
    ASSERT_TRUE(module->acquire());

    manager.Unload("compte");
    EXPECT_EQ(manager.Reconcile(), 0u);
    EXPECT_EQ(destroyed, 0);

    module->release();
    EXPECT_EQ(manager.Reconcile(), 1u);
    EXPECT_EQ(destroyed, 1);
}

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
    IModuleManager manager;
    auto owned = std::make_unique<DummyModule>("graphic2", "faux");
    DummyModule &module = *owned;
    manager.add("faux_impl", std::move(owned));

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
    IModuleManager manager;
    auto owned = std::make_unique<DummyModule>("graphic2", "faux");
    DummyModule &module = *owned;
    manager.add("faux_impl", std::move(owned));

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
    IModuleManager manager;
    auto owned = std::make_unique<DummyModule>("graphic2", "faux");
    DummyModule &module = *owned;
    manager.add("faux_impl", std::move(owned));

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
    IModuleManager manager;
    auto owned = std::make_unique<DummyModule>("graphic2", "faux");
    DummyModule &module = *owned;
    manager.add("faux_impl", std::move(owned));

    module.acquire();
    module.condemn();
    ASSERT_TRUE(module.mustClose());

    module.reset();

    EXPECT_FALSE(module.mustClose());
    EXPECT_EQ(module.uses(), 0u);
    EXPECT_TRUE(module.isClosed());
}

/* ---- le manager pose par bind() ------------------------------------ */

TEST(ModuleBind, RemembersItsManager)
{
    IModuleManager manager;
    auto owned = std::make_unique<DummyModule>("graphic2", "faux");
    DummyModule &module = *owned;

    EXPECT_EQ(module.seen(), nullptr);
    manager.add("faux_impl", std::move(owned));
    EXPECT_EQ(module.seen(), &manager);
}

/* ---- ce que voit un invite ---------------------------------------- */

class ManagerView : public ::testing::Test {

    protected:
        void SetUp() override
        {
            auto graphicOwned = std::make_unique<DummyModule>("graphic2", "faux");
            auto audioOwned = std::make_unique<DummyModule>("audio", "faux");
            auto otherOwned = std::make_unique<DummyModule>("graphic2", "autre");

            graphic = graphicOwned.get();
            audio = audioOwned.get();
            other = otherOwned.get();

            std::vector<std::unique_ptr<IModule>> fauxImpl;
            fauxImpl.push_back(std::move(graphicOwned));
            fauxImpl.push_back(std::move(audioOwned));
            manager.add("faux_impl", std::move(fauxImpl));

            manager.add("autre_impl", std::move(otherOwned));
        }

        IModuleManager manager;
        DummyModule *graphic;
        DummyModule *audio;
        DummyModule *other;
};

TEST_F(ManagerView, FindsByTypeAndKey)
{
    EXPECT_EQ(manager.Get("graphic2", "faux_impl"), graphic);
    EXPECT_EQ(manager.Get("audio", "faux_impl"), audio);
    EXPECT_EQ(manager.Get("graphic2", "autre_impl"), other);
    EXPECT_EQ(manager.Get("audio", "autre_impl"), nullptr);
}

TEST_F(ManagerView, ListsARow)
{
    /* size() compte les cases de la ligne, trous compris ; filled() ce qui
     * s'y trouve vraiment - autre_impl n'a pas d'audio. */
    EXPECT_EQ(manager.GetAllByType("graphic2").size(), 2u);
    EXPECT_EQ(filled(manager.GetAllByType("graphic2")), 2u);
    EXPECT_EQ(filled(manager.GetAllByType("audio")), 1u);
    EXPECT_TRUE(manager.GetAllByType("game").empty());
}

TEST_F(ManagerView, ListsAColumn)
{
    EXPECT_EQ(filled(manager.GetAllByKey("faux_impl")), 2u);
    EXPECT_EQ(filled(manager.GetAllByKey("autre_impl")), 1u);
    EXPECT_TRUE(manager.GetAllByKey("inconnu").empty());
}

TEST_F(ManagerView, ListsEverything)
{
    EXPECT_EQ(manager.GetAll().size(), 4u);    // 2 lignes x 2 colonnes
    EXPECT_EQ(filled(manager.GetAll()), 3u);   // dont un trou
}

/* ---- Unload/Reconcile, sans dll --------------------------------- */

TEST_F(ManagerView, UnloadThenReconcileClosesAFreeColumn)
{
    manager.Unload("autre_impl");
    EXPECT_EQ(manager.Reconcile(), 1u);

    EXPECT_EQ(manager.Get("graphic2", "autre_impl"), nullptr);
    EXPECT_NE(manager.Get("graphic2", "faux_impl"), nullptr);
}

TEST_F(ManagerView, ReconcileWaitsWhileAModuleIsHeld)
{
    ASSERT_TRUE(other->acquire());

    manager.Unload("autre_impl");
    EXPECT_EQ(manager.Reconcile(), 0u);   // encore tenu

    other->release();
    EXPECT_EQ(manager.Reconcile(), 1u);
}

/* ---- les claims : exclusivite sur une ressource -------------------- */

/**
 * @brief Sans claims(), acquire() ne refuse jamais.
 *
 * C'est le cas par defaut - la plupart des modules ne bloquent rien - et
 * c'est ce qui garantit que ce changement ne casse aucun module existant :
 * personne n'a encore de claims() a declarer.
 */
TEST(ModuleClaims, NoClaimsNeverConflicts)
{
    IModuleManager manager;
    auto aOwned = std::make_unique<DummyModule>("graphic2", "un");
    auto bOwned = std::make_unique<DummyModule>("graphic2", "deux");
    DummyModule &a = *aOwned;
    DummyModule &b = *bOwned;
    manager.add("un", std::move(aOwned));
    manager.add("deux", std::move(bOwned));

    EXPECT_TRUE(a.acquire());
    EXPECT_TRUE(b.acquire());
}

/**
 * @brief Deux modules revendiquant la meme ressource ne peuvent pas
 *        coexister.
 *
 * PREVENTIF : acquire() refuse (false, compteur inchange) plutot que de
 * laisser deux fournisseurs OpenGL tourner en meme temps - la collision ne
 * se rattrape pas apres coup.
 */
TEST(ModuleClaims, ConflictingClaimRefusesAcquire)
{
    IModuleManager manager;
    auto sfmlOwned = std::make_unique<DummyModule>("graphic2", "sfml", std::initializer_list<const char *>{"opengl"});
    auto raylibOwned = std::make_unique<DummyModule>("graphic2", "raylib", std::initializer_list<const char *>{"opengl"});
    DummyModule &sfml = *sfmlOwned;
    DummyModule &raylib = *raylibOwned;
    manager.add("sfml", std::move(sfmlOwned));
    manager.add("raylib", std::move(raylibOwned));

    EXPECT_TRUE(sfml.acquire());
    EXPECT_FALSE(raylib.acquire());
    EXPECT_EQ(raylib.uses(), 0u);
}

/**
 * @brief Relacher la claim la rend disponible au suivant.
 */
TEST(ModuleClaims, ReleaseFreesTheClaim)
{
    IModuleManager manager;
    auto sfmlOwned = std::make_unique<DummyModule>("graphic2", "sfml", std::initializer_list<const char *>{"opengl"});
    auto raylibOwned = std::make_unique<DummyModule>("graphic2", "raylib", std::initializer_list<const char *>{"opengl"});
    DummyModule &sfml = *sfmlOwned;
    DummyModule &raylib = *raylibOwned;
    manager.add("sfml", std::move(sfmlOwned));
    manager.add("raylib", std::move(raylibOwned));

    ASSERT_TRUE(sfml.acquire());
    ASSERT_FALSE(raylib.acquire());

    sfml.release();
    EXPECT_TRUE(raylib.acquire());
}

/**
 * @brief Des claims differentes ne se genent jamais.
 */
TEST(ModuleClaims, DifferentClaimsDoNotConflict)
{
    IModuleManager manager;
    auto graphicOwned = std::make_unique<DummyModule>("graphic2", "sfml", std::initializer_list<const char *>{"opengl"});
    auto audioOwned = std::make_unique<DummyModule>("audio", "openal", std::initializer_list<const char *>{"audio_device"});
    DummyModule &graphic = *graphicOwned;
    DummyModule &audio = *audioOwned;
    manager.add("sfml", std::move(graphicOwned));
    manager.add("openal", std::move(audioOwned));

    EXPECT_TRUE(graphic.acquire());
    EXPECT_TRUE(audio.acquire());
}
