#include "lab_m1/Tema1/Tema1.h"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include "glm/gtc/matrix_transform.hpp"

using namespace m1;
using namespace tema1;

namespace {
const float W = 1600, H = 900, PI = 3.14159265359f;
const glm::vec3 brass(0.60f, 0.46f, 0.26f), gold(0.88f, 0.70f, 0.37f);
const glm::vec3 ivory(0.91f, 0.86f, 0.74f), muted(0.60f, 0.55f, 0.44f);
const glm::vec3 dark(0.075f, 0.056f, 0.043f), blue(0.31f, 0.59f, 0.76f);
glm::mat3 T(float x, float y) { glm::mat3 m(1); m[2] = glm::vec3(x, y, 1); return m; }
glm::mat3 S(float x, float y) { glm::mat3 m(1); m[0][0] = x; m[1][1] = y; return m; }
glm::mat3 R(float a) {
    glm::mat3 m(1); m[0] = glm::vec3(std::cos(a), std::sin(a), 0);
    m[1] = glm::vec3(-std::sin(a), std::cos(a), 0); return m;
}
std::string Number(int n) { return std::to_string(n); }
const char* Descriptions[] = {
    "Deals V damage each turn. Shield absorbs damage first.",
    "Restores V health each turn, up to 30 HP.",
    "Grants V shield each turn. Unspent shield carries over.",
    "Steals one random enemy hand card. Loses 1 V per turn.",
    "Reduces all enemy board values by 1. Loses 1 V per turn.",
    "Removes all active frost on both sides. Loses 1 V per turn."
};
}

Tema1::Tema1() : game(1) {}
Tema1::~Tema1() = default;

void Tema1::Init() {
    if (const char* path = std::getenv("TEMA1_BENCHMARK_FILE")) benchmark.file = path;
    forceRebuild = std::getenv("TEMA1_BENCHMARK_REBUILD") != nullptr;
    GetCameraInput()->SetActive(false);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glfwSetWindowTitle(glfwGetCurrentContext(),"GWENT - The Card Table | EGC Tema 1");
    renderer.reset(new BatchRenderer());
    renderer->Init(PATH_JOIN(window->props.selfDir,"assets","fonts","Hack-Bold.ttf"));
    Restart();
}

void Tema1::Restart() {
    frameDirty = true;
    game.Reset(static_cast<unsigned>(std::time(nullptr)) ^ static_cast<unsigned>(std::rand()), false);
    visuals.clear(); history.clear();
    dealIndex = 0; dragged = hovered = enemyCard = -1; effectIndex = 0;
    hitFlash = {{0, 0}}; help = false;
    message = "THE CARDS ARE DEALT";
    detail = "Ten cards each. Make every choice count.";
    SetState(State::Deal, 0.18f);
}

void Tema1::Viewport() {
    // Query GLFW itself: the framework's resolution getter double-scales Retina sizes.
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
void Tema1::SetState(State s, float duration) { state = s; clock = 0; wait = duration; frameDirty = true; }

void Tema1::Retarget(int id, Rect r, float duration) {
    frameDirty = true;
    Visual& v = visuals[id];
    if (!v.initialized) { v.at = r; v.initialized = true; }
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
        const auto& board = game.players[o].board;
        const int rows = static_cast<int>((board.size() + 9) / 10);
        for (size_t i = 0; i < board.size(); ++i) {
            const int row = static_cast<int>(i / 10);
            const int count = std::min(10, static_cast<int>(board.size()) - row * 10);
            const float width = count * 96.f - 16;
            float y = (o == 0 ? 270.f : 520.f);
            if (rows > 1) y = (o == 0 ? 209.f : 459.f) + row * 122.f;
            Retarget(board[i], Rect(820 - width / 2 + (i % 10) * 96.f, y, 80, 112));
        }
    }
}
void Tema1::Animate(float dt) {
    for (auto& entry : visuals) {
        Visual& v = entry.second;
        if (entry.first == dragged) continue;
        if (v.elapsed < v.duration) {
            frameDirty = true;
            v.elapsed = std::min(v.duration,v.elapsed+dt);
            float a = v.elapsed / std::max(.001f,v.duration);
            a = a*a*(3-2*a);
            v.at = Rect(v.from.x + (v.target.x-v.from.x)*a,
                        v.from.y + (v.target.y-v.from.y)*a,
                        v.from.w + (v.target.w-v.from.w)*a,
                        v.from.h + (v.target.h-v.from.h)*a);
        }
        if (v.dying && v.vanish < 1) {
            frameDirty = true;
            v.vanish = std::min(1.f,v.vanish+dt/.28f);
        }
    }
    for (float& flash : hitFlash) if (flash > 0) {
        frameDirty = true; flash = std::max(0.f,flash-dt);
    }
}
void Tema1::Update(float dt) {
    const float realDt = dt;
    // Keep elapsed wall time: clamping to 0.1 made animations run in slow motion
    // whenever a frame took longer than 100 ms.
    dt = std::max(0.f,dt);
    time += dt;
    const glm::ivec2 cursor = window->GetCursorPosition();
    mouse = Pointer(cursor.x, cursor.y);
    if (dragged >= 0 && (!glfwGetWindowAttrib(glfwGetCurrentContext(), GLFW_FOCUSED)
                        || !window->MouseHold(GLFW_MOUSE_BUTTON_LEFT))) CancelDrag();
    Animate(dt);
    if (!help) {
        clock += dt;
        if (state != State::Ready && state != State::Over && clock >= wait) Advance();
    }
    const int previousHover = hovered;
    hovered = -1;
    if (!help && state == State::Ready && dragged < 0) {
        for (auto it = game.players[0].hand.rbegin(); it != game.players[0].hand.rend(); ++it)
            if (visuals[*it].at.Contains(mouse)) { hovered = *it; break; }
        if (hovered < 0) for (int o = 0; o < 2; ++o)
            for (int id : game.players[o].board)
                if (visuals[id].at.Contains(mouse)) hovered = id;
    }
    if (hovered != previousHover || state == State::Reveal) frameDirty = true;
    const auto renderStart = std::chrono::steady_clock::now();
    Render();
    const double renderMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - renderStart).count();
    if (benchmark.Record(realDt, renderMs, state == State::Ready, framebufferW, framebufferH)) window->Close();
    // Opt-in developer checks; ordinary games never execute the test driver.
    if (const char* directory = std::getenv("TEMA1_VERIFY_DIR")) VerifyScene(directory);
}

void Tema1::Advance() {
    if (state == State::Deal) {
        if (dealIndex < 20) {
            const int owner = dealIndex % 2, id = game.Draw(owner);
            Visual& v = visuals[id];
            v.initialized = true; v.at = Rect(1435, owner == 0 ? 275 : 530, 90, 126);
            Layout();
            // Deck-to-hand draw takes slightly longer than its neighbors' reflow.
            Retarget(id, visuals[id].target, .30f);
            ++dealIndex;
            SetState(State::Deal, dealIndex == 20 ? .40f : .14f);
        } else NextTurn();
    } else if (state == State::Return) {
        NextTurn();
    } else if (state == State::PlayerPlace) {
        EnemyPlay();
    } else if (state == State::EnemyPlace) {
        if (enemyCard >= 0) SetState(State::Reveal, .35f);
        else { effectIndex = 0; SetState(State::Resolve, .15f); }
    } else if (state == State::Reveal) {
        effectIndex = 0;
        SetState(State::Resolve, .20f);
    } else if (state == State::Resolve) {
        if (effectIndex < 6 && game.result == Result::Playing) {
            static const char* phases[] = {"SPIES", "CLEAR WEATHER", "FROST", "SHIELDS", "DAMAGE", "HEALING"};
            message = std::string("RESOLVING ") + phases[effectIndex];
            const auto events = game.Resolve(static_cast<Effect>(effectIndex));
            HandleEvents(events);
            ++effectIndex;
            Layout();
            SetState(State::Resolve, events.empty() ? .17f : .65f);
        } else {
            game.FinishTurn();
            enemyCard = -1;
            NextTurn();
        }
    }
}
void Tema1::EnemyPlay() {
    enemyCard = game.Choose(1);
    if (enemyCard >= 0) game.Play(1, enemyCard);
    Layout();
    message = enemyCard >= 0 ? "OPPONENT PLAYS" : "OPPONENT HAS NO CARDS";
    detail = enemyCard >= 0 ? "A hidden card enters the battlefield." : "Their surviving cards will still act.";
    SetState(State::EnemyPlace, .34f);
}
void Tema1::NextTurn() {
    if (game.result != Result::Playing) {
        message = game.result == Result::PlayerWin ? "VICTORY" : game.result == Result::EnemyWin ? "DEFEAT" : "A DRAW";
        detail = "The table remembers every choice.";
        SetState(State::Over, 0);
    } else if (game.players[0].hand.empty() && game.players[1].hand.empty()) {
        game.FinishTurn(); NextTurn();
    } else if (game.players[0].hand.empty()) {
        message = "YOUR HAND IS EMPTY";
        detail = "Automatic pass. Your board cards remain active.";
        SetState(State::PlayerPlace, .75f);
    } else {
        message = "YOUR TURN";
        detail = "Drag one card onto your half of the battlefield.";
        SetState(State::Ready, 0);
    }
}
void Tema1::HandleEvents(const std::vector<Event>& events) {
    int transfers = 0, losses = 0;
    for (const Event& event : events) {
        if (event.kind == Event::Destroy) {
            Visual& v = visuals[event.card];
            v.dying = true; v.vanish = 0; ++losses;
        } else if (event.kind == Event::Transfer) {
            ++transfers;
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
    if (transfers) history.insert(history.begin(), Number(transfers) + " CARD(S) STOLEN");
    if (losses) history.insert(history.begin(), Number(losses) + " CARD(S) REMOVED");
    if (history.size() > 4) history.resize(4);
    detail = "Surviving board cards activate in effect order.";
}

void Tema1::OnMouseMove(int x, int y, int, int) {
    frameDirty = true;
    mouse = Pointer(x, y);
    if (dragged >= 0) {
        visuals[dragged].at.x = mouse.x - grab.x;
        visuals[dragged].at.y = mouse.y - grab.y;
    }
}
void Tema1::OnMouseBtnPress(int x, int y, int buttons, int) {
    frameDirty = true;
    if (!IS_BIT_SET(buttons, GLFW_MOUSE_BUTTON_LEFT)) return;
    mouse = Pointer(x, y);
    if (help) { help = false; return; }
    if (state == State::Over && Rect(650, 316, 300, 55).Contains(mouse)) { Restart(); return; }
    if (Rect(1420, 28, 130, 30).Contains(mouse)) { help = true; return; }
    if (state != State::Ready || dragged >= 0) return;
    for (auto it = game.players[0].hand.rbegin(); it != game.players[0].hand.rend(); ++it) {
        if (visuals[*it].at.Contains(mouse)) {
            dragged = *it;
            grab = mouse - glm::vec2(visuals[*it].at.x, visuals[*it].at.y);
            return;
        }
    }
}
void Tema1::OnMouseBtnRelease(int x, int y, int buttons, int) {
    if (!IS_BIT_SET(buttons, GLFW_MOUSE_BUTTON_LEFT) || dragged < 0) return;
    mouse = Pointer(x, y);
    const int id = dragged;
    dragged = -1;
    if (Rect(276, 201, 1088, 248).Contains(mouse) && game.Play(0, id)) {
        Layout();
        message = "YOU PLAY " + std::string(Name(game.cards[id].type));
        detail = "The opponent is choosing a response.";
        SetState(State::PlayerPlace, .30f);
    } else {
        Layout();
        message = "RETURNING TO YOUR HAND";
        detail = "Release inside your half of the battlefield.";
        SetState(State::Return, .26f);
    }
}
void Tema1::CancelDrag() {
    if (dragged < 0) return;
    dragged = -1; Layout(); SetState(State::Return, .26f);
}
void Tema1::OnKeyPress(int key, int) {
    frameDirty = true;
    if (key == GLFW_KEY_H) { CancelDrag(); help = !help; }
    if (key == GLFW_KEY_R && state == State::Over) Restart();
    if (key == GLFW_KEY_BACKSPACE && dragged >= 0) CancelDrag();
}
void Tema1::OnWindowResize(int, int) { Viewport(); frameDirty = true; }

void Tema1::Shape(const char* mesh, float x, float y, float w, float h,
                  glm::vec3 color, float angle, float alpha, int pattern) {
    const glm::mat3 transform = group * T(x, y) * R(angle) * S(w, h);
    renderer->Shape(mesh,transform,glm::vec4(color,alpha),pattern);
}
void Tema1::Box(Rect r, glm::vec3 color, float alpha, int pattern) {
    Shape("t1_quad", r.x + r.w / 2, r.y + r.h / 2, r.w, r.h, color, 0, alpha, pattern);
}
void Tema1::Frame(Rect r, glm::vec3 color, float t) {
    Box(Rect(r.x,r.y,r.w,t),color); Box(Rect(r.x,r.y+r.h-t,r.w,t),color);
    Box(Rect(r.x,r.y,t,r.h),color); Box(Rect(r.x+r.w-t,r.y,t,r.h),color);
}
void Tema1::Line(float x1, float y1, float x2, float y2, float width, glm::vec3 color) {
    Shape("t1_quad", (x1+x2)/2, (y1+y2)/2,
        std::sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)), width, color, std::atan2(y2-y1,x2-x1));
}
float Tema1::TextWidth(const std::string& value, float size) const {
    return renderer->TextWidth(value,size);
}
void Tema1::Label(const std::string& value, float x, float top, float size, glm::vec3 color) {
    const float scaleY = glm::length(glm::vec2(group[1]));
    const float scaleX = glm::length(glm::vec2(group[0]));
    if (scaleX < scaleY * .85f) return;  // text is hidden while the card is edge-on
    const glm::vec3 p = group * glm::vec3(x, top, 1);
    renderer->Text(value,p.x,p.y,size*scaleY,color);
}
void Tema1::Center(const std::string& value, float x, float top, float size, glm::vec3 color) {
    Label(value, x - TextWidth(value, size) / 2, top, size, color);
}
glm::vec3 Tema1::TypeColor(Type t) const {
    static const glm::vec3 colors[] = {
        {.57f,.13f,.10f}, {.17f,.39f,.24f}, {.15f,.32f,.51f},
        {.075f,.075f,.085f}, {.80f,.83f,.81f}, {.76f,.60f,.20f}
    };
    return colors[static_cast<int>(t)];
}

void Tema1::Ornament(float x, float y, float width) {
    Line(x-width/2,y,x-12,y,1,brass); Line(x+12,y,x+width/2,y,1,brass);
    Shape("t1_quad",x,y,8,8,gold,PI/4);
    Shape("t1_quad",x-width/2,y,4,4,brass,PI/4);
    Shape("t1_quad",x+width/2,y,4,4,brass,PI/4);
}

void Tema1::BoardStatic() {
    Box(Rect(0,0,W,H),glm::vec3(.13f,.085f,.052f),1,1);
    Frame(Rect(14,14,1572,872),brass,2);
    Frame(Rect(21,21,1558,858),glm::vec3(.29f,.22f,.13f),1);
    Box(Rect(258,26,1124,848),glm::vec3(.065f,.045f,.029f));
    Frame(Rect(259,27,1122,846),brass,2);
    Box(Rect(274,200,1092,500),glm::vec3(.235f,.175f,.112f),1,1);
    Box(Rect(278,204,1084,242),glm::vec3(.285f,.215f,.141f),1,2);
    Box(Rect(278,454,1084,242),glm::vec3(.235f,.176f,.123f),1,2);
    Frame(Rect(274,200,1092,500),brass,2);
    Line(276,450,1364,450,3,brass);
    Ornament(820,450,1060);
    // Faint engraved diamonds, rather than lane divisions or advanced-only slots.
    for (int owner = 0; owner < 2; ++owner) {
        const float cy = owner == 0 ? 325 : 575;
        Shape("t1_quad",820,cy,78,78,brass,PI/4,.10f);
        Shape("t1_quad",820,cy,72,72,glm::vec3(.245f,.185f,.12f),PI/4,.65f);
    }
    Box(Rect(274,34,1092,140),glm::vec3(.10f,.075f,.049f),1,1);
    Box(Rect(274,746,1092,130),glm::vec3(.10f,.075f,.049f),1,1);
    Line(280,182,1360,182,1,brass); Line(280,734,1360,734,1,brass);
    Center("GWENT",132,845,36,gold);
    Center("THE CARD TABLE",132,803,11,muted);
    Ornament(132,780,160);
    Center("DRAG TO PLAY",1485,83,10,muted);
    Label("LEFT MOUSE  PICK UP / PLACE",32,74,9,muted);
    Label("BACKSPACE   CANCEL DRAG",32,54,9,muted);
    Label("ESC         EXIT",32,34,9,muted);
}

void Tema1::Board() {
    if (boardCache.empty()) {
        const size_t mark = renderer->Mark();
        BoardStatic();
        boardCache = renderer->CopyFrom(mark);
    } else renderer->Append(boardCache);
    if (dragged >= 0) {
        const bool inside = Rect(276,201,1088,248).Contains(mouse);
        Box(Rect(278,204,1084,242),gold,inside ? .09f : .035f);
        Frame(Rect(280,206,1080,238),inside ? gold : brass,2);
    }
    Label("TURN " + Number(game.turn),292,724,11,muted);
    Center(detail,865,724,10,muted);
    Center(message,820,196,14,gold);
    Center("H  RULES",1485,49,12,Rect(1420,28,130,30).Contains(mouse) ? gold : ivory);
}

void Tema1::Status(int owner) {
    const Player& p = game.players[owner];
    const std::string key = Number(p.hp)+":"+Number(p.shield)+":"+Number(static_cast<int>(p.hand.size()));
    if (hitFlash[owner] <= 0 && statusKey[owner] == key && !statusCache[owner].empty()) {
        renderer->Append(statusCache[owner]); return;
    }
    const size_t mark = renderer->Mark();
    const float y = owner == 0 ? 222.f : 476.f;
    Box(Rect(30,y,206,211),glm::vec3(.065f,.051f,.039f),1,2);
    Frame(Rect(30,y,206,211),brass,1);
    Center(owner == 0 ? "YOU" : "OPPONENT",133,y+192,19,ivory);
    Line(44,y+164,222,y+164,1,brass);
    Shape("t1_circle",75,y+117,68,68,brass);
    Shape("t1_circle",75,y+117,62,62,glm::vec3(.13f,.105f,.075f));
    if (hitFlash[owner] > 0) Shape("t1_circle",75,y+117,62,62,glm::vec3(.7f,.12f,.08f),0,hitFlash[owner]);
    Center(Number(p.hp),75,y+132,27,ivory);
    Center("HP",75,y+97,10,muted);
    for (int i = 0; i < 30; ++i) {
        const Rect pip(115+(i%10)*10.6f,y+127-(i/10)*14,8,10);
        Box(pip,i < p.hp ? glm::vec3(.70f,.20f,.15f) : glm::vec3(.18f,.14f,.11f));
        Frame(pip,i < p.hp ? glm::vec3(.87f,.37f,.25f) : glm::vec3(.26f,.21f,.16f),.6f);
    }
    Label("SHIELD " + Number(p.shield),44,y+74,10,blue);
    // At most 20 hand cards can be played; even the largest possible shield pool
    // fits in this wrapped geometric display. Scale units rather than cap rules.
    const int columns = 24, rows = std::max(1,(p.shield+columns-1)/columns);
    const float pipH = std::min(5.f,38.f/rows);
    for (int i = 0; i < p.shield; ++i)
        Box(Rect(44+(i%columns)*7.5f,y+54-(i/columns)*(pipH+1),5.5f,pipH),blue);
    const int n = static_cast<int>(p.hand.size());
    Center(Number(n) + (n == 1 ? " CARD IN HAND" : " CARDS IN HAND"),133,y+20,10,muted);
    if (hitFlash[owner] <= 0) {
        statusKey[owner] = key;
        statusCache[owner] = renderer->CopyFrom(mark);
    }
}

void Tema1::Deck(int owner) {
    const float y = owner == 0 ? 275.f : 530.f;
    const auto& p = game.players[owner];
    std::string key = Number(static_cast<int>(p.deck.size()))+":"+Number(static_cast<int>(p.discard.size()));
    if (owner == 0) for (const auto& line : history) key += ":"+line;
    if (deckKey[owner] == key && !deckCache[owner].empty()) { renderer->Append(deckCache[owner]); return; }
    const size_t mark = renderer->Mark();
    Center(owner == 0 ? "YOUR DECK" : "ENEMY DECK",1480,y+160,12,ivory);
    for (int i = 3; i >= 0; --i) {
        const Rect r(1435+i*4.f,y+i*4.f,90,126);
        Box(r,glm::vec3(.13f,.105f,.075f),1,1); Frame(r,brass,1.5f);
    }
    Shape("t1_quad",1480,y+63,38,38,brass,PI/4);
    Shape("t1_quad",1480,y+63,30,30,glm::vec3(.16f,.12f,.08f),PI/4);
    Shape("t1_quad",1480,y+63,12,12,gold,PI/4);
    Center(Number(static_cast<int>(p.deck.size())) + " REMAINING",1480,y-14,11,gold);
    Center(Number(static_cast<int>(p.discard.size())) + " DISCARDED",1480,y-36,10,muted);
    if (owner == 0) {
        Ornament(1480,456,150);
        for (size_t i = 0; i < history.size(); ++i)
            Center(history[i],1480,182-i*20.f,8,muted);
    }
    deckKey[owner] = key;
    deckCache[owner] = renderer->CopyFrom(mark);
}

void Tema1::Symbol(Type type, float x, float y, float size, glm::vec3 color) {
    if (type == Type::Damage) Shape("t1_triangle",x,y,size,size,color);
    else if (type == Type::Heal) Shape("t1_quad",x,y,size*.70f,size*.70f,color,PI/4);
    else if (type == Type::Shield) {
        Shape("t1_circle",x,y,size,size,color);
        Shape("t1_circle",x,y+1,size*.65f,size*.65f,TypeColor(type));
        Shape("t1_quad",x,y,size*.12f,size*.48f,color);
    } else if (type == Type::Spy) {
        Shape("t1_quad",x-size*.18f,y,size*.48f,size*.48f,color,PI/4);
        Shape("t1_quad",x+size*.18f,y,size*.48f,size*.48f,color,PI/4);
        Shape("t1_circle",x,y,size*.26f,size*.26f,TypeColor(type));
    } else if (type == Type::Frost) {
        for (int i = 0; i < 3; ++i) {
            const float a = i*PI/3;
            const glm::vec2 d(std::cos(a),std::sin(a));
            Line(x-d.x*size/2,y-d.y*size/2,x+d.x*size/2,y+d.y*size/2,size*.065f,color);
        }
        Shape("t1_quad",x,y,size*.26f,size*.26f,color,PI/4);
    } else {
        Shape("t1_circle",x,y,size*.55f,size*.55f,color);
        for (int i = 0; i < 8; ++i) {
            const float a = i*PI/4;
            Shape("t1_triangle",x+std::cos(a)*size*.42f,y+std::sin(a)*size*.42f,
                  size*.14f,size*.20f,color,a-PI/2);
        }
    }
}

void Tema1::Card(int id, Rect r, bool face, bool selected, float flip) {
    const auto saved = group;
    const tema1::Card& c = game.cards[id];
    group = group * T(r.x+r.w/2,r.y+r.h/2) * S(r.w/90*flip,r.h/126) * T(-45,-63);
    Box(Rect(4,-5,90,126),glm::vec3(0),.38f);
    if (selected) {
        Box(Rect(-4,-4,98,134),gold,.20f);
        Frame(Rect(-3,-3,96,132),gold,2);
    }
    const auto key = std::make_pair(face ? static_cast<int>(c.type) : -1,face ? c.value : 0);
    auto cached = cardCache.find(key);
    if (cached == cardCache.end()) {
        const glm::mat3 cardTransform = group;
        group = glm::mat3(1);
        const size_t mark = renderer->Mark();
        CardBody(c,face);
        auto list = renderer->ExtractFrom(mark);
        cached = cardCache.emplace(key,std::move(list)).first;
        group = cardTransform;
    }
    renderer->Append(cached->second,group);
    group = saved;
}

void Tema1::CardBody(const tema1::Card& c, bool face) {
    Box(Rect(0,0,90,126),brass);
    Box(Rect(2,2,86,122),dark);
    Box(Rect(5,5,80,116),face ? TypeColor(c.type) : glm::vec3(.17f,.125f,.08f),1,face ? 2 : 1);
    Frame(Rect(7,7,76,112),face ? ivory*.65f : brass, .8f);
    if (!face) {
        Shape("t1_quad",45,63,44,44,brass,PI/4);
        Shape("t1_quad",45,63,37,37,dark,PI/4);
        Shape("t1_quad",45,63,23,23,brass,PI/4);
        Shape("t1_quad",45,63,16,16,glm::vec3(.17f,.125f,.08f),PI/4);
        Shape("t1_quad",45,63,5,5,gold,PI/4);
        Line(17,21,28,21,1,brass); Line(62,105,73,105,1,brass);
    } else {
        const glm::vec3 ink = c.type == Type::Frost || c.type == Type::Clear ? dark : ivory;
        // A square/diamond frame explicitly distinguishes permanent/temporary types.
        Shape("t1_quad",49,65,49,49,ink,Temporary(c.type) ? PI/4 : 0,.15f);
        Shape("t1_quad",49,65,45,45,TypeColor(c.type),Temporary(c.type) ? PI/4 : 0);
        Symbol(c.type,49,65,33,ink);
        Shape("t1_circle",19,106,24,24,brass);
        Shape("t1_circle",19,106,20,20,ivory);
        Center(Number(std::max(0,c.value)),19,114,15,dark);
        // Required geometric value units are on the card's side, not just a numeral.
        for (int i = 0; i < std::max(0,c.value); ++i) {
            Box(Rect(11,77-i*12.f,7,8),ink);
            Frame(Rect(11,77-i*12.f,7,8),brass,.7f);
        }
        Box(Rect(7,8,76,21),ivory);
        Center(Name(c.type),45,23,c.type == Type::Clear ? 8 : 9,dark);
        Center(Temporary(c.type) ? "TEMPORARY" : "PERMANENT",47,41,6.5f,ink);
    }
}

void Tema1::Tooltip(int id) {
    const tema1::Card& c = game.cards[id];
    Box(Rect(374,704,892,31),dark,.98f);
    Frame(Rect(374,704,892,31),brass,1);
    Center(std::string(Name(c.type)) + "  /  " + Descriptions[static_cast<int>(c.type)],820,724,10,ivory);
}

void Tema1::Render() {
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    if (!frameDirty && !forceRebuild) { renderer->Redraw(); return; }
    renderer->Begin();
    group = glm::mat3(1);
    Board(); Status(0); Status(1); Deck(0); Deck(1);
    for (int o = 0; o < 2; ++o) {
        for (int id : game.players[o].board) {
            bool face = true;
            float flip = 1;
            if (id == enemyCard && (state == State::EnemyPlace || state == State::Reveal)) {
                face = false;
                if (state == State::Reveal) {
                    const float angle = PI * std::min(1.f,clock/.35f);
                    flip = std::max(.015f,std::abs(std::cos(angle)));
                    face = angle >= PI/2;
                }
            }
            Card(id,visuals[id].at,face,id == hovered,flip);
        }
        for (int id : game.players[o].hand)
            if (id != dragged) Card(id,visuals[id].at,o == 0,id == hovered);
    }
    for (const auto& entry : visuals) if (entry.second.dying && entry.second.vanish < 1) {
        const Visual& v = entry.second;
        const float scale = 1-v.vanish;
        Rect r(v.at.x+v.at.w*(1-scale)/2,v.at.y+v.at.h*(1-scale)/2,v.at.w*scale,v.at.h*scale);
        if (scale > .02f) Card(entry.first,r,true);
    }
    if (hovered >= 0 && dragged < 0) Tooltip(hovered);
    if (dragged >= 0) {
        Card(dragged,visuals[dragged].at,true,true);
    }
    if (state == State::Over) {
        Box(Rect(0,0,W,H),glm::vec3(0),.65f);
        Box(Rect(470,284,660,330),dark,1,1); Frame(Rect(470,284,660,330),brass,2);
        Frame(Rect(478,292,644,314),brass,1);
        Ornament(800,566,490);
        Center(message,800,535,46,gold);
        Center("FINAL HEALTH",800,470,13,muted);
        Center("YOU  " + Number(game.players[0].hp) + "   :   " + Number(game.players[1].hp) + "  OPPONENT",800,436,22,ivory);
        Center(detail,800,392,12,muted);
        const bool over = Rect(650,316,300,55).Contains(mouse);
        Box(Rect(650,316,300,55),over ? brass : glm::vec3(.22f,.16f,.09f));
        Frame(Rect(650,316,300,55),gold,1);
        Center("PLAY AGAIN  [R]",800,353,19,ivory);
    }
    if (help) {
        Box(Rect(0,0,W,H),glm::vec3(0),.78f);
        Box(Rect(335,185,930,550),dark,1,1); Frame(Rect(335,185,930,550),brass,2);
        Center("THE RULES OF THE TABLE",800,701,28,gold);
        Center("ONE CARD. ONE RESPONSE. ALL SURVIVING EFFECTS.",800,652,13,muted);
        for (int i = 0; i < 6; ++i) {
            const float y = 596-i*49.f;
            Symbol(static_cast<Type>(i),382,y-5,22,i == static_cast<int>(Type::Spy)
                   ? ivory : TypeColor(static_cast<Type>(i)));
            Label(Name(static_cast<Type>(i)),410,y+6,13,gold);
            Label(Descriptions[i],555,y+6,10,ivory);
        }
        Center("ORDER: SPY > CLEAR > FROST > SHIELD > DAMAGE > HEAL",800,282,13,gold);
        Center("No cards are drawn after the opening deal. Reach 0 HP and you lose.",800,246,11,ivory);
        Center("Both hands empty: higher HP wins. Equal HP or simultaneous lethal: draw.",800,225,11,ivory);
        Center("H OR CLICK TO RETURN",800,204,10,muted);
    }
    renderer->Submit(forceRebuild);
    frameDirty = false;
}

#include "lab_m1/Tema1/tests/SceneChecks.inl"
