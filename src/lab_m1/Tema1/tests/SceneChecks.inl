// Opt-in end-to-end checks. Fixtures exercise real input handlers and the GPU batch.
#include <fstream>
#include <iostream>
#include <stdexcept>

void Tema1::VerifyScene(const char* directory) {
    auto expect = [](bool valid, const char* message) {
        if (!valid) throw std::runtime_error(std::string("Tema1 scene check: ") + message);
    };
    auto pixel = [this](glm::vec2 logical) {
        int width, height; glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
        return glm::ivec2(std::lround((viewportX + logical.x * viewportW / W) * width / framebufferW),
            std::lround((framebufferH - viewportY - logical.y * viewportH / H) * height / framebufferH));
    };
    auto capture = [this, directory](const char* name) {
        std::vector<unsigned char> pixels(static_cast<size_t>(framebufferW) * framebufferH * 3);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, framebufferW, framebufferH, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        std::ofstream file(std::string(directory) + "/" + name + ".ppm", std::ios::binary);
        file << "P6\n" << framebufferW << " " << framebufferH << "\n255\n";
        for (int y = framebufferH - 1; y >= 0; --y)
            file.write(reinterpret_cast<const char*>(pixels.data() + size_t(y) * framebufferW * 3), framebufferW * 3);
        if (!file) throw std::runtime_error("Cannot save scene verification capture");
    };
    auto click = [this, &pixel](Rect r) {
        const auto p = pixel(glm::vec2(r.x + r.w / 2, r.y + r.h / 2));
        OnMouseBtnPress(p.x, p.y, 1, 0);
    };
    auto play = [this, &pixel, &expect, &capture](int id, glm::vec2 destination) {
        const Rect r = visuals.at(id).at;
        const auto from = pixel(glm::vec2(r.x + r.w / 2, r.y + r.h / 2));
        const auto to = pixel(destination);
        OnMouseBtnPress(from.x, from.y, 1, 0);
        expect(dragged == id, "hand rectangle selects expected card");
        OnMouseMove(to.x, to.y, 0, 0);
        expect(glm::length(glm::vec2(visuals[id].at.x, visuals[id].at.y) + grab - mouse) < 1,
               "drag preserves grab offset");
        if (verifyStage == 4 || verifyStage == 9) {
            Render();
            capture(verifyStage == 4 ? "04b-lane-highlight" : "09b-decoy-target-highlight");
        }
        OnMouseBtnRelease(to.x, to.y, 1, 0);
    };
    auto center = [](Rect r) { return glm::vec2(r.x + r.w / 2, r.y + r.h / 2); };
    auto ready = [this]() { return state == State::Ready && clock > .35f; };
    auto settleFixture = [this]() {
        visuals.clear(); history.clear(); revealed.clear(); inspection = help = false;
        enemyCard = dragged = hovered = -1; effectIndex = 0;
        hitFlash = {{0, 0}}; Layout(); Animate(1); SetState(State::Ready, 0);
    };
    auto winningPlay = [this, &settleFixture, &play, &center]() {
        game.players[0].passed = false;
        game.players[1].passed = true;
        game.Add(0, Type::Damage, 30, Zone::Board, Lane::Melee);
        const int id = game.Add(0, Type::Damage, 1, Zone::Hand, Lane::Siege);
        settleFixture();
        play(id, center(LaneRect(0, Lane::Siege)));
    };
    expect(game.Invariant(), "ownership and lane invariant after scene update");
    expect(time < 180, "scene verification watchdog");

    if (verifyStage == 0 && state == State::SelectLeader) {
        capture("01-leader-menu");
        click(LeaderChoiceRect(1));
        expect(selectedLeader == 1 && state == State::SelectLeader, "first click selects without starting");
        ++verifyStage;
    } else if (verifyStage == 1) {
        capture("02-leader-highlight");
        click(LeaderChoiceRect(1));
        expect(state == State::Deal && game.players[0].leader == Leader::Emhyr, "second click starts selected leader");
        // Leader randomness is covered in the rule suite; this turn checks a card response.
        game.players[1].leaderUsed = true;
        ++verifyStage;
    } else if (verifyStage == 2 && ready()) {
        expect(game.players[0].hand.size() == 10 && game.players[1].hand.size() == 10, "opening hand sizes");
        // Choose the response seed only after the enemy has received its opening hand.
        bool found = false;
        for (unsigned seed = 0; seed < 100; ++seed) {
            Game probe = game; probe.random.seed(seed);
            if (probe.ChooseAction(1).kind == Action::PlayCard) {
                game.random.seed(seed); found = true; break;
            }
        }
        expect(found, "seeded opening enemy has a legal card response");
        for (glm::vec2 logical : {glm::vec2(0), glm::vec2(820, 325), glm::vec2(1600, 900)}) {
            const auto p = pixel(logical);
            expect(glm::length(Pointer(p.x, p.y) - logical) < 3, "Retina coordinate round trip");
        }
        for (int id : game.players[0].hand) if (Unit(game.cards[id].type)) { verifyCard = id; break; }
        expect(verifyCard >= 0, "opening fixture has a unit");
        verifyHand = static_cast<int>(game.players[0].hand.size());
        capture("03-opening");
        play(verifyCard, glm::vec2(1480, 450));
        expect(state == State::Return && game.cards[verifyCard].zone == Zone::Hand, "outside drop returns to hand");
        ++verifyStage;
    } else if (verifyStage == 3 && ready()) {
        expect(game.turn == 1 && game.players[0].hand.size() == static_cast<size_t>(verifyHand), "invalid drop spends no action");
        const Lane wrong = static_cast<Lane>((static_cast<int>(game.cards[verifyCard].printedLane) + 1) % 3);
        play(verifyCard, center(LaneRect(0, wrong)));
        expect(state == State::Return && game.cards[verifyCard].zone == Zone::Hand, "wrong lane returns to hand");
        ++verifyStage;
    } else if (verifyStage == 4 && ready()) {
        capture("04-invalid-return");
        const Lane lane = game.cards[verifyCard].printedLane;
        // The 4px lane gap is invalid, as are the opposing half and letterbox margins.
        expect(!game.CanAct(0, DropAction(verifyCard, glm::vec2(820, 365))), "lane gap rejected");
        expect(!game.CanAct(0, DropAction(verifyCard, center(LaneRect(1, lane)))), "enemy lane rejected");
        expect(!game.CanAct(0, DropAction(verifyCard, glm::vec2(-10, -10))), "letterbox point rejected");
        play(verifyCard, center(LaneRect(0, lane)));
        expect(state == State::PlayerPlace && game.cards[verifyCard].lane == lane, "valid lane placement commits once");
        ++verifyStage;
    } else if (verifyStage == 5 && ready() && game.turn == 2) {
        expect(verifyHand == -1, "enemy rotating reveal observed before resolution");
        capture("05-after-turn");
        OnKeyPress(GLFW_KEY_H, 0); expect(help, "rules overlay opens");
        ++verifyStage;
    } else if (verifyStage == 6) {
        capture("06-rules"); OnKeyPress(GLFW_KEY_H, 0);
        glfwSetWindowSize(glfwGetCurrentContext(), 1000, 800);
        ++verifyStage;
    } else if (verifyStage == 7) {
        for (int row = 0; row < 3; ++row) {
            const glm::vec2 logical = center(LaneRect(0, static_cast<Lane>(row)));
            const auto p = pixel(logical);
            expect(glm::length(Pointer(p.x, p.y) - logical) < 3, "resized lane pointer mapping");
        }
        capture("07-resized");
        game.Empty(123);
        for (int owner = 0; owner < 2; ++owner) for (int n = 0; n < 24; ++n)
            game.Add(owner, static_cast<Type>(n % 3), n % 3 + 1, Zone::Board, Lane::Melee);
        game.Add(0, Type::Clear, 1, Zone::Hand);
        settleFixture(); ++verifyStage;
    } else if (verifyStage == 8) {
        capture("08-crowded-lane");
        for (int owner = 0; owner < 2; ++owner) {
            const Rect r = LaneRect(owner, Lane::Melee);
            for (int id : game.players[owner].board) {
                const Rect c = visuals[id].at;
                expect(c.x >= r.x + 78 && c.x + c.w <= r.x + r.w && c.y >= r.y && c.y + c.h <= r.y + r.h,
                       "crowded lane cards stay inside lane");
            }
        }
        glfwSetWindowSize(glfwGetCurrentContext(), 1280, 720);
        game.Empty(321);
        game.players[0].leader = Leader::Emhyr; game.players[0].leaderUsed = false;
        game.players[1].leader = Leader::Francesca; game.players[1].passed = true;
        verifyCard = game.Add(0, Type::Damage, 3, Zone::Board, Lane::Melee);
        game.Add(0, Type::Shield, 2, Zone::Board, Lane::Siege);
        game.Add(0, Type::Medic, 2, Zone::Board, Lane::Ranged);
        game.Add(1, Type::Damage, 1, Zone::Board, Lane::Melee);
        game.Add(1, Type::Heal, 1, Zone::Board, Lane::Ranged);
        for (Type type : {Type::Decoy, Type::Medic, Type::Frost, Type::Fog, Type::Rain, Type::Horn,
                          Type::Scorch, Type::Clear, Type::Spy})
            game.Add(0, type, type == Type::Clear || type == Type::Decoy ? 1 : 2, Zone::Hand);
        for (int i = 0; i < 3; ++i) game.Add(1, static_cast<Type>(i), 2, Zone::Hand, static_cast<Lane>(i));
        settleFixture(); ++verifyStage;
    } else if (verifyStage == 9 && ready()) {
        capture("09-advanced-cards");
        hovered = verifyCard; frameDirty = true; Render(); capture("09c-unit-hover-preview");
        hovered = -1; frameDirty = true;
        int decoy = -1;
        for (int id : game.players[0].hand) if (game.cards[id].type == Type::Decoy) decoy = id;
        expect(decoy >= 0, "Decoy fixture available");
        play(decoy, center(visuals[verifyCard].at));
        expect(game.cards[verifyCard].zone == Zone::Board, "Decoy waits until utility after Scorch");
        ++verifyStage;
    } else if (verifyStage == 10 && ready()) {
        expect(game.cards[verifyCard].zone == Zone::Hand && game.cards[verifyCard].value == 3, "Decoy returns unit at printed V");
        capture("10-decoy-return");
        verifyHand = game.Add(0, Type::Rain, 2, Zone::Discard);
        game.cards[verifyHand].value = 0;
        int medic = -1;
        for (int id : game.players[0].hand) if (game.cards[id].type == Type::Medic) medic = id;
        expect(medic >= 0, "Medic fixture available");
        play(medic, center(LaneRect(0, Lane::Ranged)));
        ++verifyStage;
    } else if (verifyStage == 11 && ready()) {
        expect(game.cards[verifyHand].zone == Zone::Hand && game.cards[verifyHand].value == 2, "Medic revives a special at printed V");
        capture("11-medic-recovery");
        click(LeaderRect(0));
        expect(game.players[0].leaderUsed && state == State::PlayerPlace, "leader click spends one action");
        ++verifyStage;
    } else if (verifyStage == 12 && inspection) {
        expect(revealed.size() == 3, "Emhyr reveals three distinct enemy cards");
        capture("12-emhyr-inspection");
        const int before = game.turn;
        OnKeyPress(GLFW_KEY_P, 0);
        expect(inspection && game.turn == before, "inspection blocks other actions");
        click(Rect(690, 225, 220, 48)); expect(!inspection, "inspection Continue closes modal");
        ++verifyStage;
    } else if (verifyStage == 13 && ready()) {
        capture("13-used-leader-known-hand");
        int known = 0;
        for (int id : game.players[1].hand) if (game.cards[id].known) ++known;
        expect(known == 3, "revealed cards remain face up in hand");
        click(LeaderRect(0));
        expect(state == State::Ready && !game.committed[0], "used leader cannot spend another action");
        game.Add(0, Type::Heal, 2, Zone::Deck, Lane::Siege);
        game.Add(1, Type::Shield, 2, Zone::Deck, Lane::Siege);
        verifyHand = static_cast<int>(game.players[0].hand.size());
        verifyCard = static_cast<int>(game.players[1].hand.size());
        click(PassRect()); expect(game.players[0].passed, "Pass button locks round");
        ++verifyStage;
    } else if (verifyStage == 14 && state == State::RoundOver) {
        capture("14-round-intermission");
        click(Rect(650, 316, 300, 55));
        expect(state == State::Deal && game.round == 2, "Next round begins one-card deal");
        ++verifyStage;
    } else if (verifyStage == 15 && ready()) {
        expect(game.players[0].hand.size() == static_cast<size_t>(verifyHand + 1)
            && game.players[1].hand.size() == static_cast<size_t>(verifyCard + 1), "exactly one card each in round 2");
        expect(game.players[0].leaderUsed && game.players[0].board.empty() && game.players[0].hp == 30
            && game.players[0].shield == 0, "round resets battlefield/health but preserves leader use");
        capture("15-round-two");
        // Exercise the complete three-round controller deterministically.
        game.players[0].crystals = 1; game.players[1].crystals = 2;
        game.Add(0, Type::Damage, 2, Zone::Deck, Lane::Melee);
        game.Add(1, Type::Heal, 2, Zone::Deck, Lane::Ranged);
        winningPlay(); ++verifyStage;
    } else if (verifyStage == 16 && state == State::RoundOver) {
        expect(game.players[0].crystals == 1 && game.players[1].crystals == 1, "round 2 result scores one crystal");
        capture("16-round-two-result");
        verifyHand = static_cast<int>(game.players[0].hand.size());
        verifyCard = static_cast<int>(game.players[1].hand.size());
        click(Rect(650, 316, 300, 55)); ++verifyStage;
    } else if (verifyStage == 17 && ready()) {
        expect(game.round == 3 && game.players[0].hand.size() == static_cast<size_t>(verifyHand + 1)
            && game.players[1].hand.size() == static_cast<size_t>(verifyCard + 1), "exactly one card each in round 3");
        capture("17-round-three"); winningPlay(); ++verifyStage;
    } else if (verifyStage == 18 && state == State::Over) {
        expect(game.result == Result::PlayerWin && game.players[1].crystals == 0, "last crystal ends match");
        capture("18-match-result"); OnKeyPress(GLFW_KEY_R, 0);
        expect(state == State::SelectLeader && game.players[0].hand.empty(), "Play again returns to leader menu");
        ++verifyStage;
    } else if (verifyStage == 19) {
        capture("19-restarted-menu");
        click(LeaderChoiceRect(0)); click(LeaderChoiceRect(0)); ++verifyStage;
    } else if (verifyStage == 20 && ready()) {
        expect(game.round == 1 && game.turn == 1 && game.players[0].hand.size() == 10
            && game.players[1].hand.size() == 10 && game.players[0].discard.empty()
            && !game.players[0].leaderUsed && game.players[0].crystals == 2, "fresh match resets all match state");
        capture("20-restarted-opening");
        std::cout << "PASS: advanced scene checks (leader selection/highlight, lanes, invalid/valid drag, crowded layout, "
                     "Medic/Decoy, leader inspection/use, help, Retina/resize, three rounds, crystals, restart)\n" << std::flush;
        ++verifyStage; window->Close();
    }
    if (verifyStage == 5 && state == State::Reveal && verifyHand != -1) {
        expect(enemyCard >= 0 && game.cards[enemyCard].zone == Zone::Board && game.turn == 1,
               "enemy card is committed and rotates before the resolution cycle");
        clock = .20f; frameDirty = true; Render();
        capture("04c-enemy-reveal"); verifyHand = -1;
    }
    // Keep actual animation time, but avoid waiting through already-settled phase gaps.
    if (state != State::SelectLeader && state != State::Ready && state != State::Over && state != State::RoundOver
        && state != State::Reveal && !help && !inspection) {
        bool moving = false;
        for (const auto& entry : visuals)
            if (!entry.second.dying && entry.second.elapsed < entry.second.duration) moving = true;
        if (!moving) clock = wait;
    }
}
