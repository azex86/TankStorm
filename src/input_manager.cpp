#include "../header/input_manager.h"

InputManager::InputManager(const Config& config)
    : config(config),
      moveLeftPressed(false),
      moveRightPressed(false),
      mouseLeftPressed(false),
      mouseLeftWasPressed(false),
      mouseX(0),
      mouseY(0)
{
}

InputManager::~InputManager()
{
}

void InputManager::update(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == config.getMoveLeftKey()) {
            moveLeftPressed = true;
        }
        if (event.key.code == config.getMoveRightKey()) {
            moveRightPressed = true;
        }
    }
    else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == config.getMoveLeftKey()) {
            moveLeftPressed = false;
        }
        if (event.key.code == config.getMoveRightKey()) {
            moveRightPressed = false;
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            mouseLeftPressed = true;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            mouseLeftPressed = false;
        }
    }
    else if (event.type == sf::Event::MouseMoved) {
        mouseX = event.mouseMove.x;
        mouseY = event.mouseMove.y;
    }
}

void InputManager::updateMousePosition(int x, int y)
{
    mouseX = x;
    mouseY = y;
}

bool InputManager::isMoveLeftPressed() const
{
    return moveLeftPressed;
}

bool InputManager::isMoveRightPressed() const
{
    return moveRightPressed;
}

bool InputManager::isShootJustPressed()
{
    if (config.isShootMouseButton()) {
        bool justPressed = mouseLeftPressed && !mouseLeftWasPressed;
        mouseLeftWasPressed = mouseLeftPressed;
        return justPressed;
    }
    return false;
}

void InputManager::resetFrameState()
{
    // Reset per-frame state if needed
}
