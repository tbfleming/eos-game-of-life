// Todd Fleming 2018

#include <eosiolib/eosio.hpp>

using namespace eosio;
using namespace std;

// board content is in strings to look reasonable in json
struct board {
    uint64_t game;
    vector<string> rows;
    uint64_t primary_key() const { return game; }
};

// <random> header is missing. This implements MINSTD.
struct rnd {
    uint32_t seed{1};
    uint32_t operator()() {
        seed = (16807ull * seed) % 2147483647u;
        return seed;
    }
};

void randomize(board& b, uint32_t num_rows, uint32_t num_cols, uint32_t seed) {
    b.rows.resize(num_rows);
    rnd r{seed};
    for (auto& row : b.rows) {
        row.clear();
        for (int i = 0; i < num_cols; ++i)
            row.push_back((r() & 1) ? '*' : ' ');
    }
}

void step(board& b) {
    int num_rows = b.rows.size();
    eosio_assert(num_rows >= 3, "game is corrupt");
    int num_cols = b.rows[0].size();
    eosio_assert(num_cols >= 3, "game is corrupt");
    for (auto& row : b.rows)
        eosio_assert(row.size() == num_cols, "game is corrupt");
    auto old = b;
    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            auto alive = [&](int dr, int dc) {
                return old.rows[(r + dr + num_rows) % num_rows]
                               [(c + dc + num_cols) % num_cols] != ' ';
            };
            auto neighbors = alive(-1, -1) + alive(-1, 0) + alive(-1, 1) +
                             alive(0, -1) + alive(0, 1) + alive(1, -1) +
                             alive(1, 0) + alive(1, 1);
            if (neighbors == 3 || (alive(0, 0) && neighbors == 2))
                b.rows[r][c] = '*';
            else
                b.rows[r][c] = ' ';
        }
    }
}

struct gameoflife : contract {
    gameoflife(account_name self) : contract{self} {}

    void removeall(account_name user) {
        require_auth(user);

        // multi_index can't erase when the format changed
        auto it = db_lowerbound_i64(_self, user, N(boards), 0);
        while (it >= 0) {
            auto del = it;
            uint64_t dummy;
            it = db_next_i64(it, &dummy);
            db_remove_i64(del);
        }
    }

    void remove(account_name user, name game) {
        require_auth(user);

        auto it = db_find_i64(_self, user, N(boards), game);
        if (it >= 0)
            db_remove_i64(it);
    }

    void create(account_name user, name game, uint32_t num_rows,
                uint32_t num_cols, uint32_t seed) {
        eosio_assert(num_rows >= 3 && num_rows <= 100,
                     "num_rows out of range [3, 100]");
        eosio_assert(num_cols >= 3 && num_cols <= 100,
                     "num_cols out of range [3, 100]");
        require_auth(user);
        remove(user, game);

        multi_index<N(boards), board> boards(_self, user);
        boards.emplace(user, [&](auto& b) {
            b.game = game;
            randomize(b, num_rows, num_cols, seed);
        });
    }

    void step(account_name user, name game) {
        require_auth(user);

        multi_index<N(boards), board> boards(_self, user);
        boards.modify(boards.get(game), _self, [&](auto& b) { ::step(b); });
    }
};

EOSIO_ABI(gameoflife, (removeall)(remove)(create)(step))
