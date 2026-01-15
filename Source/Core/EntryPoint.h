#pragma once

extern Kaya::Application* Kaya::CreateApplication();

int main(int argc, char** argv) {
    auto app = Kaya::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
