#pragma once

#include "VkTypes.h"
#include <VkBootstrapDispatch.h>

namespace Utils
{
    bool LoadShaderModule(vkb::DispatchTable device_dispatch, const char* file_path, VkShaderModule* out_shader_module);
}