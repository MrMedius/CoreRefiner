#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include "ObjectBase.h"

class ObjectCodex
{
private:
    ObjectCodex() = default;
    ~ObjectCodex() = default;
    static ObjectCodex& Get_() noexcept
    {
        static ObjectCodex inst;
        return inst;
    }
public:
    ObjectCodex(const ObjectCodex&) = delete;
    ObjectCodex& operator=(const ObjectCodex&) = delete;

    // GetObject
    template <typename T, typename... Args>
    static T* Acquire(Object_Type_Tag tag, Args&&... args)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from Object");
        return Get_().Acquire_<T>(tag, true, std::forward<Args>(args)...);
    }
    template <typename T, typename... Args>
    static T* AcquireDeactive(Object_Type_Tag tag, Args&&... args)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from Object");
        return Get_().Acquire_<T>(tag, false, std::forward<Args>(args)...);
    }

    template <typename T>
    static void Push(Object_Type_Tag tag, T* object)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from Object");
        Get_().Push_<T>(tag, object);
    }

    template <typename T>
    static std::vector<T*> FindActiveObjectsByTag(Object_Type_Tag tag)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from Object");
        return Get_().FindActiveObjectsByTag_<T>(tag);
    }

    template <typename T>
    static T* FindFirstActiveObjectByTag(Object_Type_Tag tag)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from Object");
        return Get_().FindFirstActiveObjectByTag_<T>(tag);
    }

    template <typename T>
    static std::vector<T*> FindObjectsByTag(Object_Type_Tag tag)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from Object");
        return Get_().FindObjectsByTag_<T>(tag);
    }

    template <typename T>
    static T* FindFirstObjectByTag(Object_Type_Tag tag)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from Object");
        return Get_().FindFirstObjectByTag_<T>(tag);
    }

    static void ClearTag(Object_Type_Tag tag)
    {
        Get_().objectPools.erase(tag);
    }

    static void ClearAll()
    {
        Get_().objectPools.clear();
    }

private:
    template <typename T, typename... Args>
    T* Acquire_(Object_Type_Tag tag, bool activate, Args&&... args)
    {
        auto& pool = objectPools[tag]; // create the vector, if it doesn't exist.

        // Find  a reusable object
        for (auto& obj : pool)
        {
            if (!obj->IsActive())
            {
                // Type matching is required for use
                if (auto* typed = dynamic_cast<T*>(obj.get()))
                {
                    if (activate) obj->Activate();

                    return typed;
                }
            }
        }

        // No available => Create a new object
        auto newObj = std::make_unique<T>(std::forward<Args>(args)...);
        auto* ret = newObj.get();
        pool.push_back(std::move(newObj));
        return ret;
    }

    template <typename T>
    void Push_(Object_Type_Tag tag, T* object)
    {
        objectPools[tag].push_back(std::unique_ptr<ObjectBase>(object));
    }

    template <typename T>
    std::vector<T*> FindActiveObjectsByTag_(Object_Type_Tag tag)
    {
        std::vector<T*> result;

        auto it = objectPools.find(tag);

        if (it == objectPools.end()) 
            return result;

        for (auto& obj : it->second)
        {
            if (obj->IsActive())
            {
                if (auto* typed = dynamic_cast<T*>(obj.get()))
                {
                    result.push_back(typed);
                }
            }
        }
        return result;
    }

    template <typename T>
    T* FindFirstActiveObjectByTag_(Object_Type_Tag tag)
    {
        auto it = objectPools.find(tag);

        if (it == objectPools.end()) 
            return nullptr;

        for (auto& obj : it->second)
        {
            if (obj->IsActive())
            {
                if (auto* typed = dynamic_cast<T*>(obj.get()))
                {
                    return typed;
                }
            }
        }
        return nullptr;
    }

    template <typename T>
    std::vector<T*> FindObjectsByTag_(Object_Type_Tag tag)
    {
        std::vector<T*> result;

        auto it = objectPools.find(tag);

        if (it == objectPools.end())
            return result;

        for (auto& obj : it->second)
        {
            if (auto* typed = dynamic_cast<T*>(obj.get()))
            {
                result.push_back(typed);
            }
        }
        return result;
    }

    template <typename T>
    T* FindFirstObjectByTag_(Object_Type_Tag tag)
    {
        auto it = objectPools.find(tag);

        if (it == objectPools.end())
            return nullptr;

        for (auto& obj : it->second)
        {
            if (auto* typed = dynamic_cast<T*>(obj.get()))
            {
                return typed;
            }
        }
        return nullptr;
    }

private:
    std::unordered_map<Object_Type_Tag, std::vector<std::unique_ptr<ObjectBase>>> objectPools;
};