#include "Application.hpp"

int main(int argc, char *argv[])
{
    RCG::Application app;

    if (!app.initialize())
    {
        return 1;
    }

    // Main loop
    while (app.update())
    {
        // Event loop is handled in Application::update()
    }

    // Cleanup happens in Application::shutdown()
    return 0;
}
