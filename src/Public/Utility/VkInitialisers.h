#pragma once

#include "VkTypes.h"

namespace Utils
{
    VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queue_family_index, VkCommandPoolCreateFlags flags);
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool command_pool, uint32_t count);
} // namespace Utils