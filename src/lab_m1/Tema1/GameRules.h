#pragma once

// The homework rules intentionally differ from the commercial GWENT rules.
// This file has no graphics dependencies, so the resolver can be tested alone.
#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace tema1 {

enum class Type { Damage, Heal, Shield, Spy, Frost, Clear };
enum class Zone { Deck, Hand, Board, Discard };
enum class Result { Playing, PlayerWin, EnemyWin, Draw };
enum class Effect { Spy, Clear, Frost, Shield, Damage, Heal };

inline const char* Name(Type t) {
    static const char* names[] = {"DAMAGE", "HEAL", "SHIELD", "SPY", "FROST", "CLEAR SKY"};
    return names[static_cast<int>(t)];
}
inline bool Temporary(Type t) { return t == Type::Spy || t == Type::Frost || t == Type::Clear; }
inline Type EffectType(Effect e) {
    static const Type types[] = {Type::Spy, Type::Clear, Type::Frost, Type::Shield, Type::Damage, Type::Heal};
    return types[static_cast<int>(e)];
}

struct Card {
    int id, owner, value, printed;
    Type type;
    Zone zone;
    Card(int i, int o, Type t, int v, Zone z)
        : id(i), owner(o), value(v), printed(v), type(t), zone(z) {}
};
struct Player {
    int hp = 30;
    int shield = 0;
    std::vector<int> deck, hand, board, discard;
};
struct Event {
    enum Kind { Transfer, Destroy, Value, Guard, Hit, Restore } kind;
    int owner, card, amount;
    Event(Kind k, int o, int c, int n) : kind(k), owner(o), card(c), amount(n) {}
};

class Game {
 public:
    static const int MaxHP = 30;
    std::array<Player, 2> players;
    std::vector<Card> cards;
    Result result = Result::Playing;
    int turn = 1;
    std::mt19937 random;

    explicit Game(unsigned seed = 1) { Reset(seed); }

    static void Remove(std::vector<int>& list, int id) {
        list.erase(std::remove(list.begin(), list.end(), id), list.end());
    }
    int Add(int owner, Type type, int value, Zone zone) {
        const int id = static_cast<int>(cards.size());
        cards.emplace_back(id, owner, type, value, zone);
        List(owner, zone).push_back(id);
        return id;
    }
    std::vector<int>& List(int owner, Zone zone) {
        Player& p = players[owner];
        if (zone == Zone::Deck) return p.deck;
        if (zone == Zone::Hand) return p.hand;
        if (zone == Zone::Board) return p.board;
        return p.discard;
    }
    void Empty(unsigned seed = 1) {
        cards.clear();
        players = std::array<Player, 2>();
        result = Result::Playing;
        turn = 1;
        random.seed(seed);
    }
    void Reset(unsigned seed, bool deal = true) {
        Empty(seed);
        for (int o = 0; o < 2; ++o) {
            for (int v = 1; v <= 3; ++v)
                for (int n = 0; n < 4; ++n) Add(o, Type::Damage, v, Zone::Deck);
            for (Type t : {Type::Heal, Type::Shield})
                for (int v = 1; v <= 2; ++v)
                    for (int n = 0; n < 3; ++n) Add(o, t, v, Zone::Deck);
            for (Type t : {Type::Spy, Type::Frost, Type::Clear})
                for (int n = 0; n < 2; ++n) Add(o, t, t == Type::Clear ? 1 : 2, Zone::Deck);
            std::shuffle(players[o].deck.begin(), players[o].deck.end(), random);
        }
        if (deal) for (int i = 0; i < 10; ++i) { Draw(0); Draw(1); }
    }
    void Move(int id, int owner, Zone zone) {
        Card& c = cards.at(id);
        Remove(List(c.owner, c.zone), id);
        c.owner = owner;
        c.zone = zone;
        List(owner, zone).push_back(id);
    }
    int Draw(int owner) {
        if (players[owner].deck.empty()) return -1;
        const int id = players[owner].deck.back();
        Move(id, owner, Zone::Hand);
        return id;
    }
    bool CanPlay(int owner, int id) const {
        return result == Result::Playing && id >= 0 && id < static_cast<int>(cards.size())
            && cards[id].owner == owner && cards[id].zone == Zone::Hand;
    }
    bool Play(int owner, int id) {
        if (!CanPlay(owner, id)) return false;
        Move(id, owner, Zone::Board);
        return true;
    }
    int Choose(int owner) {
        const auto& hand = players[owner].hand;
        if (hand.empty()) return -1;
        std::uniform_int_distribution<int> choose(0, static_cast<int>(hand.size()) - 1);
        return hand[choose(random)];
    }
    std::vector<int> Sources(Type type) const {
        std::vector<int> ids;
        for (const Player& p : players)
            for (int id : p.board)
                if (cards[id].type == type && cards[id].value > 0) ids.push_back(id);
        return ids;
    }
    void Destroy(int id, std::vector<Event>& events) {
        if (cards[id].zone != Zone::Board) return;
        const int owner = cards[id].owner;
        Move(id, owner, Zone::Discard);
        events.emplace_back(Event::Destroy, owner, id, 0);
    }
    void Decay(const std::vector<int>& sources, std::vector<Event>& events) {
        for (int id : sources) if (cards[id].zone == Zone::Board) {
            --cards[id].value;
            events.emplace_back(Event::Value, cards[id].owner, id, -1);
            if (cards[id].value <= 0) Destroy(id, events);
        }
    }
    // Same-type sources use a phase-start snapshot; no vector is mutated while iterated.
    std::vector<Event> Resolve(Effect phase) {
        std::vector<Event> events;
        if (result != Result::Playing) return events;
        const std::vector<int> sources = Sources(EffectType(phase));
        if (phase == Effect::Spy) {
            std::array<std::vector<int>, 2> available = {{players[0].hand, players[1].hand}};
            std::vector<std::pair<int, int>> transfers;
            for (int id : sources) {
                const int owner = cards[id].owner;
                auto& targets = available[1 - owner];
                if (targets.empty()) continue;
                std::uniform_int_distribution<int> pick(0, static_cast<int>(targets.size()) - 1);
                const int index = pick(random), target = targets[index];
                targets.erase(targets.begin() + index);
                transfers.emplace_back(target, owner);
            }
            for (const auto& transfer : transfers) {
                Move(transfer.first, transfer.second, Zone::Hand);
                events.emplace_back(Event::Transfer, transfer.second, transfer.first, 1);
            }
            Decay(sources, events);
        } else if (phase == Effect::Clear) {
            if (!sources.empty()) {
                const auto weather = Sources(Type::Frost);
                for (int id : weather) Destroy(id, events);
            }
            Decay(sources, events);
        } else if (phase == Effect::Frost) {
            std::array<int, 2> reductions = {{0, 0}};
            for (int id : sources) ++reductions[1 - cards[id].owner];
            std::vector<int> board = players[0].board;
            board.insert(board.end(), players[1].board.begin(), players[1].board.end());
            for (int id : board) {
                const int loss = reductions[cards[id].owner]
                    + (cards[id].type == Type::Frost ? 1 : 0);
                cards[id].value -= loss;
                if (loss) events.emplace_back(Event::Value, cards[id].owner, id, -loss);
                if (cards[id].value <= 0) Destroy(id, events);
            }
        } else {
            std::array<int, 2> totals = {{0, 0}};
            for (int id : sources) totals[cards[id].owner] += cards[id].value;
            for (int owner = 0; owner < 2; ++owner) {
                Player& p = players[owner];
                if (phase == Effect::Shield) {
                    p.shield += totals[owner];
                    if (totals[owner]) events.emplace_back(Event::Guard, owner, -1, totals[owner]);
                } else if (phase == Effect::Damage) {
                    const int absorbed = std::min(p.shield, totals[1 - owner]);
                    p.shield -= absorbed;
                    const int damage = totals[1 - owner] - absorbed;
                    p.hp = std::max(0, p.hp - damage);
                    if (totals[1 - owner]) events.emplace_back(Event::Hit, owner, -1, damage);
                } else {
                    const int old = p.hp;
                    p.hp = std::min(static_cast<int>(MaxHP), p.hp + totals[owner]);
                    if (p.hp != old) events.emplace_back(Event::Restore, owner, -1, p.hp - old);
                }
            }
            if (phase == Effect::Damage) CheckLethal();
        }
        return events;
    }
    void CheckLethal() {
        if (players[0].hp <= 0 && players[1].hp <= 0) result = Result::Draw;
        else if (players[0].hp <= 0) result = Result::EnemyWin;
        else if (players[1].hp <= 0) result = Result::PlayerWin;
    }
    void FinishTurn() {
        CheckLethal();
        if (result == Result::Playing && players[0].hand.empty() && players[1].hand.empty()) {
            result = players[0].hp == players[1].hp ? Result::Draw
                : players[0].hp > players[1].hp ? Result::PlayerWin : Result::EnemyWin;
        }
        if (result == Result::Playing) ++turn;
    }
    bool Invariant() const {
        std::vector<int> seen(cards.size(), 0);
        for (int owner = 0; owner < 2; ++owner) {
            const Player& p = players[owner];
            const std::vector<int>* lists[] = {&p.deck, &p.hand, &p.board, &p.discard};
            for (int z = 0; z < 4; ++z) for (int id : *lists[z]) {
                if (id < 0 || id >= static_cast<int>(cards.size())) return false;
                if (++seen[id] != 1 || cards[id].owner != owner
                    || static_cast<int>(cards[id].zone) != z) return false;
                if (z == 2 && cards[id].value <= 0) return false;
            }
        }
        return std::all_of(seen.begin(), seen.end(), [](int n) { return n == 1; });
    }
};
}  // namespace tema1
