#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>

#include <functional>
#include <array>

// This struct is used to store the location and size of a button and the code it should execute when clicked
struct GameOverButton {
    // The position (of the top-left corner) of the button and its size in pixels
    glm::vec2 position, size;
    // The function that should be executed when the button is clicked. It takes no arguments and returns nothing.
    std::function<void()> action;

    // This function returns true if the given vector v is inside the button. Otherwise, false is returned.
    // This is used to check if the mouse is hovering over the button.
    bool isInside(const glm::vec2& v) const {
        return position.x <= v.x && position.y <= v.y &&
            v.x <= position.x + size.x &&
            v.y <= position.y + size.y;
    }

    // This function returns the local to world matrix to transform a rectangle of size 1x1
    // (and whose top-left corner is at the origin) to be the button.
    glm::mat4 getLocalToWorld() const {
        return glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f)) * 
            glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
    }
};

// This state shows a game over screen and provides options to retry or go back to the menu
class GameOverState: public our::State {

    // A material holding the game over shader and the game over texture to draw
    our::TexturedMaterial* gameOverMaterial;
    // A material to be used to highlight hovered buttons (we will use blending to create a negative effect).
    our::TintedMaterial* highlightMaterial;
    // A rectangle mesh on which the game over material will be drawn
    our::Mesh* rectangle;
    // A variable to record the time since the state is entered (it will be used for the fading effect).
    float time;
    // An array of the buttons that we can interact with (retry and back to menu)
    std::array<GameOverButton, 2> buttons;

    void onInitialize() override {
        // First, we create a material for the game over background
        gameOverMaterial = new our::TexturedMaterial();
        // Here, we load the shader that will be used to draw the background
        gameOverMaterial->shader = new our::ShaderProgram();
        gameOverMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        gameOverMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        gameOverMaterial->shader->link();
        // Then we load the game over texture
        gameOverMaterial->texture = our::texture_utils::loadImage("assets/textures/game_over.jpg"); // Using menu texture for now, should be replaced with a game over texture
        // Initially, the game over material will be black, then it will fade in
        gameOverMaterial->tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        gameOverMaterial->sampler = new our::Sampler();
        gameOverMaterial->sampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gameOverMaterial->sampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Second, we create a material to highlight the hovered buttons
        highlightMaterial = new our::TintedMaterial();
        // Since the highlight is not textured, we used the tinted material shaders
        highlightMaterial->shader = new our::ShaderProgram();
        highlightMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        highlightMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        highlightMaterial->shader->link();
        // The tint is white since we will subtract the background color from it to create a negative effect.
        highlightMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        // To create a negative effect, we enable blending, set the equation to be subtract,
        // and set the factors to be one for both the source and the destination. 
        highlightMaterial->pipelineState.blending.enabled = true;
        highlightMaterial->pipelineState.blending.equation = GL_FUNC_SUBTRACT;
        highlightMaterial->pipelineState.blending.sourceFactor = GL_ONE;
        highlightMaterial->pipelineState.blending.destinationFactor = GL_ONE;

        // Then we create a rectangle whose top-left corner is at the origin and its size is 1x1.
        // Note that the texture coordinates at the origin is (0.0, 1.0) since we will use the 
        // projection matrix to make the origin at the the top-left corner of the screen.
        rectangle = new our::Mesh({
            {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        },{
            0, 1, 2, 2, 3, 0,
        });

        // Reset the time elapsed since the state is entered.
        time = 0;

        // Fill the positions, sizes and actions for the game over buttons
        buttons[0].position = {830.0f, 607.0f};
        buttons[0].size = {400.0f, 33.0f};
        buttons[0].action = [this](){this->getApp()->changeState("play");}; // Retry - go back to play state

        buttons[1].position = {830.0f, 644.0f};
        buttons[1].size = {400.0f, 33.0f};
        buttons[1].action = [this](){this->getApp()->changeState("menu");}; // Back to menu
    }

    void onDraw(double deltaTime) override {
        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_SPACE)){
            // If the space key is pressed in this frame, go to the play state (retry)
            getApp()->changeState("play");
        } else if(keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            // If the escape key is pressed in this frame, go back to the menu
            getApp()->changeState("menu");
        }

        // Get a reference to the mouse object and get the current mouse position
        auto& mouse = getApp()->getMouse();
        glm::vec2 mousePosition = mouse.getMousePosition();

        // If the mouse left-button is just pressed, check if the mouse was inside
        // any game over button. If it was inside a button, run the action of the button.
        if(mouse.justPressed(0)){
            for(auto& button: buttons){
                if(button.isInside(mousePosition))
                    button.action();
            }
        }

        // Get the framebuffer size to set the viewport and create the projection matrix.
        glm::ivec2 size = getApp()->getFrameBufferSize();
        // Make sure the viewport covers the whole size of the framebuffer.
        glViewport(0, 0, size.x, size.y);

        // The view matrix is an identity (there is no camera that moves around).
        // The projection matrix applies an orthographic projection whose size is the framebuffer size in pixels
        // so that we can define our object locations and sizes in pixels.
        // Note that the top is at 0.0 and the bottom is at the framebuffer height. This allows us to consider the top-left
        // corner of the window to be the origin which makes dealing with the mouse input easier. 
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        // The local to world (model) matrix of the background which is just a scaling matrix to make the game over screen cover the whole
        // window. Note that we define the scale in pixels.
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        // First, we apply the fading effect.
        time += (float)deltaTime;
        gameOverMaterial->tint = glm::vec4(glm::smoothstep(0.00f, 2.00f, time));
        // Then we render the game over background
        // Notice that I don't clear the screen first, since I assume that the game over rectangle will draw over the whole
        // window anyway.
        gameOverMaterial->setup();
        gameOverMaterial->shader->set("transform", VP*M);
        rectangle->draw();

        // For every button, check if the mouse is inside it. If the mouse is inside, we draw the highlight rectangle over it.
        for(auto& button: buttons){
            if(button.isInside(mousePosition)){
                highlightMaterial->setup();
                highlightMaterial->shader->set("transform", VP*button.getLocalToWorld());
                rectangle->draw();
            }
        }
    }

    void onDestroy() override {
        // Delete all the allocated resources
        delete rectangle;
        delete gameOverMaterial->texture;
        delete gameOverMaterial->shader;
        delete gameOverMaterial;
        delete highlightMaterial->shader;
        delete highlightMaterial;
    }
};