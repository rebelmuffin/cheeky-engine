#pragma once

#include <deque>
#include <functional>

namespace Utils
{
    class DeletionQueue
    {
      public:
        using FunctionType = std::function<void()>;
        void PushFunction(const FunctionType& func);
        void Flush();

      private:
        std::deque<std::function<void()>> m_deletors;
    };
} // namespace Utils