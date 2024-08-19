#include "Utility/VkPipelines.h"

#include "Utility/VkInitialisers.h"

#include <fstream>

namespace Utils
{
    bool LoadShaderModule(vkb::DispatchTable device_dispatch, const char* file_path, VkShaderModule* out_shader_module)
    {
        // open with cursor at the end
        std::ifstream file(file_path, std::ios::ate | std::ios::binary);
        if (file.is_open() == false)
        {
            return false;
        }

        std::size_t file_size = std::size_t(file.tellg());
        // spirv expects uint32_t buffer
        std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

        // seek to file begin
        file.seekg(0);

        // load into buffer
        file.read((char*)buffer.data(), int64_t(file_size));

        file.close();

        VkShaderModuleCreateInfo shader_create_info{};
        shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_create_info.pNext = nullptr;
        shader_create_info.pCode = buffer.data();
        shader_create_info.codeSize = uint32_t(buffer.size() * sizeof(uint32_t));

        // check for success. Here we tolerate failures, don't wanna abort just because cannot load shader
        VkShaderModule shader_module;
        VkResult result = device_dispatch.createShaderModule(&shader_create_info, nullptr, &shader_module);
        if (result != VK_SUCCESS)
        {
            std::cout << "[!] Failed to load shader at: " << file_path << std::endl;
        }

        *out_shader_module = shader_module;
        return true;
    }
} // namespace Utils