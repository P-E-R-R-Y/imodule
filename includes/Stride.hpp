/**
 * @file Stride.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief Une table a deux axes nommes, lue par pas.
 *
 * @addtogroup imodule
 * @{
 */

#ifndef STRIDE_HPP
#define STRIDE_HPP

#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class Stride
 * @brief Une table `ligne x colonne`, nommee des deux cotes, rangee a plat.
 *
 * Une ligne ou une colonne se lit en place : un depart, un pas, une
 * longueur. Rien n'est recopie dans un sens ni dans l'autre.
 *
 * Les deux axes sont des chaines. Les numeros de ligne et de colonne
 * restent internes.
 *
 * Les cases vides valent T{} et sont rendues telles quelles - Stride ne
 * filtre rien et ne sait pas ce que T{} signifie.
 */
template <typename T>
class Stride {

    public:
        /**
         * @class Span
         * @brief Une ligne ou une colonne : un depart, un pas, une longueur.
         *
         * Lue directement dans la table, sans allocation. Contient les
         * cases vides.
         */
        class Span {
            public:
                class Iterator {
                    public:
                        using iterator_category = std::forward_iterator_tag;
                        using difference_type = std::ptrdiff_t;
                        using value_type = T;
                        using pointer = const T *;
                        using reference = const T &;

                        Iterator(const T *at, size_t step) : _at(at), _step(step) {}

                        const T &operator*() const { return *_at; }
                        Iterator &operator++() { _at += _step; return *this; }
                        bool operator==(const Iterator &o) const { return _at == o._at; }
                        bool operator!=(const Iterator &o) const { return _at != o._at; }

                    private:
                        const T *_at;
                        size_t _step;
                };

                Span() = default;
                Span(const T *base, size_t step, size_t count)
                    : _base(base), _step(step), _count(count) {}

                const T &operator[](size_t at) const { return _base[at * _step]; }

                /** @brief Le nombre de cases, vides comprises. */
                size_t size() const { return _count; }
                bool empty() const { return _count == 0; }

                Iterator begin() const { return Iterator(_base, _step); }
                Iterator end() const { return Iterator(_base + _count * _step, _step); }

            private:
                const T *_base = nullptr;
                size_t _step = 1;
                size_t _count = 0;
        };

        /** @brief La case, T{} si l'une des deux chaines est inconnue. */
        T at(const std::string &row, const std::string &column) const {
            const auto r = _rowOf.find(row);
            const auto c = _columnOf.find(column);

            if (r == _rowOf.end() || c == _columnOf.end())
                return T{};
            return _cells[c->second * _rows.size() + r->second];
        }

        /** @brief La ligne : une case par colonne. Vide si la ligne est inconnue. */
        Span row(const std::string &row) const {
            const auto r = _rowOf.find(row);

            if (r == _rowOf.end())
                return Span();
            return Span(_cells.data() + r->second, _rows.size(), _columnNames.size());
        }

        /** @brief La colonne : une case par ligne. Vide si la colonne est inconnue. */
        Span column(const std::string &column) const {
            const auto c = _columnOf.find(column);

            if (c == _columnOf.end())
                return Span();
            return columnAt(c->second);
        }

        /** @brief Toute la table, colonne par colonne. */
        Span all() const {
            return Span(_cells.data(), 1, _cells.size());
        }

        /** @brief Cette colonne existe-t-elle deja ? */
        bool hasColumn(const std::string &column) const {
            return _columnOf.count(column) != 0;
        }

        /**
         * @brief Ajoute une colonne vide.
         *
         * @param column
         * @return false si le nom est deja pris
         */
        bool addColumn(const std::string &column) {
            if (_columnOf.count(column))
                return false;

            _columnOf.emplace(column, _columnNames.size());
            _columnNames.push_back(column);
            _cells.resize(_cells.size() + _rows.size(), T{});
            return true;
        }

        /**
         * @brief Ecrit une case. La ligne est creee si elle est nouvelle.
         *
         * @param row
         * @param column doit exister (addColumn())
         * @param value
         * @return false si la colonne est inconnue
         */
        bool set(const std::string &row, const std::string &column, T value) {
            const auto c = _columnOf.find(column);

            if (c == _columnOf.end())
                return false;

            /* La ligne avant l'indice de case : la creer decale la table. */
            const size_t at = rowOf(row);

            _cells[c->second * _rows.size() + at] = value;
            return true;
        }

        /**
         * @brief Retire les colonnes que @p should accepte.
         *
         * @param should recoit (nom, Span de la colonne)
         * @return combien ont ete retirees
         */
        template <typename Predicate>
        size_t eraseColumnsIf(Predicate should) {
            size_t erased = 0;

            for (size_t column = 0; column < _columnNames.size(); ) {
                if (!should(_columnNames[column], columnAt(column))) {
                    column++;
                    continue;
                }

                const auto begin = _cells.begin() + column * _rows.size();

                _cells.erase(begin, begin + _rows.size());
                _columnNames.erase(_columnNames.begin() + column);
                reindex();
                erased++;
            }
            return erased;
        }

        /** @brief Les lignes, dans l'ordre de decouverte. */
        const std::vector<std::string> &rows() const { return _rows; }

        /** @brief Les colonnes, dans l'ordre d'ajout. */
        const std::vector<std::string> &columnNames() const { return _columnNames; }

    private:
        /** @brief La colonne par indice. */
        Span columnAt(size_t column) const {
            return Span(_cells.data() + column * _rows.size(), 1, _rows.size());
        }

        /**
         * @brief L'indice de cette ligne, creee si elle est nouvelle.
         *
         * Une ligne de plus veut dire une case de plus dans chaque colonne :
         * toute la table est reflowee, en une fois.
         */
        size_t rowOf(const std::string &row) {
            const auto found = _rowOf.find(row);

            if (found != _rowOf.end())
                return found->second;

            const size_t oldRows = _rows.size();
            const size_t newRows = oldRows + 1;
            const size_t columns = _columnNames.size();

            std::vector<T> grown(columns * newRows, T{});
            for (size_t column = 0; column < columns; column++)
                for (size_t r = 0; r < oldRows; r++)
                    grown[column * newRows + r] = _cells[column * oldRows + r];
            _cells = std::move(grown);

            _rowOf.emplace(row, oldRows);
            _rows.push_back(row);
            return oldRows;
        }

        /** @brief Les colonnes ont bouge : on refait la correspondance. */
        void reindex() {
            _columnOf.clear();
            for (size_t at = 0; at < _columnNames.size(); at++)
                _columnOf.emplace(_columnNames[at], at);
        }

        /* La table, colonne-majeure : _cells[colonne * _rows.size() + ligne]. */
        std::vector<T> _cells;

        std::vector<std::string> _rows;         ///< ligne   -> nom
        std::vector<std::string> _columnNames;  ///< colonne -> nom
        std::unordered_map<std::string, size_t> _rowOf;
        std::unordered_map<std::string, size_t> _columnOf;
};

/** @} */

#endif /* !STRIDE_HPP */
