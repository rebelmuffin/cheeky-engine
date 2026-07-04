#pragma once

#include "Game/GameScene.h"

namespace Game::Utils
{
    /// Load the given gltf file as a scene under the given node.
    void LoadGltfIntoGameScene(Renderer::VulkanEngine& engine, Node& node, std::filesystem::path file_path);
} // namespace Game::Utils