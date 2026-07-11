#pragma once

#include "Game/ECS.h"
#include "Game/GameScene.h"

namespace Game::Utils
{
    /// Load the given gltf file as a scene under the given node.
    void LoadGltfIntoGameScene(Renderer::VulkanEngine& engine, Node& node, std::filesystem::path file_path);

    void LoadGltfIntoGameSceneECS(ECS::World& world, Node& node, std::filesystem::path file_path);
} // namespace Game::Utils