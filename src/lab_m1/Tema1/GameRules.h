#pragma once

// Homework-specific GWENT rules; no graphics dependencies.
#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace tema1 {
enum class Type { Damage, Heal, Shield, Spy, Frost, Clear, Fog, Rain, Horn, Scorch, Medic, Decoy };
enum class Zone { Deck, Hand, Board, Discard };
enum class Lane { None = -1, Melee = 0, Ranged = 1, Siege = 2, Any = 3 };
enum class Leader { FoltestClear, Emhyr, Eredin, Francesca, FoltestSiege, Count };
enum class Result { Playing, PlayerWin, EnemyWin, Draw };
enum class Effect { Leaders, Scorch, Utility, Spy, Clear, Weather, Horn, Shield, Damage, Heal,
                    Frost = Weather };
inline const std::array<Effect, 10>& Phases() {
    static const std::array<Effect, 10> phases = {{Effect::Leaders, Effect::Scorch,
        Effect::Utility, Effect::Spy, Effect::Clear, Effect::Weather, Effect::Horn,
        Effect::Shield, Effect::Damage, Effect::Heal}};
    return phases;
}
inline const char* PhaseName(Effect e) {
    static const char* names[] = {"LEADERS", "SCORCH", "MEDIC / DECOY", "SPY", "CLEAR SKY",
        "WEATHER", "WAR HORN", "SHIELD", "DAMAGE", "HEAL"};
    return names[static_cast<int>(e)];
}
inline const char* Name(Type t) {
    static const char* names[] = {"DAMAGE", "HEAL", "SHIELD", "SPY", "FROST", "CLEAR SKY",
        "FOG", "RAIN", "WAR HORN", "SCORCH", "MEDIC", "DECOY"};
    return names[static_cast<int>(t)];
}
inline const char* LaneName(Lane l) {
    if (l == Lane::Melee) return "MELEE";
    if (l == Lane::Ranged) return "RANGED";
    if (l == Lane::Siege) return "SIEGE";
    return l == Lane::Any ? "ANY LANE" : "NO LANE";
}
inline const char* LeaderName(Leader l) {
    static const char* names[] = {"FOLTEST: CLEAR SKY", "EMHYR", "EREDIN", "FRANCESCA", "FOLTEST: SIEGE HORN"};
    const int n = static_cast<int>(l);
    return n >= 0 && n < static_cast<int>(Leader::Count) ? names[n] : "NO LEADER";
}
inline const char* LeaderDescription(Leader l) {
    static const char* descriptions[] = {
        "Remove every weather card on both sides.",
        "Reveal up to three random cards in the opposing hand.",
        "Double your melee cards' effective values for this turn.",
        "Destroy one strongest opposing melee card; break ties randomly.",
        "Double your next turn's siege card. The opportunity expires that turn."};
    const int n = static_cast<int>(l);
    return n >= 0 && n < static_cast<int>(Leader::Count) ? descriptions[n] : "";
}
inline const char* Description(Type t) {
    static const char* descriptions[] = {
        "Deal V damage each turn, after shields.",
        "Restore V HP each turn, up to 30. Lethal damage cannot be healed.",
        "Add V shield each turn. Shield carries between turns.",
        "Steal one random opposing hand card each turn; lose one V.",
        "Reduce opposing melee cards by one each turn; lose one V.",
        "Remove all weather on both sides; lose one V each turn.",
        "Reduce opposing ranged cards by one each turn; lose one V.",
        "Reduce opposing siege cards by one each turn; lose one V.",
        "Double your next turn's played card; lose one V. Horns do not stack.",
        "Cancel the opposing card played this turn if its V is at least this V; lose one V.",
        "On play, return a random discarded card to your hand at printed V.",
        "Play over a friendly unit to return it at printed V. Decoy stays inert."};
    return descriptions[static_cast<int>(t)];
}
inline bool Weather(Type t) { return t == Type::Frost || t == Type::Fog || t == Type::Rain; }
inline Lane WeatherLane(Type t) { return t == Type::Frost ? Lane::Melee : t == Type::Fog ? Lane::Ranged : Lane::Siege; }
inline bool Temporary(Type t) { return t == Type::Spy || Weather(t) || t == Type::Clear || t == Type::Horn || t == Type::Scorch; }
inline bool Unit(Type t) { return t == Type::Damage || t == Type::Heal || t == Type::Shield || t == Type::Medic; }
inline Type EffectType(Effect e) {
    if (e == Effect::Scorch) return Type::Scorch;
    if (e == Effect::Spy) return Type::Spy;
    if (e == Effect::Clear) return Type::Clear;
    if (e == Effect::Weather) return Type::Frost;
    if (e == Effect::Horn) return Type::Horn;
    if (e == Effect::Shield) return Type::Shield;
    if (e == Effect::Heal) return Type::Heal;
    return Type::Damage;
}
struct Card {
    int id, owner, value, printed;
    Type type;
    Zone zone;
    Lane printedLane, lane;
    bool known = false, suppressed = false;
    Card(int i, int o, Type t, int v, Zone z, Lane allowed = Lane::Any)
        : id(i), owner(o), value(v), printed(v), type(t), zone(z), printedLane(allowed), lane(Lane::None) {}
};
struct Player {
    int hp = 30, shield = 0, crystals = 2;
    std::vector<int> deck, hand, board, discard;
    Leader leader = Leader::FoltestClear;
    bool leaderUsed = false, passed = false;
};
struct Action {
    enum Kind { PlayCard, UseLeader, Pass, Skip } kind;
    int card;
    Lane lane;
    int target;
    Action(Kind k = Skip, int c = -1, Lane l = Lane::None, int t = -1) : kind(k), card(c), lane(l), target(t) {}
};
struct Event {
    enum Kind { Transfer, Destroy, Value, Guard, Hit, Restore, Return, Revive, Reveal,
                Suppress, Boost, LeaderUsed, RoundEnd, RoundStart } kind;
    int owner, card, amount;
    Event(Kind k, int o, int c, int n) : kind(k), owner(o), card(c), amount(n) {}
};
class Game {
 public:
    static const int MaxHP = 30;
    std::array<Player, 2> players;
    std::vector<Card> cards;
    Result result = Result::Playing, roundResult = Result::Playing;
    int round = 1, turn = 1;
    std::mt19937 random;
    std::array<Action, 2> actions;
    std::array<bool, 2> committed = {{false, false}};
    std::array<bool, 2> utilityUsed = {{false, false}};
    std::array<int, 2> played = {{-1, -1}}, targets = {{-1, -1}};
    std::array<bool, 2> meleeBoost = {{false, false}};
    std::array<int, 2> hornTurn = {{-1, -1}};
    std::array<Lane, 2> hornLane = {{Lane::None, Lane::None}};
    explicit Game(unsigned seed = 1) { Reset(seed); }
    static void Remove(std::vector<int>& list, int id) { list.erase(std::remove(list.begin(), list.end(), id), list.end()); }
    std::vector<int>& List(int owner, Zone zone) {
        Player& p = players[owner];
        if (zone == Zone::Deck) return p.deck;
        if (zone == Zone::Hand) return p.hand;
        if (zone == Zone::Board) return p.board;
        return p.discard;
    }
    int Add(int owner, Type type, int value, Zone zone, Lane allowed = Lane::Any) {
        if (Weather(type)) allowed = WeatherLane(type);
        if (type == Type::Medic && allowed == Lane::Any) allowed = Lane::Ranged;
        const int id = static_cast<int>(cards.size());
        cards.emplace_back(id, owner, type, value, zone, allowed);
        Card& c = cards.back();
        if (zone == Zone::Board) c.lane = allowed == Lane::Any ? Lane::Melee : allowed;
        c.known = owner == 0 || zone == Zone::Board || zone == Zone::Discard;
        List(owner, zone).push_back(id);
        return id;
    }
    void Empty(unsigned seed = 1) {
        cards.clear(); players = std::array<Player, 2>();
        for (Player& p : players) p.leaderUsed = true;
        result = roundResult = Result::Playing; roundScored = false; round = turn = 1; random.seed(seed);
        hornTurn = {{-1, -1}}; hornLane = {{Lane::None, Lane::None}}; ClearTurn();
    }
    void Reset(unsigned seed, bool deal = true) {
        Empty(seed);
        for (int o = 0; o < 2; ++o) {
            players[o].leaderUsed = false;
            int lane = 0;
            for (int v = 1; v <= 3; ++v) for (int n = 0; n < 4; ++n)
                Add(o, Type::Damage, v, Zone::Deck, static_cast<Lane>(lane++ % 3));
            for (Type t : {Type::Heal, Type::Shield}) for (int v = 1; v <= 2; ++v) for (int n = 0; n < 2; ++n)
                Add(o, t, v, Zone::Deck, static_cast<Lane>(lane++ % 3));
            for (int n = 0; n < 2; ++n) Add(o, Type::Medic, 2, Zone::Deck, Lane::Ranged);
            for (Type t : {Type::Spy, Type::Frost, Type::Fog, Type::Rain, Type::Horn, Type::Scorch}) Add(o, t, 2, Zone::Deck);
            Add(o, Type::Clear, 1, Zone::Deck); Add(o, Type::Decoy, 1, Zone::Deck);
            std::shuffle(players[o].deck.begin(), players[o].deck.end(), random);
        }
        std::uniform_int_distribution<int> leader(0, static_cast<int>(Leader::Count) - 1);
        players[1].leader = static_cast<Leader>(leader(random));
        if (deal) for (int i = 0; i < 10; ++i) { Draw(0); Draw(1); }
    }
    void Move(int id, int owner, Zone zone) {
        Card& c = cards.at(id); Remove(List(c.owner, c.zone), id); c.owner = owner; c.zone = zone;
        if (zone != Zone::Board) { c.lane = Lane::None; c.suppressed = false; }
        // Hand inspection lasts only while that exact card remains in that hand.
        c.known = owner == 0 || zone == Zone::Board || zone == Zone::Discard;
        List(owner, zone).push_back(id);
    }
    int Draw(int owner) {
        if (players[owner].deck.empty()) return -1;
        const int id = players[owner].deck.back(); Move(id, owner, Zone::Hand); return id;
    }
    std::vector<Lane> LegalLanes(int owner, int id) const {
        std::vector<Lane> lanes;
        if (!Running() || !Owner(owner) || players[owner].passed || !HandCard(owner, id)) return lanes;
        const Card& c = cards[id];
        if (c.type == Type::Decoy) {
            for (int target : players[owner].board)
                if (Unit(cards[target].type) && cards[target].value > 0
                    && std::find(lanes.begin(), lanes.end(), cards[target].lane) == lanes.end()) lanes.push_back(cards[target].lane);
            std::sort(lanes.begin(), lanes.end(), [](Lane a, Lane b) { return static_cast<int>(a) < static_cast<int>(b); });
        } else if (c.printedLane == Lane::Any) lanes = {Lane::Melee, Lane::Ranged, Lane::Siege};
        else if (RealLane(c.printedLane)) lanes.push_back(c.printedLane);
        return lanes;
    }
    bool CanPlay(int owner, int id, Lane lane = Lane::None, int target = -1) const {
        if (lane == Lane::None) {
            const auto lanes = LegalLanes(owner, id);
            if (!lanes.empty()) lane = lanes.front();
        }
        return Owner(owner) && !committed[owner] && LegalPlacement(owner, id, lane, target);
    }
    bool CanUseLeader(int owner) const {
        return Running() && Owner(owner) && !committed[owner] && !players[owner].passed && !players[owner].leaderUsed;
    }
    // Availability for a future turn; Pass and the current commitment are excluded.
    bool HasAction(int owner) const {
        if (!Running() || !Owner(owner) || players[owner].passed) return false;
        if (!players[owner].leaderUsed) return true;
        for (int id : players[owner].hand) if (!LegalLanes(owner, id).empty()) return true;
        return false;
    }
    bool CanAct(int owner, const Action& a) const {
        if (!Running() || !Owner(owner) || committed[owner]) return false;
        if (a.kind == Action::Skip) return players[owner].passed || !HasAction(owner);
        if (players[owner].passed) return false;
        if (a.kind == Action::Pass) return true;
        if (a.kind == Action::UseLeader) return CanUseLeader(owner);
        return a.kind == Action::PlayCard && LegalPlacement(owner, a.card, a.lane, a.target);
    }
    bool Commit(int owner, const Action& requested) {
        if (!CanAct(owner, requested)) return false;
        Action a = requested;
        committed[owner] = true; actions[owner] = a;
        if (a.kind == Action::PlayCard) {
            Card& c = cards[a.card];
            if (hornTurn[owner] == turn && (hornLane[owner] == Lane::Any || hornLane[owner] == a.lane)) c.value *= 2;
            Move(a.card, owner, Zone::Board); c.lane = a.lane; c.suppressed = false;
            played[owner] = a.card; targets[owner] = a.target;
        } else if (a.kind == Action::UseLeader) players[owner].leaderUsed = true;
        else if (a.kind == Action::Pass) players[owner].passed = true;
        if (hornTurn[owner] == turn) { hornTurn[owner] = -1; hornLane[owner] = Lane::None; }
        return true;
    }
    bool Play(int owner, int id, Lane lane = Lane::None, int target = -1) {
        if (lane == Lane::None) {
            const auto lanes = LegalLanes(owner, id);
            if (!lanes.empty()) lane = lanes.front();
        }
        return Commit(owner, Action(Action::PlayCard, id, lane, target));
    }
    bool AnyCommitted() const {
        for (int o = 0; o < 2; ++o) if (committed[o] && (actions[o].kind == Action::PlayCard || actions[o].kind == Action::UseLeader)) return true;
        return false;
    }
    Action ChooseAction(int owner) {
        if (!Running() || !Owner(owner) || committed[owner]) return Action();
        if (players[owner].passed) return Action(Action::Skip);
        std::vector<Action> choices;
        for (int id : players[owner].hand) if (!LegalLanes(owner, id).empty()) choices.emplace_back(Action::PlayCard, id);
        if (!players[owner].leaderUsed) choices.emplace_back(Action::UseLeader);
        if (choices.empty()) return Action(Action::Skip);
        choices.emplace_back(Action::Pass);
        Action a = choices[Pick(static_cast<int>(choices.size()))];
        if (a.kind == Action::PlayCard) {
            const auto lanes = LegalLanes(owner, a.card); a.lane = lanes[Pick(static_cast<int>(lanes.size()))];
            if (cards[a.card].type == Type::Decoy) {
                std::vector<int> candidates;
                for (int id : players[owner].board) if (Unit(cards[id].type) && cards[id].lane == a.lane) candidates.push_back(id);
                a.target = candidates[Pick(static_cast<int>(candidates.size()))];
            }
        }
        return a;
    }
    int Choose(int owner) {
        std::vector<int> choices;
        for (int id : players[owner].hand) if (!LegalLanes(owner, id).empty()) choices.push_back(id);
        return choices.empty() ? -1 : choices[Pick(static_cast<int>(choices.size()))];
    }
    int EffectiveValue(int id) const {
        const Card& c = cards.at(id);
        return c.value * (c.zone == Zone::Board && c.lane == Lane::Melee && meleeBoost[c.owner] ? 2 : 1);
    }
    std::vector<int> Sources(Type type) const {
        std::vector<int> ids;
        for (const Player& p : players) for (int id : p.board) if (cards[id].type == type && cards[id].value > 0) ids.push_back(id);
        return ids;
    }
    void Destroy(int id, std::vector<Event>& events) {
        if (!Valid(id) || cards[id].zone != Zone::Board) return;
        const int owner = cards[id].owner; Move(id, owner, Zone::Discard); events.emplace_back(Event::Destroy, owner, id, 0);
    }
    void Decay(const std::vector<int>& sources, std::vector<Event>& events) {
        for (int id : sources) if (cards[id].zone == Zone::Board) {
            --cards[id].value; events.emplace_back(Event::Value, cards[id].owner, id, -1);
            if (cards[id].value <= 0) Destroy(id, events);
        }
    }
    // Each phase snapshots its sources. Cancellation never freezes lifetime.
    std::vector<Event> Resolve(Effect phase) {
        std::vector<Event> events;
        if (!Running()) return events;
        if (phase == Effect::Leaders) ResolveLeaders(events);
        else if (phase == Effect::Scorch) {
            const auto sources = Sources(Type::Scorch); std::vector<int> cancel;
            for (int id : sources) {
                const int target = played[1 - cards[id].owner];
                if (OnBoard(target) && EffectiveValue(target) >= EffectiveValue(id)) cancel.push_back(target);
            }
            for (int id : cancel) if (!cards[id].suppressed) {
                cards[id].suppressed = true; events.emplace_back(Event::Suppress, cards[id].owner, id, 1);
            }
            Decay(sources, events);
        } else if (phase == Effect::Utility) ResolveUtility(events);
        else if (phase == Effect::Spy) {
            const auto sources = Sources(Type::Spy);
            std::array<std::vector<int>, 2> available = {{players[0].hand, players[1].hand}};
            std::vector<std::pair<int, int>> transfers;
            for (int id : sources) if (!cards[id].suppressed) {
                const int owner = cards[id].owner; auto& candidates = available[1 - owner];
                if (candidates.empty()) continue;
                const int index = Pick(static_cast<int>(candidates.size())); transfers.emplace_back(candidates[index], owner);
                candidates.erase(candidates.begin() + index);
            }
            for (const auto& transfer : transfers) {
                Move(transfer.first, transfer.second, Zone::Hand); events.emplace_back(Event::Transfer, transfer.second, transfer.first, 1);
            }
            Decay(sources, events);
        } else if (phase == Effect::Clear) {
            const auto sources = Sources(Type::Clear); bool clear = false;
            for (int id : sources) if (!cards[id].suppressed) clear = true;
            if (clear) ClearWeather(events);
            Decay(sources, events);
        } else if (phase == Effect::Weather) ResolveWeather(events);
        else if (phase == Effect::Horn) {
            const auto sources = Sources(Type::Horn);
            for (int id : sources) if (!cards[id].suppressed) ArmHorn(cards[id].owner, Lane::Any, events);
            Decay(sources, events);
        } else {
            const auto sources = Sources(EffectType(phase)); std::array<int, 2> totals = {{0, 0}};
            for (int id : sources) if (!cards[id].suppressed) totals[cards[id].owner] += EffectiveValue(id);
            for (int owner = 0; owner < 2; ++owner) {
                Player& p = players[owner];
                if (phase == Effect::Shield) {
                    p.shield += totals[owner]; if (totals[owner]) events.emplace_back(Event::Guard, owner, -1, totals[owner]);
                } else if (phase == Effect::Damage) {
                    const int absorbed = std::min(p.shield, totals[1 - owner]); p.shield -= absorbed;
                    const int damage = totals[1 - owner] - absorbed; p.hp = std::max(0, p.hp - damage);
                    if (totals[1 - owner]) events.emplace_back(Event::Hit, owner, -1, damage);
                } else if (phase == Effect::Heal) {
                    const int old = p.hp; p.hp = std::min(static_cast<int>(MaxHP), p.hp + totals[owner]);
                    if (p.hp != old) events.emplace_back(Event::Restore, owner, -1, p.hp - old);
                }
            }
            if (phase == Effect::Damage) {
                CheckLethal();
                if (roundResult != Result::Playing) events.emplace_back(Event::RoundEnd, -1, -1, static_cast<int>(roundResult));
            }
        }
        return events;
    }
    void CheckLethal() {
        if (!Running()) return;
        if (players[0].hp <= 0 && players[1].hp <= 0) EndRound(Result::Draw);
        else if (players[0].hp <= 0) EndRound(Result::EnemyWin);
        else if (players[1].hp <= 0) EndRound(Result::PlayerWin);
    }
    std::vector<Event> FinishTurn() {
        std::vector<Event> events; const bool wasPlaying = roundResult == Result::Playing; CheckLethal();
        if (Running() && !HasAction(0) && !HasAction(1)) EndRound(players[0].hp == players[1].hp ? Result::Draw
            : players[0].hp > players[1].hp ? Result::PlayerWin : Result::EnemyWin);
        ScoreRound();
        if (wasPlaying && roundResult != Result::Playing) events.emplace_back(Event::RoundEnd, -1, -1, static_cast<int>(roundResult));
        if (Running()) ++turn;
        for (int o = 0; o < 2; ++o) if (hornTurn[o] < turn) { hornTurn[o] = -1; hornLane[o] = Lane::None; }
        ClearTurn(); return events;
    }
    bool NextRound(bool deal = true) {
        if (result != Result::Playing || roundResult == Result::Playing || !roundScored || round >= 3) return false;
        for (int o = 0; o < 2; ++o) {
            const auto board = players[o].board; for (int id : board) Move(id, o, Zone::Discard);
            players[o].hp = MaxHP; players[o].shield = 0; players[o].passed = false;
        }
        ++round; turn = 1; roundResult = Result::Playing; roundScored = false;
        hornTurn = {{-1, -1}}; hornLane = {{Lane::None, Lane::None}}; ClearTurn();
        if (deal) { Draw(0); Draw(1); }
        return true;
    }
    bool Invariant() const {
        if (round < 1 || round > 3 || turn < 1) return false;
        std::vector<int> seen(cards.size(), 0);
        for (int owner = 0; owner < 2; ++owner) {
            const Player& p = players[owner];
            if (p.hp < 0 || p.hp > MaxHP || p.shield < 0 || p.crystals < 0 || p.crystals > 2) return false;
            const std::vector<int>* lists[] = {&p.deck, &p.hand, &p.board, &p.discard};
            for (int z = 0; z < 4; ++z) for (int id : *lists[z]) {
                if (!Valid(id) || ++seen[id] != 1) return false;
                const Card& c = cards[id];
                if (c.id != id || c.owner != owner || static_cast<int>(c.zone) != z) return false;
                if (z == 2) {
                    if (c.value <= 0 || !RealLane(c.lane)) return false;
                    if (c.printedLane != Lane::Any && c.lane != c.printedLane) return false;
                } else if (c.lane != Lane::None || c.suppressed) return false;
            }
        }
        return std::all_of(seen.begin(), seen.end(), [](int n) { return n == 1; });
    }
 private:
    bool roundScored = false;
    static bool Owner(int o) { return o == 0 || o == 1; }
    static bool RealLane(Lane l) { return l == Lane::Melee || l == Lane::Ranged || l == Lane::Siege; }
    bool Valid(int id) const { return id >= 0 && id < static_cast<int>(cards.size()); }
    bool OnBoard(int id) const { return Valid(id) && cards[id].zone == Zone::Board && cards[id].value > 0; }
    bool HandCard(int owner, int id) const { return Valid(id) && cards[id].owner == owner && cards[id].zone == Zone::Hand; }
    bool Running() const { return result == Result::Playing && roundResult == Result::Playing; }
    int Pick(int size) { std::uniform_int_distribution<int> pick(0, size - 1); return pick(random); }
    bool LegalPlacement(int owner, int id, Lane lane, int target) const {
        if (!RealLane(lane)) return false;
        const auto lanes = LegalLanes(owner, id); if (lanes.empty()) return false;
        if (std::find(lanes.begin(), lanes.end(), lane) == lanes.end()) return false;
        if (cards[id].type == Type::Decoy) return OnBoard(target) && cards[target].owner == owner && Unit(cards[target].type) && cards[target].lane == lane;
        return true;
    }
    void ClearTurn() {
        actions = std::array<Action, 2>(); committed = {{false, false}}; utilityUsed = {{false, false}};
        played = {{-1, -1}}; targets = {{-1, -1}}; meleeBoost = {{false, false}};
        for (Card& c : cards) c.suppressed = false;
    }
    void EndRound(Result outcome) {
        if (!Running()) return;
        roundResult = outcome;
    }
    void ScoreRound() {
        if (roundResult == Result::Playing || roundScored) return;
        roundScored = true;
        if (roundResult == Result::EnemyWin || roundResult == Result::Draw) --players[0].crystals;
        if (roundResult == Result::PlayerWin || roundResult == Result::Draw) --players[1].crystals;
        if (players[0].crystals == 0 && players[1].crystals == 0) result = Result::Draw;
        else if (players[0].crystals == 0) result = Result::EnemyWin;
        else if (players[1].crystals == 0) result = Result::PlayerWin;
    }
    void ClearWeather(std::vector<Event>& events) {
        std::vector<int> weather;
        for (const Player& p : players) for (int id : p.board) if (Weather(cards[id].type)) weather.push_back(id);
        for (int id : weather) Destroy(id, events);
    }
    void ArmHorn(int owner, Lane lane, std::vector<Event>& events) {
        if (hornTurn[owner] != turn + 1 || hornLane[owner] != Lane::Any) hornLane[owner] = lane;
        hornTurn[owner] = turn + 1; events.emplace_back(Event::Boost, owner, -1, static_cast<int>(lane));
    }
    void ResolveLeaders(std::vector<Event>& events) {
        std::vector<int> destroy; std::array<std::vector<int>, 2> reveals; bool clear = false;
        // Select every target before applying destructive effects.
        for (int owner = 0; owner < 2; ++owner) {
            if (!committed[owner] || actions[owner].kind != Action::UseLeader) continue;
            const Leader leader = players[owner].leader;
            events.emplace_back(Event::LeaderUsed, owner, -1, static_cast<int>(leader));
            if (leader == Leader::FoltestClear) clear = true;
            else if (leader == Leader::Emhyr) {
                reveals[owner] = players[1 - owner].hand; std::shuffle(reveals[owner].begin(), reveals[owner].end(), random);
                if (reveals[owner].size() > 3) reveals[owner].resize(3);
            } else if (leader == Leader::Eredin) meleeBoost[owner] = true;
            else if (leader == Leader::Francesca) {
                int strongest = -1; std::vector<int> tied;
                for (int id : players[1 - owner].board) if (cards[id].lane == Lane::Melee) {
                    const int value = cards[id].value;
                    if (value > strongest) { strongest = value; tied.clear(); }
                    if (value == strongest) tied.push_back(id);
                }
                if (!tied.empty()) destroy.push_back(tied[Pick(static_cast<int>(tied.size()))]);
            } else if (leader == Leader::FoltestSiege) ArmHorn(owner, Lane::Siege, events);
        }
        if (clear) ClearWeather(events);
        for (int id : destroy) Destroy(id, events);
        for (int owner = 0; owner < 2; ++owner) for (int id : reveals[owner]) {
            if (owner == 0 && cards[id].owner == 1) cards[id].known = true;
            events.emplace_back(Event::Reveal, owner, id, 1);
        }
    }
    void ResolveUtility(std::vector<Event>& events) {
        for (int owner = 0; owner < 2; ++owner) {
            if (utilityUsed[owner]) continue;
            utilityUsed[owner] = true;
            const int id = played[owner]; if (!OnBoard(id) || cards[id].suppressed) continue;
            if (cards[id].type == Type::Medic) {
                const auto available = players[owner].discard; if (available.empty()) continue;
                const int revived = available[Pick(static_cast<int>(available.size()))];
                Move(revived, owner, Zone::Hand); cards[revived].value = cards[revived].printed;
                events.emplace_back(Event::Revive, owner, revived, cards[revived].value);
            } else if (cards[id].type == Type::Decoy) {
                const int target = targets[owner];
                if (!OnBoard(target) || cards[target].owner != owner || !Unit(cards[target].type)) continue;
                auto& board = players[owner].board;
                const int position = static_cast<int>(std::find(board.begin(), board.end(), target) - board.begin());
                Move(target, owner, Zone::Hand); cards[target].value = cards[target].printed; Remove(board, id);
                board.insert(board.begin() + std::min(position, static_cast<int>(board.size())), id);
                events.emplace_back(Event::Return, owner, target, cards[target].value);
            }
        }
    }
    void ResolveWeather(std::vector<Event>& events) {
        std::array<std::array<int, 3>, 2> reductions = {{{{0, 0, 0}}, {{0, 0, 0}}}};
        std::vector<int> board;
        for (const Player& p : players) for (int id : p.board) {
            board.push_back(id);
            if (Weather(cards[id].type) && !cards[id].suppressed) ++reductions[1 - cards[id].owner][static_cast<int>(WeatherLane(cards[id].type))];
        }
        for (int id : board) {
            const int loss = reductions[cards[id].owner][static_cast<int>(cards[id].lane)] + (Weather(cards[id].type) ? 1 : 0);
            cards[id].value -= loss; if (loss) events.emplace_back(Event::Value, cards[id].owner, id, -loss);
            if (cards[id].value <= 0) Destroy(id, events);
        }
    }
};
}  // namespace tema1
