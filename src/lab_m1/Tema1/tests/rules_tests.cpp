// The application build also discovers this file; only the standalone test
// command defines TEMA1_RULE_TESTS and introduces this main function.
#ifdef TEMA1_RULE_TESTS
#include "../GameRules.h"
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace tema1;
namespace {
int checks = 0;
const char* section = "initialization";
void Expect(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(std::string(section) + ": " + message);
}
void ResolveTurn(Game& game) {
    for (Effect phase : Phases()) {
        game.Resolve(phase);
        Expect(game.Invariant(), "invariant after resolution phase");
    }
    game.FinishTurn();
    Expect(game.Invariant(), "invariant after completed turn");
}
void KeepPlaying(Game& game) {
    game.Add(0, Type::Heal, 1, Zone::Hand, Lane::Ranged);
    game.Add(1, Type::Heal, 1, Zone::Hand, Lane::Ranged);
}
void EnableLeader(Game& game, int owner, Leader leader) {
    game.players[owner].leader = leader;
    game.players[owner].leaderUsed = false;
}
void UseLeader(Game& game, int owner, Leader leader) {
    EnableLeader(game, owner, leader);
    Expect(game.Commit(owner, Action(Action::UseLeader)), "leader action commits");
}
void OpeningAndLegality() {
    section = "opening and legal actions";
    Game game(42), again(42);
    Expect(game.cards.size() == 60 && game.Invariant(), "sixty unique cards");
    Expect(game.players[0].hand.size() == 10 && game.players[1].deck.size() == 20, "opening ten-card deal");
    Expect(game.players[0].hand == again.players[0].hand, "repeatable seeded deal");
    for (int owner = 0; owner < 2; ++owner) {
        int count[12] = {}, damageValues[4] = {}, damageLanes[3] = {};
        for (const Card& card : game.cards) if (card.owner == owner) {
            ++count[static_cast<int>(card.type)];
            if (card.type == Type::Damage) {
                ++damageValues[card.printed];
                ++damageLanes[static_cast<int>(card.printedLane)];
            }
            if (Unit(card.type)) Expect(card.printedLane == Lane::Melee || card.printedLane == Lane::Ranged
                || card.printedLane == Lane::Siege, "units have a printed lane");
            if (card.type == Type::Medic) Expect(card.printedLane == Lane::Ranged, "medics are ranged");
        }
        Expect(count[static_cast<int>(Type::Damage)] == 12 && count[static_cast<int>(Type::Heal)] == 4
            && count[static_cast<int>(Type::Shield)] == 4 && count[static_cast<int>(Type::Medic)] == 2,
               "permanent deck composition");
        for (Type type : {Type::Spy, Type::Frost, Type::Clear, Type::Fog, Type::Rain,
                          Type::Horn, Type::Scorch, Type::Decoy})
            Expect(count[static_cast<int>(type)] == 1, "one of every special card");
        Expect(damageValues[1] == 4 && damageValues[2] == 4 && damageValues[3] == 4, "damage value distribution");
        Expect(damageLanes[0] == 4 && damageLanes[1] == 4 && damageLanes[2] == 4, "damage lanes round-robin");
        Expect(game.players[owner].crystals == 2 && !game.players[owner].leaderUsed, "fresh crystals and leaders");
    }
    const Effect expected[] = {Effect::Leaders, Effect::Scorch, Effect::Utility, Effect::Spy,
        Effect::Clear, Effect::Weather, Effect::Horn, Effect::Shield, Effect::Damage, Effect::Heal};
    const auto& phases = Phases();
    Expect(phases.size() == sizeof(expected) / sizeof(expected[0]), "one complete phase list");
    for (size_t i = 0; i < phases.size(); ++i)
        Expect(phases[i] == expected[i] && PhaseName(phases[i])[0], "prescribed phase ordering");
    game.Empty();
    const int unit = game.Add(0, Type::Damage, 3, Zone::Hand, Lane::Siege);
    Expect(!game.Commit(0, Action(Action::Skip)), "cannot skip a playable card");
    Expect(!game.CanPlay(1, unit, Lane::Siege) && !game.CanPlay(0, -1, Lane::Siege), "owner and ID validation");
    Expect(!game.CanPlay(0, unit, Lane::Melee) && !game.CanPlay(0, unit, Lane::Any)
        && !game.CanAct(0, Action(Action::PlayCard, unit, Lane::None)), "wrong and non-board lanes rejected");
    Expect(game.Play(0, unit, Lane::Siege), "correct lane commits");
    const int extra = game.Add(0, Type::Shield, 1, Zone::Hand, Lane::Siege);
    Expect(!game.Play(0, extra, Lane::Siege) && !game.Commit(0, Action(Action::Skip)), "one primary action per turn");
    Expect(game.cards[unit].lane == Lane::Siege && game.AnyCommitted(), "lane and turn context");
    game.Empty();
    for (const auto& entry : {std::make_pair(Type::Frost, Lane::Melee), std::make_pair(Type::Fog, Lane::Ranged),
                             std::make_pair(Type::Rain, Lane::Siege)}) {
        const int id = game.Add(0, entry.first, 2, Zone::Hand);
        for (Lane lane : {Lane::Melee, Lane::Ranged, Lane::Siege})
            Expect(game.CanPlay(0, id, lane) == (lane == entry.second), "weather lane restriction");
    }
    const int spy = game.Add(0, Type::Spy, 2, Zone::Hand);
    for (Lane lane : {Lane::Melee, Lane::Ranged, Lane::Siege})
        Expect(game.CanPlay(0, spy, lane), "non-weather temporary accepts all friendly lanes");
}
void CoreRecurrence() {
    section = "permanent effects and spies";
    Game game;
    game.Empty();
    const int damage = game.Add(0, Type::Damage, 3, Zone::Board, Lane::Melee);
    game.Resolve(Effect::Damage); game.Resolve(Effect::Damage);
    Expect(game.players[1].hp == 24 && game.cards[damage].value == 3, "permanent damage recurs");
    game.Empty(); game.players[0].hp = 10;
    const int shield = game.Add(0, Type::Shield, 3, Zone::Board, Lane::Siege);
    game.Add(1, Type::Damage, 5, Zone::Board, Lane::Melee);
    game.Add(0, Type::Heal, 2, Zone::Board, Lane::Ranged);
    game.Resolve(Effect::Shield); game.Resolve(Effect::Damage); game.Resolve(Effect::Heal);
    Expect(game.players[0].hp == 10 && game.players[0].shield == 0, "shield absorbs before health and heal");
    game.Resolve(Effect::Shield); game.Resolve(Effect::Shield);
    std::vector<Event> events; game.Destroy(shield, events);
    Expect(game.players[0].shield == 6, "shield survives source destruction");
    game.players[0].hp = 29; game.Resolve(Effect::Heal);
    Expect(game.players[0].hp == 30, "healing cap");
    game.Empty();
    const int spy0 = game.Add(0, Type::Spy, 2, Zone::Board, Lane::Melee);
    const int spy1 = game.Add(1, Type::Spy, 2, Zone::Board, Lane::Melee);
    const int hand0 = game.Add(0, Type::Damage, 1, Zone::Hand, Lane::Melee);
    const int hand1 = game.Add(1, Type::Shield, 1, Zone::Hand, Lane::Siege);
    game.cards[hand1].known = true; game.Resolve(Effect::Spy);
    Expect(game.players[0].hand == std::vector<int>(1, hand1) && game.players[1].hand == std::vector<int>(1, hand0),
           "simultaneous spy swap");
    Expect(!game.cards[hand0].known && game.cards[spy0].value == 1 && game.cards[spy1].value == 1 && game.Invariant(),
           "spy ownership, hidden information and decay");
    game.Resolve(Effect::Spy);
    Expect(game.cards[spy0].zone == Zone::Discard && game.cards[spy1].zone == Zone::Discard, "spy expiration");
    game.Empty();
    const int noTarget = game.Add(0, Type::Spy, 1, Zone::Board, Lane::Ranged);
    game.Resolve(Effect::Spy);
    Expect(game.cards[noTarget].zone == Zone::Discard, "no-target temporary decays");
    game.Empty();
    game.Add(0, Type::Spy, 2, Zone::Board, Lane::Melee); game.Add(0, Type::Spy, 2, Zone::Board, Lane::Siege);
    game.Add(1, Type::Heal, 1, Zone::Hand, Lane::Ranged); game.Resolve(Effect::Spy);
    Expect(game.players[0].hand.size() == 1 && game.players[1].hand.empty() && game.Invariant(),
           "multiple spies choose distinct phase-start targets");
}
void WeatherAndClear() {
    section = "lane weather";
    Game game; game.Empty();
    const int frost = game.Add(0, Type::Frost, 2, Zone::Board, Lane::Melee);
    const int fog = game.Add(0, Type::Fog, 2, Zone::Board, Lane::Ranged);
    const int rain = game.Add(1, Type::Rain, 2, Zone::Board, Lane::Siege);
    const int melee = game.Add(1, Type::Damage, 3, Zone::Board, Lane::Melee);
    const int ranged = game.Add(1, Type::Heal, 3, Zone::Board, Lane::Ranged);
    const int siege = game.Add(1, Type::Shield, 3, Zone::Board, Lane::Siege);
    const int friendly = game.Add(0, Type::Damage, 3, Zone::Board, Lane::Melee);
    const int ownSiege = game.Add(0, Type::Shield, 3, Zone::Board, Lane::Siege);
    game.Resolve(Effect::Weather);
    Expect(game.cards[melee].value == 2 && game.cards[ranged].value == 2 && game.cards[siege].value == 3
        && game.cards[friendly].value == 3 && game.cards[ownSiege].value == 2, "only opposing matching rows reduced");
    Expect(game.cards[frost].value == 1 && game.cards[fog].value == 1 && game.cards[rain].value == 1,
           "weather sources each lose one lifetime point");
    game.Empty();
    const int frost0 = game.Add(0, Type::Frost, 2, Zone::Board, Lane::Melee);
    const int frost1 = game.Add(1, Type::Frost, 2, Zone::Board, Lane::Melee);
    const int dead = game.Add(1, Type::Damage, 1, Zone::Board, Lane::Melee);
    const int survivor = game.Add(0, Type::Damage, 2, Zone::Board, Lane::Melee);
    game.Resolve(Effect::Weather);
    Expect(game.cards[frost0].zone == Zone::Discard && game.cards[frost1].zone == Zone::Discard,
           "opposing expiring weather sources both activate");
    Expect(game.cards[dead].zone == Zone::Discard && game.cards[survivor].value == 1, "weather destruction");
    game.Resolve(Effect::Damage);
    Expect(game.players[0].hp == 30 && game.players[1].hp == 29, "destroyed sources cannot damage later");
    game.Empty();
    const int clear = game.Add(0, Type::Clear, 1, Zone::Board, Lane::Siege);
    std::vector<int> weather;
    for (int owner = 0; owner < 2; ++owner) {
        weather.push_back(game.Add(owner, Type::Frost, 2, Zone::Board, Lane::Melee));
        weather.push_back(game.Add(owner, Type::Fog, 2, Zone::Board, Lane::Ranged));
        weather.push_back(game.Add(owner, Type::Rain, 2, Zone::Board, Lane::Siege));
    }
    const int unchanged = game.Add(1, Type::Heal, 2, Zone::Board, Lane::Ranged);
    game.Resolve(Effect::Clear); game.Resolve(Effect::Weather);
    for (int id : weather) Expect(game.cards[id].zone == Zone::Discard, "clear removes every weather on both sides");
    Expect(game.cards[clear].zone == Zone::Discard && game.cards[unchanged].value == 2, "clear expires before weather");
    game.Empty();
    game.Add(0, Type::Fog, 2, Zone::Board, Lane::Ranged);
    const int reduced = game.Add(1, Type::Heal, 3, Zone::Board, Lane::Ranged);
    game.Resolve(Effect::Weather); game.Add(1, Type::Clear, 1, Zone::Board, Lane::Melee); game.Resolve(Effect::Clear);
    Expect(game.cards[reduced].value == 2, "clear does not restore previous reductions");
}
void ScorchAndHorn() {
    section = "scorch and horn";
    Game game; game.Empty(); KeepPlaying(game);
    const int scorch = game.Add(0, Type::Scorch, 2, Zone::Board, Lane::Siege);
    const int old = game.Add(1, Type::Damage, 5, Zone::Board, Lane::Melee);
    const int fresh = game.Add(1, Type::Damage, 2, Zone::Hand, Lane::Melee);
    Expect(game.Play(1, fresh, Lane::Melee), "target placement"); game.Resolve(Effect::Scorch);
    Expect(game.cards[fresh].suppressed && !game.cards[old].suppressed && game.cards[fresh].zone == Zone::Board,
           "scorch suppresses only the new opposing card, without destroying it");
    Expect(game.cards[scorch].value == 1, "scorch decay"); game.Resolve(Effect::Damage);
    Expect(game.players[0].hp == 25, "old damage acts, suppressed new damage does not");
    game.FinishTurn(); Expect(!game.cards[fresh].suppressed, "suppression expires at turn boundary");
    game.Resolve(Effect::Damage); Expect(game.players[0].hp == 18, "suppressed card resumes next turn");
    game.Empty();
    const int s0 = game.Add(0, Type::Scorch, 2, Zone::Hand), s1 = game.Add(1, Type::Scorch, 2, Zone::Hand);
    Expect(game.Play(0, s0, Lane::Melee) && game.Play(1, s1, Lane::Siege), "opposing scorch placement");
    game.Resolve(Effect::Scorch);
    Expect(game.cards[s0].suppressed && game.cards[s1].suppressed && game.cards[s0].value == 1 && game.cards[s1].value == 1,
           "opposing scorches evaluate simultaneously and both decay");
    game.Empty(); KeepPlaying(game); game.Add(0, Type::Scorch, 1, Zone::Board, Lane::Melee);
    const int spy = game.Add(1, Type::Spy, 2, Zone::Hand);
    Expect(game.Play(1, spy, Lane::Ranged), "spy target placement"); const size_t handSize = game.players[1].hand.size();
    game.Resolve(Effect::Scorch); game.Resolve(Effect::Spy);
    Expect(game.cards[spy].value == 1 && game.players[1].hand.size() == handSize, "suppressed spy cannot steal but decays");
    game.FinishTurn(); game.Resolve(Effect::Spy);
    Expect(game.cards[spy].zone == Zone::Discard && game.players[1].hand.size() == handSize + 1,
           "temporary resumes then expires next turn");
    game.Empty(); KeepPlaying(game); game.Add(0, Type::Scorch, 3, Zone::Board, Lane::Melee);
    const int small = game.Add(1, Type::Damage, 2, Zone::Hand, Lane::Siege);
    game.Play(1, small, Lane::Siege); game.Resolve(Effect::Scorch);
    Expect(!game.cards[small].suppressed, "below-threshold target is unaffected");
    game.Empty(); KeepPlaying(game);
    const int horn0 = game.Add(0, Type::Horn, 2, Zone::Board, Lane::Melee);
    const int horn1 = game.Add(0, Type::Horn, 2, Zone::Board, Lane::Siege);
    const int target = game.Add(0, Type::Damage, 3, Zone::Hand, Lane::Melee);
    game.Resolve(Effect::Horn);
    Expect(game.cards[target].value == 3 && game.cards[horn0].value == 1 && game.cards[horn1].value == 1,
           "horn arms future placement and decays without changing hand cards");
    game.FinishTurn(); Expect(game.Play(0, target, Lane::Melee) && game.cards[target].value == 6,
           "multiple horns double next-turn card once");
    game.Add(1, Type::Scorch, 5, Zone::Board, Lane::Melee); game.Resolve(Effect::Scorch);
    Expect(game.cards[target].suppressed, "horn applies before scorch threshold comparison");
    game.Empty(); KeepPlaying(game);
    const int horn = game.Add(0, Type::Horn, 1, Zone::Board, Lane::Melee);
    game.Resolve(Effect::Horn); game.FinishTurn();
    Expect(game.cards[horn].zone == Zone::Discard, "expired horn retains pending effect");
    UseLeader(game, 0, Leader::FoltestClear); game.Resolve(Effect::Leaders); game.FinishTurn();
    const int late = game.Add(0, Type::Damage, 3, Zone::Hand, Lane::Melee);
    Expect(game.Play(0, late, Lane::Melee) && game.cards[late].value == 3, "leader action lets horn opportunity expire");
}
void Recovery() {
    section = "medic and decoy";
    Game game; game.Empty(); KeepPlaying(game);
    const int medic = game.Add(0, Type::Medic, 2, Zone::Hand, Lane::Ranged);
    const int special = game.Add(0, Type::Clear, 1, Zone::Discard);
    game.cards[special].value = 0; game.cards[special].known = true; game.cards[special].suppressed = true;
    Expect(game.Play(0, medic, Lane::Ranged), "medic placement"); game.Resolve(Effect::Utility);
    Expect(game.cards[special].zone == Zone::Hand && game.cards[special].value == 1
        && !game.cards[special].suppressed, "medic revives specials and restores printed state");
    const int second = game.Add(0, Type::Shield, 2, Zone::Discard, Lane::Siege);
    game.Resolve(Effect::Utility); Expect(game.cards[second].zone == Zone::Discard, "medic activates at most once per turn");
    game.FinishTurn(); game.Resolve(Effect::Utility);
    Expect(game.cards[second].zone == Zone::Discard, "medic does not recover again on later turns without replay");
    game.FinishTurn();
    const int decoy = game.Add(0, Type::Decoy, 1, Zone::Hand);
    const int opposing = game.Add(1, Type::Heal, 2, Zone::Board, Lane::Ranged);
    Expect(!game.CanPlay(0, decoy, Lane::Ranged, opposing) && !game.CanPlay(0, decoy, Lane::Melee, medic)
        && !game.CanPlay(0, decoy, Lane::Ranged), "decoy requires matching friendly board target");
    const int weather = game.Add(0, Type::Fog, 2, Zone::Board, Lane::Ranged);
    Expect(game.CanPlay(0, decoy, Lane::Ranged, weather), "decoy can target a special");
    game.cards[medic].value = 4; Expect(game.Play(0, decoy, Lane::Ranged, medic), "valid decoy placement");
    Expect(game.cards[medic].zone == Zone::Board, "decoy swap waits until after scorch");
    game.Resolve(Effect::Utility);
    Expect(game.cards[medic].zone == Zone::Hand && game.cards[medic].value == 2
        && game.cards[decoy].zone == Zone::Board && game.cards[decoy].lane == Lane::Ranged,
           "decoy returns reset unit and remains in its lane");
    game.Resolve(Effect::Utility);
    Expect(game.cards[decoy].zone == Zone::Board && game.cards[decoy].value == 1, "decoy is inert without recurring decay");
    game.FinishTurn(); Expect(game.Play(0, medic, Lane::Ranged), "recalled medic replays"); game.Resolve(Effect::Utility);
    Expect(game.cards[second].zone == Zone::Hand, "replayed medic triggers its ability again");
    game.Empty(); KeepPlaying(game);
    const int emptyMedic = game.Add(0, Type::Medic, 2, Zone::Hand, Lane::Ranged);
    game.Play(0, emptyMedic, Lane::Ranged); game.Resolve(Effect::Utility);
    Expect(game.cards[emptyMedic].zone == Zone::Board && game.players[0].discard.empty(), "empty-discard medic is a safe no-op");
    game.Empty(); KeepPlaying(game); game.Add(1, Type::Scorch, 1, Zone::Board, Lane::Melee);
    const int suppressedMedic = game.Add(0, Type::Medic, 2, Zone::Hand, Lane::Ranged);
    const int deadUnit = game.Add(0, Type::Damage, 3, Zone::Discard, Lane::Melee);
    game.Play(0, suppressedMedic, Lane::Ranged); game.Resolve(Effect::Scorch); game.Resolve(Effect::Utility);
    Expect(game.cards[deadUnit].zone == Zone::Discard, "scorch cancels medic recovery");
    game.FinishTurn(); game.Resolve(Effect::Utility);
    Expect(game.cards[deadUnit].zone == Zone::Discard, "suppressed on-play ability is not deferred");
    game.Empty(); KeepPlaying(game); game.Add(1, Type::Scorch, 1, Zone::Board, Lane::Melee);
    const int unit = game.Add(0, Type::Damage, 3, Zone::Board, Lane::Melee);
    const int canceledDecoy = game.Add(0, Type::Decoy, 1, Zone::Hand);
    game.Play(0, canceledDecoy, Lane::Melee, unit); game.Resolve(Effect::Scorch); game.Resolve(Effect::Utility);
    Expect(game.cards[unit].zone == Zone::Board && game.cards[canceledDecoy].zone == Zone::Board,
           "scorch cancels decoy without returning target early");
    game.Empty(); const int lonely = game.Add(0, Type::Decoy, 1, Zone::Hand);
    Expect(!game.CanPlay(0, lonely, Lane::Melee) && !game.HasAction(0), "targetless decoy is not a playable action");
    game.Empty(); KeepPlaying(game);
    const int enemyMedic = game.Add(1, Type::Medic, 2, Zone::Hand, Lane::Ranged);
    const int enemyWeather = game.Add(1, Type::Frost, 2, Zone::Discard);
    game.cards[enemyWeather].value = 0;
    Expect(game.cards[enemyWeather].known, "discarded cards are public");
    game.Play(1, enemyMedic, Lane::Ranged); game.Resolve(Effect::Utility);
    Expect(game.cards[enemyWeather].zone == Zone::Hand && !game.cards[enemyWeather].known
        && game.cards[enemyWeather].value == 2, "enemy revival restores V without leaking its new hand entry");
    game.FinishTurn();
    const int enemyDecoy = game.Add(1, Type::Decoy, 1, Zone::Hand);
    Expect(game.Play(1, enemyDecoy, Lane::Ranged, enemyMedic), "enemy decoy placement");
    game.Resolve(Effect::Utility);
    Expect(game.cards[enemyMedic].zone == Zone::Hand && !game.cards[enemyMedic].known,
           "enemy decoy return clears prior public identity");

    // Every board type is a legal target; preserve its slot and make it replayable.
    for (Type type : {Type::Damage, Type::Heal, Type::Shield, Type::Spy, Type::Frost,
                      Type::Clear, Type::Fog, Type::Rain, Type::Horn, Type::Scorch, Type::Medic, Type::Decoy}) {
        for (int slot = 0; slot < 3; ++slot) {
            game.Empty(); KeepPlaying(game);
            const Lane lane = Weather(type) ? WeatherLane(type) : Lane::Ranged;
            for (int i = 0; i < slot; ++i) game.Add(0, Type::Shield, 1, Zone::Board, lane);
            const int target = game.Add(0, type, 2, Zone::Board, lane);
            for (int i = slot; i < 2; ++i) game.Add(0, Type::Shield, 1, Zone::Board, lane);
            const int replacement = game.Add(0, Type::Decoy, 1, Zone::Hand);
            game.cards[target].value = 4;
            Expect(game.Play(0, replacement, lane, target), "Decoy accepts every friendly board type");
            game.Resolve(Effect::Utility);
            Expect(game.players[0].board[slot] == replacement && game.cards[target].zone == Zone::Hand
                && game.cards[target].value == 2 && !game.cards[target].suppressed && game.Invariant(),
                "replacement preserves first/middle/last slot and restores returned card");
            game.FinishTurn();
            Expect(game.Play(0, target, lane, type == Type::Decoy ? replacement : -1), "returned card can be replayed");
        }
    }

    game.Empty(); KeepPlaying(game);
    game.Add(0, Type::Medic, 2, Zone::Board, Lane::Ranged);
    game.Add(0, Type::Medic, 2, Zone::Board, Lane::Ranged);
    game.Add(0, Type::Rain, 2, Zone::Discard);
    game.Add(0, Type::Shield, 2, Zone::Discard, Lane::Siege);
    const size_t beforeRecovery = game.players[0].hand.size();
    game.Resolve(Effect::Utility);
    Expect(game.players[0].discard.size() == 2 && game.players[0].hand.size() == beforeRecovery
        && game.Invariant(), "surviving medics do not recycle discarded cards without replay");
}

void EnemyResponses() {
    section = "random enemy card response";
    Game game;
    for (unsigned seed = 0; seed < 100; ++seed) {
        game.Empty(seed);
        EnableLeader(game, 1, Leader::Emhyr);
        const int decoy = game.Add(1, Type::Decoy, 1, Zone::Hand);
        Expect(game.ChooseAction(1).kind == Action::UseLeader, "targetless hand falls back to unused leader");
        game.players[1].leaderUsed = true;
        Expect(game.ChooseAction(1).kind == Action::Skip, "no card or leader safely skips");
        game.players[1].leaderUsed = false;
        const int target = game.Add(1, Type::Fog, 2, Zone::Board);
        const Action swap = game.ChooseAction(1);
        Expect(swap.kind == Action::PlayCard && swap.card == decoy && swap.target == target
            && swap.lane == Lane::Ranged && game.CanAct(1, swap), "enemy decoy selects a friendly special");
        game.Add(1, Type::Damage, 3, Zone::Hand, Lane::Melee);
        game.Add(1, Type::Shield, 2, Zone::Hand, Lane::Siege);
        for (int n = 0; n < 10; ++n) {
            const Action action = game.ChooseAction(1);
            Expect(action.kind == Action::PlayCard && game.CanAct(1, action), "playable hand always gets a legal card response");
        }
    }
}
void Leaders() {
    section = "leader abilities";
    Game game; game.Empty(); KeepPlaying(game);
    const int weather0 = game.Add(0, Type::Frost, 2, Zone::Board, Lane::Melee);
    const int weather1 = game.Add(1, Type::Rain, 2, Zone::Board, Lane::Siege);
    UseLeader(game, 0, Leader::FoltestClear);
    Expect(game.players[0].leaderUsed && !game.CanUseLeader(0), "leader consumed at commitment");
    game.Resolve(Effect::Leaders);
    Expect(game.cards[weather0].zone == Zone::Discard && game.cards[weather1].zone == Zone::Discard, "leader clears both weather sides");
    game.FinishTurn(); Expect(!game.CanUseLeader(0), "leader cannot be reused");
    game.Empty(); for (int i = 0; i < 5; ++i) game.Add(1, Type::Damage, i + 1, Zone::Hand, Lane::Melee);
    UseLeader(game, 0, Leader::Emhyr); const auto reveals = game.Resolve(Effect::Leaders); std::set<int> seen;
    for (const Event& event : reveals) if (event.kind == Event::Reveal) {
        Expect(event.owner == 0, "reveal event identifies viewer"); seen.insert(event.card);
    }
    int known = 0; for (int id : game.players[1].hand) if (game.cards[id].known) ++known;
    Expect(seen.size() == 3 && known == 3, "Emhyr reveals three distinct hand cards");
    const int revealed = *seen.begin();
    Expect(game.Play(1, revealed, Lane::Melee) && game.cards[revealed].zone == Zone::Board,
           "revealed card remains legally playable");
    game.Move(revealed, 1, Zone::Hand);
    Expect(!game.cards[revealed].known, "revealed identity resets on a new hand entry");
    game.Empty(); const int only = game.Add(1, Type::Shield, 1, Zone::Hand, Lane::Siege);
    UseLeader(game, 0, Leader::Emhyr); game.Resolve(Effect::Leaders); Expect(game.cards[only].known, "Emhyr handles fewer than three cards");
    game.Empty(); UseLeader(game, 0, Leader::Emhyr); game.Resolve(Effect::Leaders);
    Expect(game.players[0].leaderUsed && game.Invariant(), "targetless leader safely consumed");
    game.Empty(); KeepPlaying(game);
    const int melee = game.Add(0, Type::Damage, 3, Zone::Board, Lane::Melee);
    const int other = game.Add(0, Type::Shield, 2, Zone::Board, Lane::Siege);
    const int temporary = game.Add(0, Type::Spy, 2, Zone::Board, Lane::Melee);
    const int enemy = game.Add(1, Type::Damage, 3, Zone::Board, Lane::Melee);
    UseLeader(game, 0, Leader::Eredin); game.Resolve(Effect::Leaders);
    Expect(game.EffectiveValue(melee) == 6 && game.EffectiveValue(temporary) == 4
        && game.EffectiveValue(other) == 2 && game.EffectiveValue(enemy) == 3, "Eredin doubles only own melee values");
    game.Resolve(Effect::Spy);
    Expect(game.cards[temporary].value == 1 && game.EffectiveValue(temporary) == 2, "Eredin never extends temporary lifetime");
    game.Resolve(Effect::Damage); Expect(game.players[1].hp == 24, "Eredin affects damage magnitude"); game.FinishTurn();
    Expect(game.cards[melee].value == 3 && game.EffectiveValue(melee) == 3, "Eredin ends without compounding base value");
    game.Empty(17); KeepPlaying(game);
    const int top0 = game.Add(1, Type::Damage, 3, Zone::Board, Lane::Melee);
    const int top1 = game.Add(1, Type::Heal, 3, Zone::Board, Lane::Melee);
    const int low = game.Add(1, Type::Shield, 2, Zone::Board, Lane::Melee);
    const int ranged = game.Add(1, Type::Damage, 8, Zone::Board, Lane::Ranged);
    UseLeader(game, 0, Leader::Francesca); game.Resolve(Effect::Leaders);
    Expect((game.cards[top0].zone == Zone::Discard) != (game.cards[top1].zone == Zone::Discard), "Francesca destroys exactly one tied maximum");
    Expect(game.cards[low].zone == Zone::Board && game.cards[ranged].zone == Zone::Board, "Francesca ignores other rows and weaker units");
    game.Empty(); UseLeader(game, 0, Leader::Francesca); game.Resolve(Effect::Leaders);
    Expect(game.players[0].leaderUsed && game.Invariant(), "Francesca handles empty melee");
    for (Lane lane : {Lane::Melee, Lane::Siege}) {
        game.Empty(); KeepPlaying(game); UseLeader(game, 0, Leader::FoltestSiege);
        game.Resolve(Effect::Leaders); game.FinishTurn(); const int target = game.Add(0, Type::Damage, 3, Zone::Hand, lane);
        Expect(game.Play(0, target, lane), "siege horn next-turn placement");
        Expect(game.cards[target].value == (lane == Lane::Siege ? 6 : 3), "siege horn lane restriction");
        game.FinishTurn(); const int late = game.Add(0, Type::Shield, 2, Zone::Hand, Lane::Siege);
        Expect(game.Play(0, late, Lane::Siege) && game.cards[late].value == 2, "siege horn expires after next turn");
    }
    game.Empty(); KeepPlaying(game); const int doomed = game.Add(1, Type::Scorch, 3, Zone::Hand);
    game.Play(1, doomed, Lane::Melee); UseLeader(game, 0, Leader::Francesca);
    game.Resolve(Effect::Leaders); game.Resolve(Effect::Scorch);
    Expect(game.cards[doomed].zone == Zone::Discard, "leader destruction precedes scorch source capture");
}
void RoundsAndExhaustion() {
    section = "rounds and exhaustion";
    Game game; game.Empty(); game.players[0].hp = game.players[1].hp = 1;
    game.Add(0, Type::Damage, 1, Zone::Board, Lane::Melee); game.Add(1, Type::Damage, 1, Zone::Board, Lane::Melee);
    game.Add(0, Type::Heal, 3, Zone::Board, Lane::Ranged);
    game.Resolve(Effect::Damage);
    Expect(game.roundResult == Result::Draw && game.result == Result::Playing
        && game.players[0].crystals == 2 && game.players[1].crystals == 2,
           "lethal records round outcome while crystal scoring waits for turn completion");
    game.CheckLethal(); game.Resolve(Effect::Heal);
    Expect(game.players[0].hp == 0 && game.players[0].crystals == 2, "lethal blocks healing without scoring early");
    game.FinishTurn();
    Expect(game.roundResult == Result::Draw && game.result == Result::Playing && game.players[0].hp == 0
        && game.players[0].crystals == 1 && game.players[1].crystals == 1, "simultaneous lethal precedes heal and costs both crystals");
    game.FinishTurn(); Expect(game.players[0].crystals == 1 && game.players[1].crystals == 1, "round award is idempotent");
    Expect(game.NextRound() && game.round == 2, "next round begins"); game.FinishTurn();
    Expect(game.result == Result::Draw && game.players[0].crystals == 0 && game.players[1].crystals == 0
        && !game.NextRound(), "simultaneous last-crystal loss draws match");
    game.Empty(); game.players[1].hp = 29; game.players[1].shield = 100; game.FinishTurn();
    Expect(game.roundResult == Result::PlayerWin && game.result == Result::Playing, "exhaustion uses HP only and ends round, not match");
    game.NextRound(); game.players[1].hp = 0; game.CheckLethal(); game.FinishTurn();
    Expect(game.result == Result::PlayerWin && game.round == 2, "two straight wins end match");
    game.Empty(); EnableLeader(game, 0, Leader::Emhyr); EnableLeader(game, 1, Leader::FoltestClear);
    game.players[1].hp = 29; game.FinishTurn();
    Expect(game.roundResult == Result::PlayerWin && !game.players[0].leaderUsed && !game.players[1].leaderUsed,
           "empty hands end round even with both leaders unused");
    game.Empty(); EnableLeader(game, 0, Leader::Emhyr); EnableLeader(game, 1, Leader::FoltestClear);
    game.FinishTurn(); Expect(game.roundResult == Result::Draw, "equal HP exhaustion draws with unused leaders");
    // Reproduce the reported loss: lose the first round, then draw at 30 HP.
    game.Empty(); game.players[0].hp = 0; game.players[1].hp = 5;
    game.FinishTurn();
    Expect(game.roundResult == Result::EnemyWin && game.players[0].crystals == 1
        && game.players[1].crystals == 2, "first loss leaves crystals at one versus two");
    Expect(game.NextRound() && game.players[0].hp == 30 && game.players[1].hp == 30,
           "next round restores both HP equally");
    game.FinishTurn();
    Expect(game.roundResult == Result::Draw && game.result == Result::EnemyWin
        && game.players[0].crystals == 0 && game.players[1].crystals == 1,
           "equal HP draws the round; prior crystal deficit loses the match");
    game.FinishTurn();
    Expect(game.players[0].hp == 30 && game.players[1].hp == 30
        && game.players[0].crystals == 0 && game.players[1].crystals == 1,
           "finished match preserves final HP and scores each round once");
    game.Empty();
    const int lastMedic = game.Add(0, Type::Medic, 2, Zone::Hand, Lane::Ranged);
    const int recovered = game.Add(0, Type::Damage, 1, Zone::Discard, Lane::Melee);
    game.Play(0, lastMedic, Lane::Ranged);
    ResolveTurn(game);
    Expect(game.roundResult == Result::Playing && game.cards[recovered].zone == Zone::Hand,
           "final effect cycle recovery prevents premature hand exhaustion");
    game.Empty(); game.players[0].hp = 0; game.CheckLethal(); game.FinishTurn();
    game.NextRound(); game.players[1].hp = 0; game.CheckLethal(); game.FinishTurn();
    Expect(game.result == Result::Playing && game.players[0].crystals == 1 && game.players[1].crystals == 1, "split rounds require third");
    game.NextRound(); game.players[0].hp = 0; game.CheckLethal(); game.FinishTurn();
    Expect(game.result == Result::EnemyWin && game.round == 3, "third-round winner wins match");
    game.Empty(); KeepPlaying(game); const int board = game.Add(0, Type::Damage, 3, Zone::Board, Lane::Melee);
    const int top0 = game.Add(0, Type::Shield, 2, Zone::Deck, Lane::Siege);
    const int top1 = game.Add(1, Type::Heal, 2, Zone::Deck, Lane::Ranged);
    const int discard = game.Add(0, Type::Fog, 2, Zone::Discard, Lane::Ranged);
    game.players[0].shield = 7; game.players[0].leader = Leader::Emhyr; game.players[0].leaderUsed = true;
    const size_t hand0 = game.players[0].hand.size(), hand1 = game.players[1].hand.size();
    game.players[1].hp = 0; game.CheckLethal(); game.FinishTurn(); Expect(game.NextRound(), "round transition accepted");
    Expect(game.players[0].hp == 30 && game.players[1].hp == 30 && game.players[0].shield == 0
        && game.players[0].board.empty() && game.cards[board].zone == Zone::Discard && game.cards[discard].zone == Zone::Discard,
           "transition clears board and shield while preserving discard");
    Expect(game.players[0].hand.size() == hand0 + 1 && game.players[1].hand.size() == hand1 + 1
        && game.cards[top0].zone == Zone::Hand && game.cards[top1].zone == Zone::Hand, "round two draws exactly one per preserved hand");
    Expect(game.players[0].leaderUsed && game.players[0].leader == Leader::Emhyr && !game.NextRound(), "leader use persists and active round cannot advance");
    game.Empty(); EnableLeader(game, 0, Leader::Emhyr);
    Expect(!game.Commit(0, Action(Action::Skip)), "cannot skip an available leader");
    game.Empty(); game.Add(0, Type::Damage, 2, Zone::Board, Lane::Melee);
    game.players[1].hp = 29;
    Expect(game.Commit(0, Action(Action::Skip)) && game.Commit(1, Action(Action::Skip)), "both unavailable sides skip");
    Expect(!game.AnyCommitted(), "two skips do not trigger an effect cycle"); game.FinishTurn();
    Expect(game.roundResult == Result::PlayerWin && game.players[1].hp == 29, "exhaustion settles HP without extra activation");
    game.Empty(); game.Add(0, Type::Spy, 2, Zone::Board, Lane::Melee);
    game.Add(0, Type::Damage, 2, Zone::Board, Lane::Melee);
    const int enemyPlay = game.Add(1, Type::Damage, 1, Zone::Hand, Lane::Melee);
    const int stolen = game.Add(1, Type::Heal, 1, Zone::Hand, Lane::Ranged);
    Expect(game.Commit(0, Action(Action::Skip)), "unavailable side automatically skips");
    Expect(game.Play(1, enemyPlay, Lane::Melee), "opponent acts while side skips"); ResolveTurn(game);
    Expect(game.cards[stolen].owner == 0 && game.CanPlay(0, stolen, Lane::Ranged), "spy recovery enables skipped side");
    Expect(game.players[1].hp == 28 && game.players[0].hp == 29, "both boards resolve when only one side acts");
    game.Empty(); game.Add(0, Type::Decoy, 1, Zone::Hand); game.Add(1, Type::Decoy, 1, Zone::Hand); game.FinishTurn();
    Expect(game.roundResult == Result::Draw, "two unusable decoys cannot stall exhaustion");
    game.Reset(7);
    Expect(game.turn == 1 && game.round == 1 && game.result == Result::Playing && game.roundResult == Result::Playing
        && game.players[0].hp == 30 && game.players[0].shield == 0 && game.players[0].crystals == 2
        && !game.players[0].leaderUsed && game.players[0].discard.empty() && game.Invariant(),
           "restart clears all prior match state");
}
void SeededMatches() {
    section = "seeded full matches";
    for (unsigned seed = 0; seed < 1000; ++seed) {
        Game game(seed); int decisions = 0;
        while (game.result == Result::Playing) {
            if (game.roundResult != Result::Playing) {
                Expect(game.NextRound(), "seeded next round"); Expect(game.round <= 3, "at most three rounds");
            }
            for (int owner = 0; owner < 2; ++owner) {
                const Action action = game.ChooseAction(owner);
                Expect(game.CanAct(owner, action) && game.Commit(owner, action), "random action legal and committed");
                Expect(game.Invariant(), "invariant after random action");
            }
            if (game.AnyCommitted()) ResolveTurn(game); else game.FinishTurn();
            for (const Player& player : game.players)
                Expect(player.hp >= 0 && player.hp <= Game::MaxHP && player.shield >= 0
                    && player.crystals >= 0 && player.crystals <= 2, "bounded HP, crystals and shield");
            if (++decisions > 2000) {
                std::ostringstream message; message << "seed " << seed << " exceeded test watchdog at round " << game.round << ", turn " << game.turn;
                throw std::runtime_error(message.str());
            }
        }
        Expect(game.Invariant() && (game.players[0].crystals == 0 || game.players[1].crystals == 0), "completed match invariant and final crystal");
    }
}
}  // namespace
int main() {
    try {
        OpeningAndLegality(); CoreRecurrence(); WeatherAndClear(); ScorchAndHorn(); Recovery(); EnemyResponses(); Leaders(); RoundsAndExhaustion(); SeededMatches();
        std::cout << "PASS: " << checks << " checks, including all advanced rules and 1000 complete seeded matches\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n'; return 1;
    }
}
#endif
