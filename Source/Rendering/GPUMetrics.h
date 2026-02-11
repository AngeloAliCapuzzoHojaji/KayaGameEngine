#pragma once

#include <string>

namespace Kaya {

struct GPUMetrics {
    // Per-frame stats (reset each frame)
    unsigned int DrawCalls = 0;
    unsigned int Vertices = 0;
    unsigned int Triangles = 0;
    unsigned int ShaderBinds = 0;
    unsigned int TextureBinds = 0;

    // Culling stats
    unsigned int ObjectsTested = 0;
    unsigned int ObjectsCulled = 0;

    // Timing
    float CPUFrameTimeMs = 0.0f;
    float GPUFrameTimeMs = 0.0f;
    float FPS = 0.0f;

    // Smoothed values for display
    float SmoothedFPS = 0.0f;
    float SmoothedCPUMs = 0.0f;
    float SmoothedGPUMs = 0.0f;

    // GPU info (queried once)
    std::string GPUVendor;
    std::string GPURenderer;
    std::string GLVersion;
    int TotalVRAMKB = 0;       // Total VRAM (if available via extension)
    int AvailableVRAMKB = 0;   // Available VRAM (if available via extension)

    // History for graphs
    static constexpr int HISTORY_SIZE = 120;
    float FPSHistory[HISTORY_SIZE] = {};
    float CPUHistory[HISTORY_SIZE] = {};
    float GPUHistory[HISTORY_SIZE] = {};
    int HistoryOffset = 0;

    void Reset() {
        DrawCalls = 0;
        Vertices = 0;
        Triangles = 0;
        ShaderBinds = 0;
        TextureBinds = 0;
        ObjectsTested = 0;
        ObjectsCulled = 0;
    }

    void PushHistory() {
        FPSHistory[HistoryOffset] = FPS;
        CPUHistory[HistoryOffset] = CPUFrameTimeMs;
        GPUHistory[HistoryOffset] = GPUFrameTimeMs;
        HistoryOffset = (HistoryOffset + 1) % HISTORY_SIZE;
    }

    void Smooth(float alpha = 0.05f) {
        SmoothedFPS = SmoothedFPS + alpha * (FPS - SmoothedFPS);
        SmoothedCPUMs = SmoothedCPUMs + alpha * (CPUFrameTimeMs - SmoothedCPUMs);
        SmoothedGPUMs = SmoothedGPUMs + alpha * (GPUFrameTimeMs - SmoothedGPUMs);
    }
};

class GPUMetricsManager {
public:
    static void Init();
    static void Shutdown();

    // Call at the start of the render frame (before any draw calls)
    static void BeginFrame();
    // Call at the end of the render frame (after all draw calls, before swap)
    static void EndFrame(float cpuDeltaTime);

    // Tracking helpers - called by Renderer internally
    static void RecordDrawCall(unsigned int vertexCount);
    static void RecordIndexedDrawCall(unsigned int indexCount);
    static void RecordShaderBind();
    static void RecordTextureBind();
    static void RecordFrustumTest(bool culled);

    // Render the overlay
    static void RenderOverlay();

    // Toggle visibility
    static void SetVisible(bool visible);
    static bool IsVisible();

    static void SetDebugCullingActive(bool active);
    static bool IsDebugCullingActive();

    static const GPUMetrics& GetMetrics();

private:
    static GPUMetrics s_Metrics;
    static bool s_Visible;
    static bool s_DebugCullingActive;
    static bool s_Initialized;

    // OpenGL timer query objects (double-buffered)
    static unsigned int s_QueryStart[2];
    static unsigned int s_QueryEnd[2];
    static int s_CurrentQuery;
    static bool s_QueryReady;
};

} // namespace Kaya
