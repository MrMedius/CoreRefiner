#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include "ObjectBase.h"

// Registry + pool for gameplay ObjectBase instances.
// Semantics:
// - SpawnPooled: Ball / Enemy_T etc. — reuse inactive instances; expect RequestDisable/Deactivate recycle.
// - AcquirePersistent: Player / Field etc. — long-lived; scene leave should Reset/Activate, not ClearTag.
// - Acquire / AcquireDeactive: legacy names kept as thin aliases.
// - Handoff / TakeHandoff: 下层生成、没有父物体的对象先按自己的标签交出。管理器按基类取走，不按具体子类。
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

    // Spawn or reuse a pooled instance and Activate it.
    template <typename T, typename... Args>
    static T* SpawnPooled(Object_Type_Tag tag, Args&&... args)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        return Get_().Acquire_<T>(tag, true, std::forward<Args>(args)...);
    }

    // Create or reuse without Activate (pool warmup).
    template <typename T, typename... Args>
    static T* SpawnPooledDeactive(Object_Type_Tag tag, Args&&... args)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        return Get_().Acquire_<T>(tag, false, std::forward<Args>(args)...);
    }

    // Persistent instance path (Player / Field): same storage, different intent.
    // Do not ClearTag these on scene leave; call Activate / host OnEnable to reset.
    template <typename T, typename... Args>
    static T* AcquirePersistent(Object_Type_Tag tag, Args&&... args)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        return Get_().Acquire_<T>(tag, true, std::forward<Args>(args)...);
    }

    // Legacy alias of SpawnPooled.
    template <typename T, typename... Args>
    static T* Acquire(Object_Type_Tag tag, Args&&... args)
    {
        return SpawnPooled<T>(tag, std::forward<Args>(args)...);
    }
    // Legacy alias of SpawnPooledDeactive.
    template <typename T, typename... Args>
    static T* AcquireDeactive(Object_Type_Tag tag, Args&&... args)
    {
        return SpawnPooledDeactive<T>(tag, std::forward<Args>(args)...);
    }

    template <typename T>
    static void Push(Object_Type_Tag tag, T* object)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        Get_().Push_<T>(tag, object);
    }

    template <typename T>
    static std::vector<T*> FindActiveObjectsByTag(Object_Type_Tag tag)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        return Get_().FindActiveObjectsByTag_<T>(tag);
    }

    template <typename T>
    static T* FindFirstActiveObjectByTag(Object_Type_Tag tag)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        return Get_().FindFirstActiveObjectByTag_<T>(tag);
    }

    template <typename T>
    static std::vector<T*> FindObjectsByTag(Object_Type_Tag tag)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        return Get_().FindObjectsByTag_<T>(tag);
    }

    template <typename T>
    static T* FindFirstObjectByTag(Object_Type_Tag tag)
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        return Get_().FindFirstObjectByTag_<T>(tag);
    }

    // 把没有父物体的对象放进这个标签的待认领名单。
    // 空指针、已有父物体、标签对不上、或已经在这份名单里的，不放。
    static void Handoff(Object_Type_Tag tag, ObjectBase* obj)
    {
        Get_().Handoff_(tag, obj);
    }

    // 从所有标签的待认领名单里取走 T 及其子类。其它对象留在原名单。
    template <typename T>
    static std::vector<T*> TakeHandoff()
    {
        static_assert(std::is_base_of<ObjectBase, T>::value, "T must inherit from ObjectBase");
        return Get_().TakeHandoff_<T>();
    }

    // Destroy all instances of a tag (rare; not used for normal scene leave).
    // 同时清掉这个标签还没认领的地址，避免对象销毁后留下失效指针。
    static void ClearTag(Object_Type_Tag tag)
    {
        Get_().objectPools.erase(tag);
        Get_().handoff_.erase(tag);
    }

    // Destroy every pooled/persistent object (shutdown only; not scene leave).
    // 待认领名单一起清掉。
    static void ClearAll()
    {
        Get_().objectPools.clear();
        Get_().handoff_.clear();
    }

private:
    template <typename T, typename... Args>
    T* Acquire_(Object_Type_Tag tag, bool activate, Args&&... args)
    {
        auto& pool = objectPools[tag]; // create the vector, if it doesn't exist.

        // Find  a reusable object
        for (auto& obj : pool)
        {
            if (obj->IsReusable())
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

    // 放入待认领名单。不拥有对象，对象仍留在 objectPools 里。
    void Handoff_(Object_Type_Tag tag, ObjectBase* obj)
    {
        if (obj == nullptr || obj->GetParent() != nullptr || obj->GetTag() != tag)
        {
            return;
        }
        std::vector<ObjectBase*>& pending = handoff_[tag];
        for (ObjectBase* existing : pending)
        {
            if (existing == obj)
            {
                return;
            }
        }
        pending.push_back(obj);
    }

    // 取出所有能转成 T 的待认领对象。空了的标签名单删掉。
    template <typename T>
    std::vector<T*> TakeHandoff_()
    {
        std::vector<T*> result;
        for (auto it = handoff_.begin(); it != handoff_.end(); )
        {
            std::vector<ObjectBase*>& pending = it->second;
            for (int i = 0; i < static_cast<int>(pending.size()); ++i)
            {
                if (T* typed = dynamic_cast<T*>(pending[static_cast<std::size_t>(i)]))
                {
                    result.push_back(typed);
                    pending.erase(pending.begin() + i);
                    --i;
                }
            }
            if (pending.empty())
            {
                it = handoff_.erase(it);
            }
            else
            {
                ++it;
            }
        }
        return result;
    }

private:
    // 对象池。按标签保存实例，并拥有它们。
    std::unordered_map<Object_Type_Tag, std::vector<std::unique_ptr<ObjectBase>>> objectPools;
    // 待认领名单。按标签暂存还没被管理器取走的对象，不拥有它们。
    std::unordered_map<Object_Type_Tag, std::vector<ObjectBase*>> handoff_;
};
