#pragma once

#include <deque>
#include <functional>

namespace Renderer::Utils
{
    using FunctionType = std::function<void()>;

    struct DeletionItem
    {
        FunctionType function;
        const char* debug_name;
    };

    class DeletionQueue
    {
      public:
        void PushFunction(const char* debug_name, const FunctionType& func);
        void Flush();

      private:
        std::deque<DeletionItem> m_deletors;
    };
} // namespace Renderer::Utils