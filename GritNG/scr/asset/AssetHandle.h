#pragma once
#include "../core/Types.h"
#include <memory>

namespace grit::asset {

    // Лёгкая обёртка-владелец над ресурсом
    template <typename T>
    class AssetHandle {
    public:
        AssetHandle() = default;
        explicit AssetHandle(std::shared_ptr<T> ptr) : m_ptr(std::move(ptr)) {}

        bool   valid()   const { return static_cast<bool>(m_ptr); }
        T* get()     const { return m_ptr.get(); }
        T& operator*()  const { return *m_ptr; }
        T* operator->() const { return  m_ptr.get(); }
        explicit operator bool() const { return valid(); }

        const std::shared_ptr<T>& shared() const { return m_ptr; }

    private:
        std::shared_ptr<T> m_ptr;
    };

} // namespace grit::asset