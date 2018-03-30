// Todd Fleming 2018

#include <eosiolib/eosio.hpp>

using namespace eosio;
using namespace std;

static const int size = 32;

// board content is in strings to look reasonable in json
struct board {
    uint64_t game;
    string rows[size];
    uint64_t primary_key() const { return game; }
};

// <random> header is missing. This implements MINSTD.
struct rnd {
    uint32_t seed{1};
    uint32_t operator()() {
        seed = (16807 * seed) % 2147483647u;
        return seed;
    }
};

void randomize(board& b, uint32_t seed) {
    rnd r{seed};
    for (auto& row : b.rows) {
        row.clear();
        for (int i = 0; i < size; ++i)
            row.push_back((r() & 1) ? '*' : ' ');
    }
}

void step(board& b) {
    for (auto& row : b.rows)
        eosio_assert(row.size() == size, "game is corrupt");
    auto old = b;
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            auto alive = [&](int dr, int dc) {
                return old.rows[(r + dr) % size][(c + dc) % size] != ' ';
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

    void create(account_name user, name game, uint32_t seed) {
        require_auth(user);
        remove(user, game);

        multi_index<N(boards), board> boards(_self, user);
        boards.emplace(user, [&](auto& b) {
            b.game = game;
            randomize(b, seed);
        });
    }

    void step(account_name user, name game) {
        require_auth(user);

        multi_index<N(boards), board> boards(_self, user);
        boards.modify(boards.get(game), _self, [&](auto& b) { ::step(b); });
    }
};

EOSIO_ABI(gameoflife, (removeall)(remove)(create)(step))
