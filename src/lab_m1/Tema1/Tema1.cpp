#include "lab_m1/Tema1/Tema1.h"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include "lab_m1/lab3/transform2D.h"

using namespace m1;
using namespace tema1;

namespace {
	const float W = 1600, H = 900, PI = 3.14159265359f;
	const glm::vec3 brass(0.60f, 0.46f, 0.26f), gold(0.88f, 0.70f, 0.37f);
	const glm::vec3 ivory(0.91f, 0.86f, 0.74f), muted(0.60f, 0.55f, 0.44f);
	const glm::vec3 dark(0.075f, 0.056f, 0.043f), blue(0.31f, 0.59f, 0.76f);
	std::string Number(int n) { return std::to_string(n); }
	unsigned MatchSeed() {
		const char* value = std::getenv("TEMA1_SEED");
		return value ? static_cast<unsigned>(std::strtoul(value, nullptr, 10))
			: static_cast<unsigned>(std::time(nullptr)) ^ static_cast<unsigned>(std::rand());
	}
}

Tema1::Tema1() : game(1) {}
Tema1::~Tema1() = default;

void Tema1::Init() {
    // Same fixed, orthographic 2D camera setup as Lab 3.
    auto camera = GetSceneCamera();
    camera->SetOrthographic(0, W, 0, H, .01f, 400);
    camera->SetPosition(glm::vec3(0, 0, 50));
    camera->SetRotation(glm::vec3(0));
    camera->Update();
    GetCameraInput()->SetActive(false);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glfwSetWindowTitle(glfwGetCurrentContext(), "GWENT - The Card Table | EGC Tema 1");
    InitMeshes();
    textRenderer.reset(new tema1::TextRenderer(window->props.selfDir, W, H));
    textRenderer->Load(PATH_JOIN(window->props.selfDir, "assets", "fonts", "Hack-Bold.ttf"), 32);
    Restart();
}

void Tema1::InitMeshes() {
    // Unit shapes are centered at the origin; model matrices set their size/position.
    const std::vector<VertexFormat> rectangle = {
        VertexFormat(glm::vec3(-.5f, -.5f, 0)), VertexFormat(glm::vec3(.5f, -.5f, 0)),
        VertexFormat(glm::vec3(.5f, .5f, 0)), VertexFormat(glm::vec3(-.5f, .5f, 0))
    };
    Mesh* quad = new Mesh("t1_quad");
    quad->InitFromData(rectangle, {0, 1, 2, 0, 2, 3});
    AddMeshToList(quad);

    Mesh* triangle = new Mesh("t1_triangle");
    triangle->InitFromData({VertexFormat(glm::vec3(0, .5f, 0)),
        VertexFormat(glm::vec3(-.5f, -.5f, 0)), VertexFormat(glm::vec3(.5f, -.5f, 0))}, {0, 1, 2});
    AddMeshToList(triangle);

    std::vector<VertexFormat> circle = {VertexFormat(glm::vec3(0))};
    std::vector<unsigned int> indices;
    const unsigned int segments = 48;
    for (unsigned int i = 0; i < segments; ++i) {
        const float angle = 2 * PI * i / segments;
        circle.emplace_back(glm::vec3(.5f * std::cos(angle), .5f * std::sin(angle), 0));
        indices.insert(indices.end(), {0, i + 1, (i + 1) % segments + 1});
    }
    Mesh* disk = new Mesh("t1_circle");
    disk->InitFromData(circle, indices);
    AddMeshToList(disk);
}

void Tema1::Restart() {
    game.Empty(MatchSeed());
    visuals.clear(); history.clear(); revealed.clear();
    selectedLeader = -1; dealIndex = 0; dealCount = 20;
    dragged = hovered = enemyCard = -1; effectIndex = 0;
    hitFlash = {{0, 0}}; help = inspection = false;
    message = "CHOOSE YOUR LEADER";
    detail = "Select a leader, then click the selected leader to begin.";
    SetState(State::SelectLeader, 0);
}

void Tema1::StartMatch() {
    if (selectedLeader < 0 || selectedLeader >= static_cast<int>(Leader::Count)) return;
    game.Reset(MatchSeed(), false);
    game.players[0].leader = static_cast<Leader>(selectedLeader);
    visuals.clear(); history.clear(); revealed.clear();
    dealIndex = 0; dealCount = 20; enemyCard = dragged = hovered = -1;
    effectIndex = 0; hitFlash = {{0, 0}}; help = inspection = false;
    message = "ROUND 1 - THE CARDS ARE DEALT";
    detail = "Ten cards each. Win the match by taking both enemy crystals.";
    SetState(State::Deal, .18f);
}

void Tema1::BeginNextRound() {
    if (!game.NextRound(false)) return;
    // Preserve continuing hand visuals and stable IDs; discard visuals belong to the old round.
    for (auto it = visuals.begin(); it != visuals.end();) {
        if (game.cards[it->first].zone != Zone::Hand) it = visuals.erase(it);
        else ++it;
    }
    dragged = hovered = enemyCard = -1; effectIndex = 0;
    revealed.clear(); inspection = help = false;
    hitFlash = {{0, 0}}; dealIndex = 0; dealCount = 2;
    message = "ROUND " + Number(game.round);
    detail = "Health restored. Each player draws one card; your hand carries over.";
    Layout();
    SetState(State::Deal, .35f);
}

Tema1::Rect Tema1::LaneRect(int owner, Lane lane) const {
    const int row = static_cast<int>(lane);
    if (owner < 0 || owner > 1 || row < 0 || row > 2) return Rect();
    return Rect(278, owner == 0 ? 368.f - row * 82.f : 454.f + row * 82.f, 1084, 78);
}
Tema1::Rect Tema1::LeaderRect(int owner) const { return Rect(48, owner == 0 ? 90 : 700, 170, 100); }
Tema1::Rect Tema1::LeaderChoiceRect(int index) const { return Rect(75 + index * 300.f, 310, 250, 310); }
Tema1::Rect Tema1::PassRect() const { return Rect(1415, 210, 140, 38); }

Action Tema1::DropAction(int id, glm::vec2 point) const {
    Lane lane = Lane::None;
    for (int row = 0; row < 3; ++row)
        if (LaneRect(0, static_cast<Lane>(row)).Contains(point)) lane = static_cast<Lane>(row);
    int target = -1;
    if (id >= 0 && game.cards[id].type == Type::Decoy && lane != Lane::None) {
        const auto& board = game.players[0].board;
        for (auto it = board.rbegin(); it != board.rend(); ++it) {
            const auto v = visuals.find(*it);
            if (game.CanPlay(0, id, lane, *it)
                && v != visuals.end() && v->second.at.Contains(point)) { target = *it; break; }
        }
    }
    return Action(Action::PlayCard, id, lane, target);
}

void Tema1::Viewport() {
    GLFWwindow* handle = glfwGetCurrentContext();
    if (!handle) return;
    glfwGetFramebufferSize(handle, &framebufferW, &framebufferH);
    const float scale = std::min(framebufferW / W, framebufferH / H);
    viewportW = std::max(1, static_cast<int>(W * scale));
    viewportH = std::max(1, static_cast<int>(H * scale));
    viewportX = (framebufferW - viewportW) / 2;
    viewportY = (framebufferH - viewportH) / 2;
    glViewport(viewportX, viewportY, viewportW, viewportH);
}

glm::vec2 Tema1::Pointer(int x, int y) const {
    int width = 0, height = 0;
    GLFWwindow* handle = glfwGetCurrentContext();
    if (!handle) return glm::vec2(-1);
    glfwGetWindowSize(handle, &width, &height);
    if (width <= 0 || height <= 0) return glm::vec2(-1);
    return glm::vec2((x * float(framebufferW) / width - viewportX) * W / viewportW,
        (framebufferH - y * float(framebufferH) / height - viewportY) * H / viewportH);
}

void Tema1::FrameStart() {
    glDisable(GL_SCISSOR_TEST);
    glClearColor(.025f, .021f, .018f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Viewport();
}
void Tema1::SetState(State s, float duration) { state = s; clock = 0; wait = duration; }

void Tema1::Retarget(int id, Rect r, float duration) {
    Visual& v = visuals[id];
    if (!v.initialized) { v.at = r; v.initialized = true; }
    v.dying = false; v.vanish = 0;
    v.from = v.at; v.target = r; v.elapsed = 0; v.duration = duration;
}
void Tema1::Layout() {
    for (int o = 0; o < 2; ++o) {
        const auto& hand = game.players[o].hand;
        if (!hand.empty()) {
            const float scale = std::min(1.f, 1080.f / (hand.size() * 100.f - 10));
            const float width = hand.size() * 100.f * scale - 10 * scale;
            for (size_t i = 0; i < hand.size(); ++i)
                if (hand[i] != dragged)
                    Retarget(hand[i], Rect(820 - width / 2 + i * 100 * scale,
                        o == 0 ? 40 : 752, 90 * scale, 126 * scale));
        }
        for (int row = 0; row < 3; ++row) {
            const Lane lane = static_cast<Lane>(row);
            const Rect bounds = LaneRect(o, lane);
            std::vector<int> ids;
            for (int id : game.players[o].board) if (game.cards[id].lane == lane) ids.push_back(id);
            if (ids.empty()) continue;
            const float scale = std::min(1.f, 976.f / (ids.size() * 56.f - 8));
            const float width = ids.size() * 56.f * scale - 8 * scale;
            const float cardW = 48 * scale, cardH = cardW * 1.4f;
            const float center = 868;
            for (size_t i = 0; i < ids.size(); ++i)
                Retarget(ids[i], Rect(center - width / 2 + i * 56 * scale,
                    bounds.y + (bounds.h - cardH) / 2, cardW, cardH));
        }
    }
}
void Tema1::Animate(float dt) {
    for (auto& entry : visuals) {
        Visual& v = entry.second;
        if (entry.first == dragged) continue;
        if (v.elapsed < v.duration) {
            v.elapsed = std::min(v.duration, v.elapsed + dt);
            float a = v.elapsed / std::max(.001f, v.duration);
            a = a * a * (3 - 2 * a);
            v.at = Rect(v.from.x + (v.target.x - v.from.x) * a,
                        v.from.y + (v.target.y - v.from.y) * a,
                        v.from.w + (v.target.w - v.from.w) * a,
                        v.from.h + (v.target.h - v.from.h) * a);
        }
        if (v.dying && v.vanish < 1) {
            v.vanish = std::min(1.f, v.vanish + dt / .28f);
        }
    }
    for (float& flash : hitFlash) if (flash > 0) {
        flash = std::max(0.f, flash - dt);
    }
}
void Tema1::Update(float dt) {
    dt = std::max(0.f, dt); time += dt;
    const glm::ivec2 cursor = window->GetCursorPosition();
    mouse = Pointer(cursor.x, cursor.y);
    if (dragged >= 0 && (!glfwGetWindowAttrib(glfwGetCurrentContext(), GLFW_FOCUSED)
                        || !window->MouseHold(GLFW_MOUSE_BUTTON_LEFT))) CancelDrag();
    Animate(dt);
    if (!help && !inspection) {
        clock += dt;
        if (state != State::Ready && state != State::Over && state != State::RoundOver
            && state != State::SelectLeader && clock >= wait) Advance();
    }
    hovered = -1;
    if (!help && !inspection && state == State::Ready && dragged < 0) {
        for (auto it = game.players[0].hand.rbegin(); it != game.players[0].hand.rend(); ++it)
            if (visuals[*it].at.Contains(mouse)) { hovered = *it; break; }
        if (hovered < 0) for (int o = 0; o < 2; ++o) {
            for (int id : game.players[o].board)
                if (visuals[id].at.Contains(mouse)) hovered = id;
            if (o == 1) for (int id : game.players[o].hand)
                if (game.cards[id].known && visuals[id].at.Contains(mouse)) hovered = id;
        }
    }
    Render();
    if (const char* directory = std::getenv("TEMA1_VERIFY_DIR")) VerifyScene(directory);
}

void Tema1::Advance() {
    if (state == State::Deal) {
        if (dealIndex < dealCount) {
            const int owner = dealIndex % 2, id = game.Draw(owner);
            if (id >= 0) {
                Visual& v = visuals[id];
                v.initialized = true; v.at = Rect(1435, owner == 0 ? 275 : 530, 90, 126);
                Layout(); Retarget(id, visuals[id].target, .30f);
            }
            ++dealIndex;
            SetState(State::Deal, dealIndex == dealCount ? .40f : .14f);
        } else NextTurn();
    } else if (state == State::Return) {
        NextTurn();
    } else if (state == State::PlayerPlace) {
        EnemyPlay();
    } else if (state == State::EnemyPlace) {
        if (enemyCard >= 0) SetState(State::Reveal, .35f);
        else { effectIndex = 0; SetState(State::Resolve, .15f); }
    } else if (state == State::Reveal) {
        effectIndex = 0; SetState(State::Resolve, .20f);
    } else if (state == State::Resolve) {
        const auto& phases = Phases();
        if (effectIndex < static_cast<int>(phases.size()) && game.roundResult == Result::Playing) {
            const Effect phase = phases[effectIndex++];
            message = std::string("RESOLVING ") + PhaseName(phase);
            const auto events = game.Resolve(phase);
            HandleEvents(events); Layout();
            SetState(State::Resolve, events.empty() ? .08f : .60f);
        } else {
            HandleEvents(game.FinishTurn()); enemyCard = -1; NextTurn();
        }
    }
}
void Tema1::SubmitAction(const Action& action) {
    if (!game.Commit(0, action)) return;
    hovered = -1;
    Layout();
    if (action.kind == Action::PlayCard) message = "YOU PLAY " + std::string(Name(game.cards[action.card].type));
    else if (action.kind == Action::UseLeader) message = "YOU CALL YOUR LEADER";
    else message = "YOU PASS THE ROUND";
    detail = "The opponent is choosing a response.";
    SetState(State::PlayerPlace, .30f);
}
void Tema1::EnemyPlay() {
    const Action action = game.ChooseAction(1);
    game.Commit(1, action);
    enemyCard = action.kind == Action::PlayCard ? action.card : -1;
    Layout();
    if (!game.AnyCommitted()) {
        HandleEvents(game.FinishTurn()); enemyCard = -1; NextTurn(); return;
    }
    message = enemyCard >= 0 ? "OPPONENT PLAYS" : action.kind == Action::UseLeader
        ? "OPPONENT CALLS THEIR LEADER" : "OPPONENT PASSES";
    detail = enemyCard >= 0 ? "A hidden card enters the battlefield." : "Surviving cards remain active.";
    SetState(State::EnemyPlace, .34f);
}
void Tema1::NextTurn() {
    if (game.result != Result::Playing) {
        message = game.result == Result::PlayerWin ? "VICTORY" : game.result == Result::EnemyWin ? "DEFEAT" : "A DRAW";
        detail = "The match is over. Choose a leader to play again.";
        SetState(State::Over, 0);
    } else if (game.roundResult != Result::Playing) {
        message = game.roundResult == Result::PlayerWin ? "ROUND WON" : game.roundResult == Result::EnemyWin
            ? "ROUND LOST" : "ROUND DRAWN";
        detail = game.roundResult == Result::Draw ? "Both sides lose one crystal." : "The losing side gives up one crystal.";
        SetState(State::RoundOver, 0);
    } else if (game.players[0].passed || !game.HasAction(0)) {
        game.Commit(0, Action(Action::Skip));
        message = game.players[0].passed ? "YOU HAVE PASSED" : "NO AVAILABLE ACTION";
        detail = "Your surviving board cards remain active.";
        SetState(State::PlayerPlace, .40f);
    } else {
        message = "YOUR TURN";
        detail = "Play a card on its lane, call your leader, or pass the round.";
        SetState(State::Ready, 0);
    }
}
void Tema1::HandleEvents(const std::vector<Event>& events) {
    int transfers = 0, losses = 0;
    std::vector<int> inspectionCards;
    for (const Event& event : events) {
        if (event.kind == Event::Destroy) {
            Visual& v = visuals[event.card];
            v.dying = true; v.vanish = 0; ++losses;
        } else if (event.kind == Event::Transfer) {
            ++transfers;
        } else if (event.kind == Event::Return || event.kind == Event::Revive) {
            Visual& v = visuals[event.card];
            if (!v.initialized) {
                v.initialized = true;
                v.at = Rect(1435, event.owner == 0 ? 275 : 530, 90, 126);
            }
            v.dying = false; v.vanish = 0;
            history.insert(history.begin(), event.kind == Event::Return ? "A CARD RETURNS TO HAND" : "MEDIC RECOVERS A CARD");
        } else if (event.kind == Event::Reveal && event.owner == 0) {
            inspectionCards.push_back(event.card);
        } else if (event.kind == Event::LeaderUsed) {
            history.insert(history.begin(), event.owner == 0 ? "YOUR LEADER ABILITY" : "ENEMY LEADER ABILITY");
        } else if (event.kind == Event::Suppress) {
            history.insert(history.begin(), "SCORCH SUPPRESSES A CARD");
        } else if (event.kind == Event::Boost) {
            history.insert(history.begin(), "A HORN BONUS IS ARMED");
        } else if (event.kind == Event::Hit || event.kind == Event::Restore || event.kind == Event::Guard) {
            const std::string who = event.owner == 0 ? "YOU" : "OPPONENT";
            std::string line;
            if (event.kind == Event::Hit) {
                line = who + (event.amount ? " -" + Number(event.amount) + " HP" : " BLOCKED THE ATTACK");
                if (event.amount) hitFlash[event.owner] = .6f;
            } else line = who + " +" + Number(event.amount) + (event.kind == Event::Guard ? " SHIELD" : " HP");
            history.insert(history.begin(), line);
        }
    }
    if (!inspectionCards.empty()) { revealed = inspectionCards; inspection = true; }
    if (transfers) history.insert(history.begin(), Number(transfers) + " CARD(S) STOLEN");
    if (losses) history.insert(history.begin(), Number(losses) + " CARD(S) REMOVED");
    if (history.size() > 4) history.resize(4);
    detail = "Surviving board cards activate in effect order.";
}

void Tema1::OnMouseMove(int x, int y, int, int) {
    mouse = Pointer(x, y);
    if (dragged >= 0) {
        visuals[dragged].at.x = mouse.x - grab.x;
        visuals[dragged].at.y = mouse.y - grab.y;
    }
}
void Tema1::OnMouseBtnPress(int x, int y, int buttons, int) {
    if (!IS_BIT_SET(buttons, GLFW_MOUSE_BUTTON_LEFT)) return;
    mouse = Pointer(x, y);
    if (help) { help = false; return; }
    if (inspection) {
        if (Rect(690, 225, 220, 48).Contains(mouse)) inspection = false;
        return;
    }
    if (state == State::SelectLeader) {
        for (int i = 0; i < static_cast<int>(Leader::Count); ++i) if (LeaderChoiceRect(i).Contains(mouse)) {
            if (selectedLeader == i) StartMatch(); else selectedLeader = i;
            return;
        }
        return;
    }
    if (state == State::Over && Rect(650, 316, 300, 55).Contains(mouse)) { Restart(); return; }
    if (state == State::RoundOver && Rect(650, 316, 300, 55).Contains(mouse)) { BeginNextRound(); return; }
    if (Rect(1420, 28, 130, 30).Contains(mouse)) { CancelDrag(); help = true; return; }
    if (state != State::Ready || dragged >= 0) return;
    if (LeaderRect(0).Contains(mouse)) { SubmitAction(Action(Action::UseLeader)); return; }
    if (PassRect().Contains(mouse)) { SubmitAction(Action(Action::Pass)); return; }
    for (auto it = game.players[0].hand.rbegin(); it != game.players[0].hand.rend(); ++it)
        if (visuals[*it].at.Contains(mouse)) {
            dragged = *it; grab = mouse - glm::vec2(visuals[*it].at.x, visuals[*it].at.y); return;
        }
}
void Tema1::OnMouseBtnRelease(int x, int y, int buttons, int) {
    if (!IS_BIT_SET(buttons, GLFW_MOUSE_BUTTON_LEFT) || dragged < 0) return;
    mouse = Pointer(x, y);
    const int id = dragged;
    const Action action = DropAction(id, mouse);
    dragged = -1;
    if (game.CanAct(0, action)) SubmitAction(action);
    else {
        Layout(); message = "RETURNING TO YOUR HAND";
        detail = game.cards[id].type == Type::Decoy ? "Drop a Decoy on one of your board cards."
            : "Release on a highlighted lane in your half of the battlefield.";
        SetState(State::Return, .26f);
    }
}
void Tema1::CancelDrag() {
    if (dragged < 0) return;
    dragged = -1; Layout(); SetState(State::Return, .26f);
}
void Tema1::OnKeyPress(int key, int) {
    if (inspection) {
        if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) inspection = false;
        return;
    }
    if (key == GLFW_KEY_H && state != State::SelectLeader) { CancelDrag(); help = !help; return; }
    if (help) return;
    if (key == GLFW_KEY_R && state == State::Over) Restart();
    if (key == GLFW_KEY_ENTER && state == State::RoundOver) BeginNextRound();
    if (key == GLFW_KEY_BACKSPACE && dragged >= 0) CancelDrag();
    if (key == GLFW_KEY_P && state == State::Ready && dragged < 0) SubmitAction(Action(Action::Pass));
}
void Tema1::OnWindowResize(int, int) { Viewport(); }

#include "lab_m1/Tema1/Tema1Presentation.inl"
#include "lab_m1/Tema1/tests/SceneChecks.inl"
