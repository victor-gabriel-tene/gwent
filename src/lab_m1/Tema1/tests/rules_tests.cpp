// The application source glob may compile this file; only the standalone test
// build defines TEMA1_RULE_TESTS, so it never introduces a second application main.
#ifdef TEMA1_RULE_TESTS
#include "../GameRules.h"
#include <iostream>
#include <stdexcept>

using namespace tema1;
static int checks = 0;
static void Expect(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
int main() {
    try {
        Game g(42);
        Expect(g.players[0].hand.size() == 10 && g.players[1].deck.size() == 20, "opening deal");
        Expect(g.cards.size() == 60 && g.Invariant(), "deck composition and ownership");
        Game again(42);
        Expect(g.players[0].hand == again.players[0].hand, "repeatable seeded shuffle");
        g.Empty();
        int damage = g.Add(0, Type::Damage, 3, Zone::Hand);
        Expect(!g.Play(1,damage) && g.Play(0,damage) && !g.Play(0,damage), "validate owner and zone");
        g.Resolve(Effect::Damage); g.Resolve(Effect::Damage);
        Expect(g.players[1].hp == 24 && g.cards[damage].value == 3, "permanent recurrence");

        g.Empty();
        g.players[0].hp = 10;
        int shield = g.Add(0,Type::Shield,3,Zone::Board);
        g.Add(1,Type::Damage,5,Zone::Board);
        g.Add(0,Type::Heal,2,Zone::Board);
        g.Resolve(Effect::Shield); g.Resolve(Effect::Damage); g.Resolve(Effect::Heal);
        Expect(g.players[0].hp == 10 && g.players[0].shield == 0, "shield absorbs before health and heal");
        g.Resolve(Effect::Shield); g.Resolve(Effect::Shield);
        std::vector<Event> events; g.Destroy(shield,events);
        Expect(g.players[0].shield == 6, "shield carries and survives source destruction");
        g.players[0].hp = 29; g.Resolve(Effect::Heal);
        Expect(g.players[0].hp == 30, "healing cap");

        g.Empty();
        int spy0 = g.Add(0,Type::Spy,2,Zone::Board);
        int spy1 = g.Add(1,Type::Spy,2,Zone::Board);
        int hand0 = g.Add(0,Type::Damage,1,Zone::Hand);
        int hand1 = g.Add(1,Type::Shield,1,Zone::Hand);
        g.Resolve(Effect::Spy);
        Expect(g.players[0].hand == std::vector<int>(1,hand1)
            && g.players[1].hand == std::vector<int>(1,hand0), "simultaneous spy swap");
        Expect(g.cards[spy0].value == 1 && g.cards[spy1].value == 1 && g.Invariant(), "spy decay and ownership");
        g.Resolve(Effect::Spy);
        Expect(g.cards[spy0].zone == Zone::Discard && g.cards[spy1].zone == Zone::Discard, "spy expiration");
        g.Empty();
        int noTarget = g.Add(0,Type::Spy,1,Zone::Board);
        g.Resolve(Effect::Spy);
        Expect(g.cards[noTarget].zone == Zone::Discard, "no-target temporary still decays");
        g.Empty();
        g.Add(0,Type::Spy,2,Zone::Board); g.Add(0,Type::Spy,2,Zone::Board);
        g.Add(1,Type::Heal,1,Zone::Hand);
        g.Resolve(Effect::Spy);
        Expect(g.players[0].hand.size() == 1 && g.players[1].hand.empty() && g.Invariant(), "spies choose distinct available targets");

        g.Empty();
        int frost0 = g.Add(0,Type::Frost,2,Zone::Board);
        int frost1 = g.Add(1,Type::Frost,2,Zone::Board);
        int enemyDamage = g.Add(1,Type::Damage,1,Zone::Board);
        int ownDamage = g.Add(0,Type::Damage,2,Zone::Board);
        g.Resolve(Effect::Frost);
        Expect(g.cards[frost0].zone == Zone::Discard && g.cards[frost1].zone == Zone::Discard,
               "opposing weather fires then decays simultaneously");
        Expect(g.cards[enemyDamage].zone == Zone::Discard && g.cards[ownDamage].value == 1, "weather reduces and destroys");
        g.Resolve(Effect::Damage);
        Expect(g.players[0].hp == 30 && g.players[1].hp == 29, "destroyed sources cannot act later");

        g.Empty();
        int clear = g.Add(0,Type::Clear,1,Zone::Board);
        frost0 = g.Add(0,Type::Frost,2,Zone::Board);
        frost1 = g.Add(1,Type::Frost,2,Zone::Board);
        int unchanged = g.Add(1,Type::Heal,2,Zone::Board);
        g.Resolve(Effect::Clear); g.Resolve(Effect::Frost);
        Expect(g.cards[clear].zone == Zone::Discard && g.cards[frost0].zone == Zone::Discard
            && g.cards[frost1].zone == Zone::Discard && g.cards[unchanged].value == 2, "clear removes both sides before frost");

        g.Empty();
        g.Add(0,Type::Frost,2,Zone::Board);
        int reduced = g.Add(1,Type::Damage,3,Zone::Board);
        g.Resolve(Effect::Frost);
        g.Add(1,Type::Clear,1,Zone::Board);
        g.Resolve(Effect::Clear);
        Expect(g.cards[reduced].value == 2, "clear does not restore lost values");

        g.Empty();
        g.players[0].hp = g.players[1].hp = 1;
        g.Add(0,Type::Damage,1,Zone::Board); g.Add(1,Type::Damage,1,Zone::Board);
        g.Add(0,Type::Heal,3,Zone::Board);
        g.Resolve(Effect::Damage); g.Resolve(Effect::Heal);
        Expect(g.result == Result::Draw && g.players[0].hp == 0, "simultaneous lethal before healing");
        g.Empty(); g.players[1].hp = 29; g.players[1].shield = 100;
        g.FinishTurn(); Expect(g.result == Result::PlayerWin, "exhaustion compares only HP");
        g.Empty(); g.FinishTurn(); Expect(g.result == Result::Draw, "equal exhaustion draw");
        g.Empty(); g.players[0].hp = 0; g.CheckLethal();
        Expect(g.result == Result::EnemyWin, "defeat");
        g.Empty(); g.Add(1,Type::Damage,1,Zone::Hand); g.FinishTurn();
        Expect(g.result == Result::Playing && g.Choose(0) == -1, "single empty hand does not end game");
        g.Add(0,Type::Spy,2,Zone::Board); g.Resolve(Effect::Spy);
        Expect(!g.players[0].hand.empty(), "spy refills empty hand");
        g.Reset(7);
        Expect(g.turn == 1 && g.players[0].hp == 30 && g.players[0].shield == 0
            && g.players[0].discard.empty() && g.Invariant(), "restart clears prior state");

        // Property-style full matches: every legal play consumes one of the initial
        // twenty hand cards. Spies transfer, never duplicate or draw more cards.
        for (unsigned seed = 0; seed < 1000; ++seed) {
            Game match(seed);
            int turns = 0;
            while (match.result == Result::Playing) {
                for (int o = 0; o < 2; ++o) {
                    const int id = match.Choose(o);
                    if (id >= 0) Expect(match.Play(o,id), "random valid placement");
                }
                for (int phase = 0; phase < 6; ++phase) {
                    match.Resolve(static_cast<Effect>(phase));
                    Expect(match.Invariant(), "ownership/value invariant after every phase");
                }
                match.FinishTurn();
                Expect(++turns <= 20, "match terminates within twenty plays");
                for (const Player& p : match.players)
                    Expect(p.hp >= 0 && p.hp <= 30 && p.shield >= 0, "bounded HP and nonnegative shield");
            }
        }
        std::cout << "PASS: " << checks << " checks, including 1000 complete seeded matches\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n'; return 1;
    }
}
#endif
