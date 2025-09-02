#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Renderer
{
    class VulkanEngine;

    using StorageId_t = size_t;
    using ReferenceCount_t = uint32_t;

    constexpr StorageId_t INVALID_RESOURCE_ID = 0ul;

    template <typename T>
    struct ResourceStorage;

    /// Structure that increments/decrements the reference counter for the lifetime management of resources.
    /// Always refers to a valid object.
    template <typename T>
    class ReferenceCountedHandle
    {
      public:
        ReferenceCountedHandle();
        ReferenceCountedHandle(const ReferenceCountedHandle<T>& other);               // copy ctor
        ReferenceCountedHandle<T>& operator=(const ReferenceCountedHandle<T>& other); // copy assignment
        ReferenceCountedHandle(ReferenceCountedHandle<T>&& other);                    // move ctor
        ReferenceCountedHandle<T>& operator=(ReferenceCountedHandle<T>&& other);      // move assignment
        ~ReferenceCountedHandle();

        T* resource = nullptr;
        StorageId_t id = 0;

        T* operator->() { return resource; }
        const T* operator->() const { return resource; }
        T& operator*() { return *resource; }
        const T& operator*() const { return *resource; }

        bool IsValid() { return id != INVALID_RESOURCE_ID; }

      private:
        ReferenceCountedHandle(T& resource, StorageId_t id, ResourceStorage<T>& owning_storage);

        // we keep track of this within the ctor and dtor of the handle. Once it reaches 0 in the dtor, we
        // notify the storage so it can be removed. Stored directly instead of lookup every time for
        // performance.
        ReferenceCount_t* ref_counter;
        ResourceStorage<T>* owning_storage;

        friend struct ResourceStorage<T>; // so the storage can create handle using private ctor.
    };

    /// Class that is responsible for identifying and managing the lifetime of resources of type T
    template <typename T>
    struct ResourceStorage
    {
        // Map where we store the resource objects.
        std::map<StorageId_t, T> resource_map{};

        // Map where we keep track of the references to the resource objects. Managed with
        // ReferenceCountedHandle.
        std::map<StorageId_t, ReferenceCount_t> resource_reference_map{};

        // #TODO replace this with interned strings
        std::map<StorageId_t, std::string> resource_name_map{};

        // When the resources are "destroyed" through the RAII usage of handles, they are put into this list.
        // Then the engine goes through it and does the necessary operations.
        std::vector<T> destroy_pending_resources{};

        StorageId_t next_storage_id = 1; // we assume 0 is invalid for easier debugging.
        bool destroyed = false;          // flag enabled after storage is cleared.

        /// Add a resource to the storage. This will start tracking the resource and create the first
        /// reference counted handle. Resource is just copied in because the resource types are meant to be
        /// POD structures.
        ReferenceCountedHandle<T> AddResource(const T& resource, std::string_view name = "unnamed_resource")
        {
            StorageId_t id = next_storage_id++;
            resource_map[id] = resource;
            resource_name_map[id] = name;
            resource_reference_map[id] = 0;

            return ReferenceCountedHandle<T>(resource_map[id], id, *this);
        }

        /// Same as other AddResource but move version.
        ReferenceCountedHandle<T> AddResource(T&& resource, std::string_view name = "unnamed_resource")
        {
            StorageId_t id = next_storage_id++;
            resource_map[id] = std::move(resource);
            resource_name_map[id] = name;
            resource_reference_map[id] = 0;

            return ReferenceCountedHandle<T>(resource_map[id], id, *this);
        }

        /// Mark the given resource for destruction. NEVER call this directly, ReferenceCountedHandle<T> does
        /// it automatically already.
        void MarkForDestruction(T* resource, StorageId_t resource_id)
        {
            // simply copy the resource into destroys without a lookup.
            destroy_pending_resources.emplace_back(*resource);

            resource_map.erase(resource_id);
            resource_reference_map.erase(resource_id);

            // maybe we could keep this around for debugging deleted resources?
            resource_name_map.erase(resource_id);
        }

        void DestroyResource(VulkanEngine& engine, const T& resource);

        void DestroyPendingResources(VulkanEngine& engine)
        {
            for (T& resource : destroy_pending_resources)
            {
                DestroyResource(engine, resource);
            }
            destroy_pending_resources.clear();
        }

        ReferenceCountedHandle<T> HandleFromID(StorageId_t id)
        {
            if (resource_map.contains(id) == false)
            {
                return ReferenceCountedHandle<T>{}; // invalid
            }

            return ReferenceCountedHandle<T>(resource_map[id], id, *this);
        }

        /// Instantly destroy all resources in the storage
        void Clear(VulkanEngine& engine)
        {
            // destroy active resources and then the ones already pending destruction
            for (auto [id, resource] : resource_map)
            {
                DestroyResource(engine, resource);
            }

            DestroyPendingResources(engine);

            resource_map.clear();
            resource_reference_map.clear();
            resource_name_map.clear();

            destroyed = true;
        }
    };

    template <typename T>
    ReferenceCountedHandle<T>::ReferenceCountedHandle(
        T& _resource, StorageId_t _id, ResourceStorage<T>& _owning_storage
    ) :
        resource(&_resource),
        id(_id),
        owning_storage(&_owning_storage)
    {
        ref_counter = &owning_storage->resource_reference_map[id];
        // increment once
        ++(*ref_counter);
    }

    template <typename T>
    ReferenceCountedHandle<T>::ReferenceCountedHandle() :
        resource(nullptr),
        id(INVALID_RESOURCE_ID),
        ref_counter(nullptr),
        owning_storage(nullptr)
    {
    }

    template <typename T>
    ReferenceCountedHandle<T>::ReferenceCountedHandle(const ReferenceCountedHandle<T>& other)
    {
        *this = other;
    }

    template <typename T>
    ReferenceCountedHandle<T>& ReferenceCountedHandle<T>::operator=(const ReferenceCountedHandle<T>& other)
    {
        resource = other.resource;
        id = other.id;
        ref_counter = other.ref_counter;
        owning_storage = other.owning_storage;

        if (IsValid() && owning_storage->destroyed == false)
        {
            // new handle means counter go up
            ++(*ref_counter);
        }

        return *this;
    }

    template <typename T>
    ReferenceCountedHandle<T>::ReferenceCountedHandle(ReferenceCountedHandle<T>&& other)
    {
        *this = std::move(other);
    }

    template <typename T>
    ReferenceCountedHandle<T>& ReferenceCountedHandle<T>::operator=(ReferenceCountedHandle<T>&& other)
    {
        resource = std::move(other.resource);
        id = std::move(other.id);
        ref_counter = std::move(other.ref_counter);
        owning_storage = std::move(other.owning_storage);

        // prevent the other object from deletusing the resource when going out of scope.
        other.resource = nullptr;
        other.id = INVALID_RESOURCE_ID;
        other.owning_storage = nullptr;
        other.ref_counter = nullptr;

        return *this;
    }

    template <typename T>
    ReferenceCountedHandle<T>::~ReferenceCountedHandle()
    {
        // storage might have been destroyed. In that case, this is a dead handle anyway and ref_counter is
        // dangling.
        if (IsValid() && owning_storage->destroyed == false)
        {
            // handle just for deleted meaning the counter goes down. If 0, it's deletus time
            --(*ref_counter);
            if (*ref_counter < 0)
            {
                // this should be impossible with copy ctor and dtor. Investigate what went wrong.
                abort();
            }

            if (*ref_counter == 0)
            {
                owning_storage->MarkForDestruction(resource, id);
            }
        }
    }

    template <typename T>
    inline void ResourceStorage<T>::DestroyResource(VulkanEngine&, const T&)
    {
        static_assert(
            std::false_type::value,
            "DestroyResource was not implemented for a type that has a ResourceStorage. All types that "
            "are contained by a ResourceStorage<T> need to have their destroy function defined "
            "with template specialisation."
        );
    }
} // namespace Renderer