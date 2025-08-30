#include "Renderer/Utility/DeletionQueue.h"
#include <iostream>

namespace Renderer::Utils
{
    void DeletionQueue::PushFunction(const char* debug_name, const FunctionType& func)
    {
        m_deletors.push_back({ func, debug_name });
    }

    void DeletionQueue::Flush()
    {
        for (auto it = m_deletors.crbegin(); it != m_deletors.crend(); ++it)
        {
#ifdef ENABLE_DEBUG_OUTPUT
            std::cout << "Deleting: " << it->debug_name << std::endl;
#endif
            it->function();
        }

        m_deletors.clear();
    }

} // namespace Renderer::Utils