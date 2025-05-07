#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <asset-loader.hpp>
#include "../common/ecs/entity.hpp"
#include <systems/physics.hpp>


// This state shows how to use the ECS framework and deserialization.
class Playstate : public our::State
{

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::PhysicsSystem physicsSystem;

    float demonSpawnTimer = 0.0f;
    int maxDemons = 5;
    float gameDuration = 180.0f; // 3 minutes (in seconds)
    float elapsedTime = 0.0f;
    bool gameEnded = false;
    
    // Win condition variables
    int totalCollectibles = 10; // Total number of collectibles in the level
    int collectedItems = 0;     // Number of items collected by the player
    bool winConditionMet = false; // Flag for win condition

    void onInitialize() override
    {
        // First of all, we get the scene configuration from the app config
        auto &config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if (config.contains("assets"))
        {
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if (config.contains("world"))
        {
            world.deserialize(config["world"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
        physicsSystem.initialize(&world, size);
        physicsSystem.initializeDemons(&world, 10); // Pool size 10
        elapsedTime = 0.0f; // Reset timer when state starts
        gameEnded = false;
        
        // Reset win condition variables
        collectedItems = 0;
        winConditionMet = false;
    }

    void onDraw(double deltaTime) override
    {
        if(!gameEnded) {
            elapsedTime += deltaTime;
            
            // Check if time is up (lose condition)
            if(elapsedTime >= gameDuration) {
                gameEnded = true;
                // Optional: Play time's up sound
                // audioSystem.playSound("time_up");
                getApp()->changeState("game-over");
                return; // Skip rest of the frame
            }
            
            // Check for win condition (collected all items)
            // For demonstration, let's say the player can press the 'C' key to simulate collecting an item

            if(physicsSystem.totalkilled == 10) {
                winConditionMet = true;
                gameEnded = true;
                // Optional: Play win sound
                // audioSystem.playSound("win");
                getApp()->changeState("win");
                return; // Skip rest of the frame
            }
        }

        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);
        // Get a reference to the keyboard object
        auto &keyboard = getApp()->getKeyboard();

        physicsSystem.update(&world, (float)deltaTime, getApp());
        physicsSystem.updateCharacterMovement(&world, cameraController, (float)deltaTime);
        // physicsSystem.debugDrawWorld(&world);
        physicsSystem.updateSpawning(&world, deltaTime);

        if (keyboard.justPressed(GLFW_KEY_ESCAPE))
        {
            // If the escape key is pressed in this frame, go to the menu state
            getApp()->changeState("menu");
        }
    }

    // void onImmediateGui() override {
    //     renderHealthHUD();
    // }
    // void renderHealthHUD() {
    //     // Set up ImGui window in bottom right
    //     ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
    //                             ImGuiWindowFlags_AlwaysAutoResize |
    //                             ImGuiWindowFlags_NoSavedSettings |
    //                             ImGuiWindowFlags_NoFocusOnAppearing |
    //                             ImGuiWindowFlags_NoNav |
    //                             ImGuiWindowFlags_NoMove;
        
    //     const float PAD = 10.0f;
    //     const ImGuiViewport* viewport = ImGui::GetMainViewport();
    //     ImVec2 work_pos = viewport->WorkPos;
    //     ImVec2 work_size = viewport->WorkSize;
    //     ImVec2 window_pos, window_pos_pivot;
    //     window_pos.x = work_pos.x + work_size.x - PAD;
    //     window_pos.y = work_pos.y + work_size.y - PAD;
    //     window_pos_pivot.x = 1.0f;
    //     window_pos_pivot.y = 1.0f;
        
    //     ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    //     ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        
    //     if (ImGui::Begin("Player Health", nullptr, flags)) {
    //         Entity* player = world->getEntity("player");
    //         if (player) {
    //             auto health = player->getComponent<HealthComponent>();
    //             if (health) {
    //                 // Health bar
    //                 float healthPercent = (float)health->currentHealth / (float)health->maxHealth;
    //                 ImGui::Text("Health: %d/%d", health->currentHealth, health->maxHealth);
    //                 ImGui::ProgressBar(healthPercent, ImVec2(-FLT_MIN, 20), 
    //                                     healthPercent >= 0.6f ? "Healthy" : 
    //                                     healthPercent >= 0.3f ? "Injured" : "Critical");
                    
    //                 // Show invulnerability effect if active
    //                 if (health->invulnerabilityTimer > 0) {
    //                     ImGui::TextColored(ImVec4(1,1,0,1), "Invulnerable!");
    //                 }
    //             }
    //         }
    //     }
    //     ImGui::End();
    // }

    void onDestroy() override
    {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};