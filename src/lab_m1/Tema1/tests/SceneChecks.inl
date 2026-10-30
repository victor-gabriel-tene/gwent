// Application-level checks use the actual scene handlers and GPU back buffer.
// Enable only for development: TEMA1_VERIFY_DIR must name an existing directory.
#include <fstream>
#include <iostream>
#include <stdexcept>

void Tema1::VerifyScene(const char* directory) {
    auto expect = [](bool valid, const char* message) {
        if (!valid) throw std::runtime_error(std::string("Tema1 scene check: ") + message);
    };
    auto pixel = [this](glm::vec2 logical) {
        int width, height;
        glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
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
    auto play = [this, &pixel, &expect](int id, glm::vec2 destination) {
        const Rect r = visuals.at(id).at;
        const auto from = pixel(glm::vec2(r.x + r.w/2, r.y + r.h/2));
        const auto to = pixel(destination);
        OnMouseBtnPress(from.x,from.y,1,0);
        expect(dragged == id, "hand rectangle selects expected card");
        OnMouseMove(to.x,to.y,0,0);
        expect(glm::length(glm::vec2(visuals[id].at.x,visuals[id].at.y) + grab - mouse) < 1,
               "drag preserves cursor grab offset");
        OnMouseBtnRelease(to.x,to.y,1,0);
    };
    expect(game.Invariant(), "card ownership after scene update");
    if (verifyStage == 0 && state == State::Ready) {
        expect(game.players[0].hand.size() == 10 && game.players[1].hand.size() == 10, "opening hand sizes");
        for (glm::vec2 logical : {glm::vec2(0),glm::vec2(820,325),glm::vec2(1600,900)}) {
            const auto p = pixel(logical);
            expect(glm::length(Pointer(p.x,p.y)-logical) < 3, "Retina coordinate round trip");
        }
        capture("01-opening");
        verifyCard = game.players[0].hand.front();
        verifyHand = static_cast<int>(game.players[0].hand.size());
        play(verifyCard,glm::vec2(1480,450));
        expect(state == State::Return && game.players[0].hand.size() == static_cast<size_t>(verifyHand),
               "invalid drop consumes no card or turn");
        ++verifyStage;
    } else if (verifyStage == 1 && state == State::Ready) {
        expect(game.turn == 1 && game.cards[verifyCard].zone == Zone::Hand, "invalid return preserves hand");
        capture("02-invalid-return");
        play(verifyCard,glm::vec2(820,325));
        expect(state == State::PlayerPlace && game.cards[verifyCard].zone == Zone::Board
            && game.players[0].hand.size() == static_cast<size_t>(verifyHand-1), "valid drop commits once");
        ++verifyStage;
    } else if (verifyStage == 2 && state == State::Ready && game.turn == 2) {
        expect(!game.players[1].board.empty() || !game.players[1].discard.empty(),
               "enemy placement finishes before next turn");
        capture("03-after-turn");
        OnKeyPress(GLFW_KEY_H,0);
        expect(help, "rules overlay opens");
        ++verifyStage;
    } else if (verifyStage == 3) {
        capture("04-rules");
        OnKeyPress(GLFW_KEY_H,0);
        glfwSetWindowSize(glfwGetCurrentContext(),1000,800);
        ++verifyStage;
    } else if (verifyStage == 4) {
        const auto p = pixel(glm::vec2(820,325));
        expect(glm::length(Pointer(p.x,p.y)-glm::vec2(820,325)) < 3, "resized coordinate round trip");
        capture("05-resized");
        ++verifyStage;
    } else if (verifyStage == 5) {
        if (state == State::Ready) play(game.players[0].hand.front(),glm::vec2(820,325));
        else if (state == State::Over) {
            capture("06-result");
            expect(game.turn <= 20, "complete UI match terminates");
            OnKeyPress(GLFW_KEY_R,0);
            expect(state == State::Deal && game.players[0].hp == 30 && game.players[0].hand.empty(),
                   "restart resets match and begins fresh deal");
            ++verifyStage;
        } else {
            // Keep each presentation state on screen for its full animation, but
            // skip idle gaps after animation targets have settled.
            bool moving = false;
            for (const auto& v : visuals)
                if (!v.second.dying && v.second.elapsed < v.second.duration) moving = true;
            if (!moving && state != State::Reveal) clock = wait;
        }
    } else if (verifyStage == 6 && state == State::Ready) {
        expect(game.turn == 1 && game.players[0].hand.size() == 10
            && game.players[1].hand.size() == 10 && game.players[0].discard.empty(), "fresh restarted deal");
        capture("07-restarted");
        std::cout << "PASS: scene checks (deal, invalid/valid drag, enemy/reveal/resolution, rules, "
                     "Retina mapping, resize, complete match, restart)\n" << std::flush;
        window->Close();
        ++verifyStage;
    }
}
