#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace tema1 {
// Opt-in, wall-clock benchmark of an idle, fully dealt game. The one-second
// warmup excludes initialization; the next six seconds include presentation/vsync.
class FrameBenchmark {
 public:
    std::string file;
    bool Record(float frameSeconds, double renderMilliseconds, bool ready, int width, int height) {
        if (file.empty() || !ready) return false;
        const auto now = std::chrono::steady_clock::now();
        if (!started) { start = now; started = true; return false; }
        const double elapsed = std::chrono::duration<double>(now - start).count();
        if (elapsed < 1) return false;
        frames.push_back(frameSeconds * 1000.0);
        renders.push_back(renderMilliseconds);
        if (elapsed < 7) return false;
        double total = 0, renderTotal = 0;
        for (double t : frames) total += t;
        for (double t : renders) renderTotal += t;
        std::sort(frames.begin(), frames.end());
        const double mean = total / frames.size();
        std::ofstream out(file);
        out << "{\n  \"frames\": " << frames.size()
            << ",\n  \"fps\": " << 1000.0 / mean
            << ",\n  \"mean_frame_ms\": " << mean
            << ",\n  \"p95_frame_ms\": " << frames[(frames.size() - 1) * 95 / 100]
            << ",\n  \"mean_render_cpu_ms\": " << renderTotal / renders.size()
            << ",\n  \"framebuffer_width\": " << width
            << ",\n  \"framebuffer_height\": " << height << "\n}\n";
        std::cout << "BENCHMARK: " << 1000.0 / mean << " FPS, " << mean
                  << " ms/frame, " << renderTotal / renders.size() << " ms render CPU; " << file << '\n';
        file.clear();
        return true;
    }
 private:
    bool started = false;
    std::chrono::steady_clock::time_point start;
    std::vector<double> frames, renders;
};
}
