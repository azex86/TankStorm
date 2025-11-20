#pragma once
#ifndef INPUT_MANAGER_HEADER
#define INPUT_MANAGER_HEADER

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include "../header/config.h"

class InputManager {
public:
    InputManager(const Config& config);
    ~InputManager();
    
    // Update input state
    void update(const sf::Event& event);
    void updateMousePosition(int x, int y);
    
    // Movement queries
    bool isMoveLeftPressed() const;
    bool isMoveRightPressed() const;
    
    // Shooting queries
    bool isShootJustPressed();
    
    // Mouse position
    int getMouseX() const { return mouseX; }
    int getMouseY() const { return mouseY; }
    
    // Reset state (for new frames)
    void resetFrameState();

private:
    const Config& config;
    
    // Keyboard state
    bool moveLeftPressed;
    bool moveRightPressed;
    
    // Mouse state
    bool mouseLeftPressed;
    bool mouseLeftWasPressed;
    int mouseX;
    int mouseY;
};

#endif // INPUT_MANAGER_HEADER
