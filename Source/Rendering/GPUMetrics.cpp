#include "Rendering/GPUMetrics.h"
#include <glad/glad.h>
#include <imgui.h>
#include <cstring>
#include <algorithm>

namespace Kaya {

GPUMetrics GPUMetricsManager::s_Metrics;
bool GPUMetricsManager::s_Visible = true;
bool GPUMetricsManager::s_Initialized = false;
unsigned int GPUMetricsManager::s_QueryStart[2] = { 0, 0 };
unsigned int GPUMetricsManager::s_QueryEnd[2] = { 0, 0 };
int GPUMetricsManager::s_CurrentQuery = 0;
bool GPUMetricsManager::s_QueryReady = false;

void GPUMetricsManager::Init() {
    // Query GPU info
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    s_Metrics.GPUVendor = vendor ? vendor : "Unknown";
    s_Metrics.GPURenderer = renderer ? renderer : "Unknown";
    s_Metrics.GLVersion = version ? version : "Unknown";

    // Try to query VRAM via NV extension (GL_NVX_gpu_memory_info)
    // 0x9047 = GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
    // 0x9049 = GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
    if (strstr(vendor ? vendor : "", "NVIDIA") != nullptr) {
        glGetIntegerv(0x9047, &s_Metrics.TotalVRAMKB);
        glGetIntegerv(0x9049, &s_Metrics.AvailableVRAMKB);
    }
    // AMD: GL_ATI_meminfo  0x87FB = VBO_FREE_MEMORY_ATI
    else if (strstr(vendor ? vendor : "", "ATI") != nullptr || strstr(vendor ? vendor : "", "AMD") != nullptr) {
        GLint memInfo[4] = {};
        glGetIntegerv(0x87FB, memInfo);
        s_Metrics.AvailableVRAMKB = memInfo[0];
    }

    // Create timer query objects (double-buffered to avoid stalls)
    glGenQueries(2, s_QueryStart);
    glGenQueries(2, s_QueryEnd);

    // Initialize history to zero
    memset(s_Metrics.FPSHistory, 0, sizeof(s_Metrics.FPSHistory));
    memset(s_Metrics.CPUHistory, 0, sizeof(s_Metrics.CPUHistory));
    memset(s_Metrics.GPUHistory, 0, sizeof(s_Metrics.GPUHistory));

    s_Initialized = true;
}

void GPUMetricsManager::Shutdown() {
    if (!s_Initialized) return;

    glDeleteQueries(2, s_QueryStart);
    glDeleteQueries(2, s_QueryEnd);
    s_Initialized = false;
}

void GPUMetricsManager::BeginFrame() {
    if (!s_Initialized) return;

    s_Metrics.Reset();

    // Begin GPU timer query for current frame
    glQueryCounter(s_QueryStart[s_CurrentQuery], GL_TIMESTAMP);
}

void GPUMetricsManager::EndFrame(float cpuDeltaTime) {
    if (!s_Initialized) return;

    // End GPU timer query
    glQueryCounter(s_QueryEnd[s_CurrentQuery], GL_TIMESTAMP);

    // Read the OTHER query (from last frame) to avoid GPU stall
    int readQuery = 1 - s_CurrentQuery;

    if (s_QueryReady) {
        GLint available = 0;
        glGetQueryObjectiv(s_QueryEnd[readQuery], GL_QUERY_RESULT_AVAILABLE, &available);

        if (available) {
            GLuint64 startTime = 0, endTime = 0;
            glGetQueryObjectui64v(s_QueryStart[readQuery], GL_QUERY_RESULT, &startTime);
            glGetQueryObjectui64v(s_QueryEnd[readQuery], GL_QUERY_RESULT, &endTime);
            s_Metrics.GPUFrameTimeMs = static_cast<float>(endTime - startTime) / 1000000.0f;
        }
    }

    // CPU timing
    s_Metrics.CPUFrameTimeMs = cpuDeltaTime * 1000.0f;
    s_Metrics.FPS = cpuDeltaTime > 0.0f ? 1.0f / cpuDeltaTime : 0.0f;

    // Smooth and record history
    s_Metrics.Smooth();
    s_Metrics.PushHistory();

    // Update VRAM availability each frame (cheap on NVIDIA)
    if (s_Metrics.TotalVRAMKB > 0) {
        glGetIntegerv(0x9049, &s_Metrics.AvailableVRAMKB);
    }

    // Swap query buffer
    s_QueryReady = true;
    s_CurrentQuery = readQuery;
}

void GPUMetricsManager::RecordDrawCall(unsigned int vertexCount) {
    s_Metrics.DrawCalls++;
    s_Metrics.Vertices += vertexCount;
    s_Metrics.Triangles += vertexCount / 3;
}

void GPUMetricsManager::RecordIndexedDrawCall(unsigned int indexCount) {
    s_Metrics.DrawCalls++;
    s_Metrics.Vertices += indexCount;
    s_Metrics.Triangles += indexCount / 3;
}

void GPUMetricsManager::RecordShaderBind() {
    s_Metrics.ShaderBinds++;
}

void GPUMetricsManager::RecordTextureBind() {
    s_Metrics.TextureBinds++;
}

void GPUMetricsManager::RenderOverlay() {
    if (!s_Visible || !s_Initialized) return;

    const float PADDING = 10.0f;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                           | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoSavedSettings
                           | ImGuiWindowFlags_NoFocusOnAppearing
                           | ImGuiWindowFlags_NoNav
                           | ImGuiWindowFlags_NoMove;

    // Position in top-right corner
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    ImVec2 windowPos(workPos.x + workSize.x - PADDING, workPos.y + PADDING);
    ImVec2 windowPivot(1.0f, 0.0f);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
    ImGui::SetNextWindowBgAlpha(0.7f);

    if (ImGui::Begin("GPU Metrics", nullptr, flags)) {
        // Header
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "GPU Metrics");
        ImGui::Separator();

        // FPS with color coding
        ImVec4 fpsColor;
        if (s_Metrics.SmoothedFPS >= 60.0f)
            fpsColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);  // Green
        else if (s_Metrics.SmoothedFPS >= 30.0f)
            fpsColor = ImVec4(1.0f, 1.0f, 0.2f, 1.0f);   // Yellow
        else
            fpsColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);   // Red

        ImGui::TextColored(fpsColor, "FPS: %.0f", s_Metrics.SmoothedFPS);

        // Frame times
        ImGui::Text("CPU: %.2f ms", s_Metrics.SmoothedCPUMs);
        ImGui::Text("GPU: %.2f ms", s_Metrics.SmoothedGPUMs);

        ImGui::Separator();

        // Draw call stats
        ImGui::Text("Draw Calls: %u", s_Metrics.DrawCalls);
        ImGui::Text("Triangles:  %u", s_Metrics.Triangles);
        ImGui::Text("Vertices:   %u", s_Metrics.Vertices);
        ImGui::Text("Shader Binds: %u", s_Metrics.ShaderBinds);
        ImGui::Text("Texture Binds: %u", s_Metrics.TextureBinds);

        // VRAM info (if available)
        if (s_Metrics.TotalVRAMKB > 0) {
            ImGui::Separator();
            int usedMB = (s_Metrics.TotalVRAMKB - s_Metrics.AvailableVRAMKB) / 1024;
            int totalMB = s_Metrics.TotalVRAMKB / 1024;
            float usage = static_cast<float>(usedMB) / static_cast<float>(totalMB);
            ImGui::Text("VRAM: %d / %d MB", usedMB, totalMB);
            ImGui::ProgressBar(usage, ImVec2(180.0f, 14.0f));
        }

        ImGui::Separator();

        // FPS graph
        ImGui::Text("FPS History:");
        // Rotate history into contiguous array for plotting
        float sortedFPS[GPUMetrics::HISTORY_SIZE];
        for (int i = 0; i < GPUMetrics::HISTORY_SIZE; i++) {
            sortedFPS[i] = s_Metrics.FPSHistory[(s_Metrics.HistoryOffset + i) % GPUMetrics::HISTORY_SIZE];
        }
        float maxFPS = *std::max_element(sortedFPS, sortedFPS + GPUMetrics::HISTORY_SIZE);
        if (maxFPS < 10.0f) maxFPS = 60.0f;
        ImGui::PlotLines("##fps", sortedFPS, GPUMetrics::HISTORY_SIZE, 0, nullptr, 0.0f, maxFPS * 1.1f, ImVec2(180.0f, 40.0f));

        // GPU time graph
        ImGui::Text("GPU Time (ms):");
        float sortedGPU[GPUMetrics::HISTORY_SIZE];
        for (int i = 0; i < GPUMetrics::HISTORY_SIZE; i++) {
            sortedGPU[i] = s_Metrics.GPUHistory[(s_Metrics.HistoryOffset + i) % GPUMetrics::HISTORY_SIZE];
        }
        float maxGPU = *std::max_element(sortedGPU, sortedGPU + GPUMetrics::HISTORY_SIZE);
        if (maxGPU < 1.0f) maxGPU = 16.6f;
        ImGui::PlotLines("##gpu", sortedGPU, GPUMetrics::HISTORY_SIZE, 0, nullptr, 0.0f, maxGPU * 1.1f, ImVec2(180.0f, 40.0f));

        ImGui::Separator();

        // GPU info
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", s_Metrics.GPURenderer.c_str());
    }
    ImGui::End();
}

void GPUMetricsManager::SetVisible(bool visible) {
    s_Visible = visible;
}

bool GPUMetricsManager::IsVisible() {
    return s_Visible;
}

const GPUMetrics& GPUMetricsManager::GetMetrics() {
    return s_Metrics;
}

} // namespace Kaya
