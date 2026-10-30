#pragma once

// Opt-in measurement only. Ordinary gameplay does not run these fixtures/timers.
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tema1 {
struct RenderChecks {
    struct Result {
        std::string name;
        double fps, p95, renderMean;
    };
    std::string file;
    int scenario = 0, animationCycle = 0;
    bool setup = false, started = false;
    double elapsed = 0, frameMs = 0;
    std::chrono::steady_clock::time_point previous;
    std::vector<double> frames, renders;
    std::vector<Result> results;

    const char* Name() const {
        static const char* names[] = {"leaders", "table", "drag", "deal", "reveal", "hover", "help"};
        return names[scenario];
    }
    void BeginFrame() {
        const auto now = std::chrono::steady_clock::now();
        if (started) {
            frameMs = std::chrono::duration<double, std::milli>(now - previous).count();
            elapsed += frameMs / 1000;
        }
        previous = now; started = true;
    }
    bool Record(double renderMs, int width, int height) {
        // Two seconds of warmup, then 30 seconds including normal presentation/VSync.
        if (elapsed < 2) return false;
        frames.push_back(frameMs); renders.push_back(renderMs);
        if (elapsed < 32) return false;
        double total = 0, renderTotal = 0;
        for (double value : frames) total += value;
        for (double value : renders) renderTotal += value;
        std::sort(frames.begin(), frames.end());
        const double fps = 1000 * frames.size() / total;
        const double p95 = frames[(frames.size() - 1) * 95 / 100];
        results.push_back({Name(), fps, p95, renderTotal / renders.size()});
        std::cout << "RENDER CHECK " << Name() << ": " << fps << " FPS, p95 " << p95
                  << " ms, render CPU " << results.back().renderMean << " ms\n" << std::flush;
        ++scenario; setup = started = false; elapsed = 0;
        frames.clear(); renders.clear();
        if (scenario < 7) return false;
        std::ofstream out(file);
        out << "{\n  \"sample_seconds\": 30,\n  \"warmup_seconds\": 2,\n  \"framebuffer_width\": " << width
            << ",\n  \"framebuffer_height\": " << height << ",\n  \"scenarios\": [\n";
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            out << "    {\"name\": \"" << r.name << "\", \"fps\": " << r.fps
                << ", \"p95_frame_ms\": " << r.p95 << ", \"mean_render_cpu_ms\": " << r.renderMean << "}"
                << (i + 1 < results.size() ? "," : "") << '\n';
        }
        out << "  ]\n}\n";
        if (!out) throw std::runtime_error("Cannot write rendering measurements");
        return true;
    }
};
}  // namespace tema1
