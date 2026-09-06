/**
 * @file TestStride.cpp
 * @brief Le rangement, isole d'IModule et de tout ce qui vient avec.
 */

#include <gtest/gtest.h>

#include "Stride.hpp"

namespace {
/* Un Span rend les trous a T{} : size() compte les cases, filled() compte
 * ce qui s'y trouve vraiment. */
size_t filled(Stride<int *>::Span span) {
    size_t count = 0;

    for (int *cell : span)
        if (cell)
            count++;
    return count;
}
} // namespace

TEST(Stride, EmptyCellIsDefaultValue)
{
    Stride<int *> table;

    EXPECT_EQ(table.at("graphic2", "sfml"), nullptr);
}

TEST(Stride, SetThenAtRoundTrips)
{
    Stride<int *> table;
    int value = 42;

    ASSERT_TRUE(table.addColumn("sfml"));
    ASSERT_TRUE(table.set("graphic2", "sfml", &value));

    EXPECT_EQ(table.at("graphic2", "sfml"), &value);
}

TEST(Stride, SetIsRefusedOnAnUnknownColumn)
{
    Stride<int *> table;
    int value = 42;

    EXPECT_FALSE(table.set("graphic2", "jamais_chargee", &value));
}

TEST(Stride, AddingTheSameColumnTwiceIsRefused)
{
    Stride<int *> table;

    EXPECT_TRUE(table.addColumn("sfml"));
    EXPECT_FALSE(table.addColumn("sfml"));
}

TEST(Stride, RowGrowsEveryExistingColumn)
{
    Stride<int *> table;
    int a = 1, b = 2;

    table.addColumn("sfml");
    table.addColumn("raylib");

    table.set("graphic2", "sfml", &a);

    /* Une ligne creee APRES coup fait quand meme grandir "sfml" : la case
     * existe et vaut T{} avant d'etre ecrite. */
    table.set("audio", "raylib", &b);

    EXPECT_EQ(table.at("audio", "sfml"), nullptr);
    EXPECT_EQ(table.at("graphic2", "raylib"), nullptr);
    EXPECT_EQ(table.at("graphic2", "sfml"), &a);
    EXPECT_EQ(table.at("audio", "raylib"), &b);
}

class StrideView : public ::testing::Test {

    protected:
        void SetUp() override
        {
            table.addColumn("sfml");
            table.addColumn("raylib");

            table.set("graphic2", "sfml", &sfmlGraphic);
            table.set("audio", "sfml", &sfmlAudio);
            table.set("graphic2", "raylib", &raylibGraphic);
            // raylib n'a pas d'audio : cette case reste T{}
        }

        Stride<int *> table;
        int sfmlGraphic = 1, sfmlAudio = 2, raylibGraphic = 3;
};

TEST_F(StrideView, RowSpansEveryColumn)
{
    /* Une ligne fait la longueur du nombre de colonnes, trous compris :
     * raylib n'a pas d'audio, sa case est rendue a nullptr. */
    EXPECT_EQ(table.row("graphic2").size(), 2u);
    EXPECT_EQ(table.row("audio").size(), 2u);
    EXPECT_EQ(filled(table.row("audio")), 1u);
    EXPECT_TRUE(table.row("inconnu").empty());
}

TEST_F(StrideView, ColumnSpansEveryRow)
{
    EXPECT_EQ(table.column("sfml").size(), 2u);
    EXPECT_EQ(filled(table.column("sfml")), 2u);
    EXPECT_EQ(filled(table.column("raylib")), 1u);
    EXPECT_TRUE(table.column("inconnu").empty());
}

TEST_F(StrideView, AllSpansTheWholeBuffer)
{
    EXPECT_EQ(table.all().size(), 4u);    // 2 lignes x 2 colonnes
    EXPECT_EQ(filled(table.all()), 3u);
}

TEST_F(StrideView, RowReadsTheRightCellsInOrder)
{
    Stride<int *>::Span row = table.row("graphic2");

    EXPECT_EQ(row[0], &sfmlGraphic);    // colonne sfml
    EXPECT_EQ(row[1], &raylibGraphic);  // colonne raylib, un pas plus loin
}

TEST_F(StrideView, EraseColumnsIfRemovesWhatItAccepts)
{
    const size_t erased = table.eraseColumnsIf(
        [](const std::string &name, Stride<int *>::Span) { return name == "sfml"; });

    EXPECT_EQ(erased, 1u);
    EXPECT_FALSE(table.hasColumn("sfml"));
    EXPECT_EQ(table.at("graphic2", "sfml"), nullptr);

    /* raylib reste lisible par son nom, la renumerotation est interne. */
    EXPECT_EQ(table.at("graphic2", "raylib"), &raylibGraphic);
    EXPECT_EQ(table.columnNames().size(), 1u);
}

TEST_F(StrideView, RowsAndColumnNamesKeepDiscoveryOrder)
{
    EXPECT_EQ(table.rows().size(), 2u);
    EXPECT_EQ(table.rows()[0], "graphic2");
    EXPECT_EQ(table.rows()[1], "audio");

    EXPECT_EQ(table.columnNames()[0], "sfml");
    EXPECT_EQ(table.columnNames()[1], "raylib");
}
