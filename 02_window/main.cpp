import LetsMakeEngine;

using namespace LetsMakeEngine;

int main() {
    // Initialize systems
    auto window = MakeWindow( WindowConfig{
        .title = "Let's Make an Engine",
        .width = 1024,
        .height = 768,
        .mode = WindowMode::windowed
    });

    while (PumpEvents()) {
        // Update game state
        // Render frame
    }

    // Shutdown systems
}
