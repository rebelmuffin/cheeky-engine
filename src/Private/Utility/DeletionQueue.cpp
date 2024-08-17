#include "Utility/DeletionQueue.h"

namespace Utils
{
    void DeletionQueue::PushFunction(const FunctionType& func)
    {
        m_deletors.push_back(func);
    }

    void DeletionQueue::Flush()
    {
        for (const FunctionType& func : m_deletors)
        {
            func();
        }

        m_deletors.clear();
    }

} // namespace Utils