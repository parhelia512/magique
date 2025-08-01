// SPDX-License-Identifier: zlib-acknowledgement
#include <raylib/raylib.h>

#include <magique/core/Core.h>
#include <magique/ecs/ECS.h>
#include <magique/ecs/Scripting.h>
#include <magique/ecs/Components.h>
#include <magique/util/Logging.h>
#include <magique/core/Animations.h>

#include "internal/globals/ECSData.h"
#include "internal/globals/EngineData.h"
#include "internal/globals/EngineConfig.h"
#include "internal/globals/DynamicCollisionData.h"
#include "internal/globals/PathFindingData.h"
#include "internal/globals/ScriptData.h"
#include "internal/utils/STLUtil.h"

namespace magique
{
    bool RegisterEntity(const EntityType type, const CreateFunc& createFunc)
    {
        MAGIQUE_ASSERT(type < static_cast<EntityType>(UINT16_MAX), "Max value is reserved!");
        auto& map = global::ECS_DATA.typeMap;
        if (map.contains(type))
            LOG_WARNING("Overriding existing create function for entity: %d (enum value)", static_cast<int>(type));
        map[type] = createFunc;

        global::SCRIPT_DATA.padUpToEntity(type); // This assures it's always valid to index with type
        // Iterates all entities
        for (auto entity : internal::REGISTRY.view<entt::entity>())
        {
            volatile int b = static_cast<int>(entity); // Try to instantiate all storage types - even in release mode
            (void)b;                                   // Suppress unused variable
        }
        return true;
    }

    bool UnRegisterEntity(const EntityType type)
    {
        MAGIQUE_ASSERT(type < static_cast<EntityType>(UINT16_MAX), "Max value is reserved!");
        auto& map = global::ECS_DATA.typeMap;
        if (type == static_cast<EntityType>(UINT16_MAX) || !map.contains(type))
        {
            return false; // Invalid ID or not registered
        }
        map.erase(type);
        return true;
    }

    bool EntityExists(const entt::entity e) { return internal::REGISTRY.valid(e); }

    bool EntityIsActor(const entt::entity entity) { return internal::REGISTRY.all_of<ActorC>(entity); }

    entt::entity GetEntity(const EntityType type)
    {
        for (const auto entity : internal::REGISTRY.view<entt::entity>())
        {
            const auto& pos = internal::REGISTRY.get<const PositionC>(entity);
            if (pos.type == type)
            {
                return entity;
            }
        }
        return entt::null;
    }

    static entt::entity CreateEntityInternal(const entt::entity id, EntityType type, float x, float y, const MapID map,
                                             const int rotation, const bool withFunc)
    {
        MAGIQUE_ASSERT(type < static_cast<EntityType>(UINT16_MAX), "Max value is reserved!");
        const auto& config = global::ENGINE_CONFIG;
        auto& ecs = global::ECS_DATA;
        auto& data = global::ENGINE_DATA;
        auto& registry = internal::REGISTRY;

        const auto entity = registry.create(id != entt::null ? id : entt::entity{ecs.entityID++});
        registry.emplace<PositionC>(entity, x, y, map, type, static_cast<uint16_t>(rotation)); // PositionC is default

        if (withFunc) [[likely]]
        {
            const auto it = ecs.typeMap.find(type);
            if (it == ecs.typeMap.end())
            {
                LOG_ERROR("No method create method registered for that entity type!");
                return entt::null; // EntityType not registered
            }
            it->second(entity, type);
        }

        if (!config.isClientMode && data.isEntityScripted(entity)) [[likely]]
        {
            InvokeEvent<onCreate>(entity);
        }
        if (registry.all_of<CameraC>(entity)) [[unlikely]]
        {
            data.cameraEntity = entity;
            data.cameraMap = map;
        }

        // Creating entities shouldn't happen that often - like this we can avoid the checks in tighter methods
        auto& pathData = global::PATH_DATA;
        if (!pathData.mapsStaticGrids.contains(map))
        {
            pathData.mapsStaticGrids.add(map);
        }
        if (!pathData.mapsDynamicGrids.contains(map))
        {
            pathData.mapsDynamicGrids.add(map);
        }
        if (!global::DY_COLL_DATA.mapEntityGrids.contains(map))
        {
            global::DY_COLL_DATA.mapEntityGrids.add(map);
        }

        return entity;
    }

    entt::entity CreateEntity(const EntityType type, const float x, const float y, const MapID map, int rotation,
                              const bool withFunc)
    {
        return CreateEntityInternal(entt::null, type, x, y, map, rotation, withFunc);
    }

    entt::entity CreateEntityEx(const entt::entity id, const EntityType type, const float x, const float y,
                                const MapID map, const int rot, const bool withFunc)
    {
        MAGIQUE_ASSERT(!EntityExists(id), "Entity already exists!");
        return CreateEntityInternal(id, type, x, y, map, rot, withFunc);
    }

    bool DestroyEntity(const entt::entity entity)
    {
        const auto& config = global::ENGINE_CONFIG;
        auto& data = global::ENGINE_DATA;
        auto& dynamic = global::DY_COLL_DATA;
        auto& registry = internal::REGISTRY;
        if (registry.valid(entity)) [[likely]]
        {
            const auto& pos = internal::POSITION_GROUP.get<const PositionC>(entity);
            if (data.destroyEntityCallback)
            {
                data.destroyEntityCallback(entity, pos);
            }
            if (!config.isClientMode && data.isEntityScripted(entity)) [[likely]]
            {
                InvokeEventDirect<onDestroy>(global::SCRIPT_DATA.scripts[pos.type], entity);
            }

            registry.destroy(entity);
            data.entityUpdateCache.erase(entity);
            UnorderedDelete(data.drawVec, entity);
            UnorderedDelete(data.entityUpdateVec, entity);
            UnorderedDelete(data.collisionVec, entity);
            data.entityNScriptedSet.erase(entity);
            if (dynamic.mapEntityGrids.contains(pos.map)) [[likely]]
            {
                dynamic.mapEntityGrids[pos.map].removeWithHoles(entity);
            }
            global::PATH_DATA.solidEntities.erase(entity);
            if (entity == GetCameraEntity())
            {
                data.cameraEntity = entt::entity{UINT32_MAX};
            }
            return true;
        }
        return false;
    }

    void DestroyEntities(const std::initializer_list<EntityType>& ids)
    {
        const auto& config = global::ENGINE_CONFIG;
        auto& data = global::ENGINE_DATA;
        auto& dyCollData = global::DY_COLL_DATA;

        const auto& group = internal::POSITION_GROUP; // Get all entities
        if (ids.size() == 0)
        {
            for (const auto e : group)
            {
                const auto& pos = group.get<PositionC>(e);
                if (data.destroyEntityCallback)
                {
                    data.destroyEntityCallback(e, pos);
                }
                if (!config.isClientMode && data.isEntityScripted(e)) [[likely]]
                {
                    InvokeEventDirect<onDestroy>(global::SCRIPT_DATA.scripts[pos.type], e);
                }
            }

            internal::REGISTRY.clear();
            data.entityUpdateCache.clear();
            data.drawVec.clear();
            data.entityUpdateVec.clear();
            data.collisionVec.clear();
            data.entityNScriptedSet.clear();
            dyCollData.mapEntityGrids.clear();
            data.cameraEntity = entt::entity{UINT32_MAX};
            global::PATH_DATA.solidEntities.clear();
            return;
        }

        for (const auto e : group)
        {
            const auto& pos = group.get<PositionC>(e);
            for (const auto id : ids)
            {
                if (pos.type == id)
                {
                    DestroyEntity(e);
                    break;
                }
            }
        }
        // Don't need to patch as its cleared each tick
    }

    void SetDestroyEntityCallback(const DestroyEntityCallback callback)
    {
        global::ENGINE_DATA.destroyEntityCallback = callback;
    }

    CollisionC& GiveCollisionRect(const entt::entity e, const float width, const float height, const int anchorX,
                                  const int anchorY)
    {
        return internal::REGISTRY.emplace<CollisionC>(e, width, height, 0.0F, 0.0F, 1.0F, static_cast<int16_t>(anchorX),
                                                      static_cast<int16_t>(anchorY), Shape::RECT,
                                                      CollisionLayer::DEFAULT_LAYER);
    }

    CollisionC& GiveCollisionCircle(const entt::entity e, const float radius)
    {
        return internal::REGISTRY.emplace<CollisionC>(e, radius, radius, 0.0F, 0.0F, 1.0F, static_cast<int16_t>(0),
                                                      static_cast<int16_t>(0), Shape::CIRCLE,
                                                      CollisionLayer::DEFAULT_LAYER, CollisionLayer::DEFAULT_LAYER);
    }

    CollisionC& GiveCollisionCapsule(const entt::entity e, const float height, const float radius)
    {
        MAGIQUE_ASSERT(height > 2 * radius,
                       "Given capsule is not well defined! Total height has to be greater than 2 * radius");
        return internal::REGISTRY.emplace<CollisionC>(e, radius, height, 0.0F, 0.0F, 1.0F, static_cast<int16_t>(0),
                                                      static_cast<int16_t>(0), Shape::CAPSULE,
                                                      CollisionLayer::DEFAULT_LAYER, CollisionLayer::DEFAULT_LAYER);
    }

    CollisionC& GiveCollisionTri(const entt::entity e, const Point p2, const Point p3, const int anchorX,
                                 const int anchorY)
    {
        return internal::REGISTRY.emplace<CollisionC>(e, p2.x, p2.y, p3.x, p3.y, 1.0F, static_cast<int16_t>(anchorX),
                                                      static_cast<int16_t>(anchorY), Shape::TRIANGLE,
                                                      CollisionLayer::DEFAULT_LAYER, CollisionLayer::DEFAULT_LAYER);
    }

    AnimationC& GiveAnimation(const entt::entity entity, const EntityType type, const AnimationState startState)
    {
        const auto& animation = GetEntityAnimation(type);
        return internal::REGISTRY.emplace<AnimationC>(entity, animation, startState);
    }

    void GiveCamera(const entt::entity entity)
    {
        internal::REGISTRY.emplace<CameraC>(entity);
        global::ENGINE_DATA.cameraMap = GetComponent<const PositionC>(entity).map;
    }

    OccluderC& GiveOccluder(const entt::entity entity, const int width, const int height, const Shape shape)
    {
        return internal::REGISTRY.emplace<OccluderC>(entity, static_cast<int16_t>(width), static_cast<int16_t>(height),
                                                     shape);
    }

    EmitterC& GiveEmitter(const entt::entity entity, const Color color, const int intensity, LightStyle style)
    {
        return internal::REGISTRY.emplace<EmitterC>(entity, color.r, color.g, color.b, color.a,
                                                    static_cast<uint16_t>(intensity), style);
    }

    void GiveActor(const entt::entity e) { internal::REGISTRY.emplace<ActorC>(e); }

    //----------------- CORE -----------------//

    void SetCameraEntity(const entt::entity e)
    {
        auto& reg = internal::REGISTRY;
        if (!reg.valid(e))
        {
            LOG_WARNING("Trying to assign camera to invalid entity: %d", static_cast<int>(e));
            return;
        }
        const auto view = reg.view<CameraC>();
        bool found = false;
        for (const auto cam : view)
        {
            if (cam == e)
            {
                LOG_WARNING("Target entity is already the camera holder!");
                return;
            }
            reg.erase<CameraC>(cam);
            found = true;
        }
        if (found)
        {
            MAGIQUE_ASSERT(!reg.any_of<CameraC>(e), "Target entity cannot have camera component already!");
            reg.emplace<CameraC>(e);
        }
        else
            LOG_ERROR("No existing entity with a camera component found!");
    }

    const std::vector<entt::entity>& GetNearbyEntities(const MapID map, const Point origin, const float radius)
    {
        // This works because the hashset uses a vector as underlying storage type (stored without holes)
        const auto& dynamicData = global::DY_COLL_DATA;
        auto& data = global::ENGINE_DATA;

        // For security - user might call this before creating any entities
        if (!dynamicData.mapEntityGrids.contains(map)) [[unlikely]]
            return data.nearbyQueryData.cache.values();

        if (data.nearbyQueryData.getIsSimilarParameters(map, origin, radius))
            return data.nearbyQueryData.cache.values();

        data.nearbyQueryData.lastRadius = radius;
        data.nearbyQueryData.lastOrigin = origin;
        data.nearbyQueryData.cache.clear();

        const auto queryX = origin.x - radius;
        const auto queryY = origin.y - radius;
        dynamicData.mapEntityGrids[map].query(data.nearbyQueryData.cache, queryX, queryY, radius * 2.0F, radius * 2.0F);
        return data.nearbyQueryData.cache.values();
    }

    bool NearbyEntitiesContain(const MapID map, const Point origin, const float radius, const entt::entity target)
    {
        auto& data = global::ENGINE_DATA;
        if (data.nearbyQueryData.getIsSimilarParameters(map, origin, radius))
            return data.nearbyQueryData.cache.contains(target);
        GetNearbyEntities(map, origin, radius);
        return data.nearbyQueryData.cache.contains(target);
    }


} // namespace magique