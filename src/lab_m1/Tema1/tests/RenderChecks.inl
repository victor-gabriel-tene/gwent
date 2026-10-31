// Deterministic scenes shared by the before/after measurements. No screenshots
// or GPU readbacks occur during timing. The normal game never enters this method.
void Tema1::PrepareRenderCheck() {
    if (!renderChecks.setup) {
        game.Reset(42);
        visuals.clear(); revealed.clear();
        dragged = hovered = enemyCard = -1;
        help = inspection = false; hitFlash = {{0, 0}};
        selectedLeader = 1;
        SetState(State::Ready, 0);
        message = "YOUR TURN";
        for (int owner = 0; owner < 2; ++owner) {
            for (int n = 0; n < 6; ++n) {
                const int id = game.players[owner].deck.back();
                game.Move(id, owner, Zone::Board);
                const Lane printed = game.cards[id].printedLane;
                game.cards[id].lane = printed == Lane::Any ? static_cast<Lane>(n % 3) : printed;
            }
        }
        game.players[0].hp = 24; game.players[1].hp = 27;
        game.players[0].shield = 16; game.players[1].shield = 12;
        Layout(); Animate(1);
        if (renderChecks.scenario == 0) SetState(State::SelectLeader, 0);
        if (renderChecks.scenario == 3) { StartMatch(); renderChecks.animationCycle = 0; }
        if (renderChecks.scenario == 4) enemyCard = game.players[1].board.front();
        if (renderChecks.scenario == 6) help = true;
        renderChecks.setup = true;
        std::cout << "RENDER CHECK starting " << renderChecks.Name() << '\n' << std::flush;
    }
    if (renderChecks.scenario == 2) {
        state = State::Ready;
        dragged = game.players[0].hand.front();
        Rect& card = visuals[dragged].at;
        grab = glm::vec2(card.w / 2, card.h / 2);
        mouse = glm::vec2(800 + 220 * std::sin(renderChecks.elapsed * 1.3),
                          325 + 80 * std::sin(renderChecks.elapsed * .8));
        card.x = mouse.x - grab.x; card.y = mouse.y - grab.y;
    } else if (renderChecks.scenario == 3) {
        const int cycle = static_cast<int>(renderChecks.elapsed / 8);
        if (cycle != renderChecks.animationCycle) { StartMatch(); renderChecks.animationCycle = cycle; }
    } else if (renderChecks.scenario == 4) {
        state = State::Reveal;
        clock = std::fmod(renderChecks.elapsed, .35);
    } else if (renderChecks.scenario == 5) {
        hovered = game.players[0].hand.front();
    }
}
