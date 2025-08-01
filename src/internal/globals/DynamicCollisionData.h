// SPDX-License-Identifier: zlib-acknowledgement
#ifndef MAGIQUE_DYNAMIC_COLLISION_DATA_H
#define MAGIQUE_DYNAMIC_COLLISION_DATA_H

#include <magique/core/Types.h>

#include "internal/datastructures/HashTypes.h"
#include "internal/datastructures/VectorType.h"
#include "internal/datastructures/MultiResolutionGrid.h"

namespace magique
{
    struct PairInfo final // Saves entity id and type
    {
        CollisionInfo info;
        entt::entity e1;
        entt::entity e2;
    };

    using CollPairCollector = AlignedVec<PairInfo>[MAGIQUE_WORKER_THREADS + 1];
    using EntityCollector = AlignedVec<entt::entity>[MAGIQUE_WORKER_THREADS + 1];
    using EntityHashGrid =
        SingleResolutionHashGrid<entt::entity, MAGIQUE_MAX_ENTITIES_CELL, MAGIQUE_COLLISION_CELL_SIZE>;

    struct DynamicCollisionData final
    {
        MapHolder<EntityHashGrid> mapEntityGrids{}; // Separate hashgrid for each map
        HashSet<uint64_t> pairSet;                  // Filters unique collision pairs
        CollPairCollector collisionPairs{};         // Collision pair collectors

        DynamicCollisionData() { pairSet.reserve(1000); }

        bool isMarked(entt::entity e1, uint32_t e2)
        {
            const auto num = (static_cast<uint64_t>(e1) << 32) | e2;
            const auto it = pairSet.find(num);
            if (it == pairSet.end())
            {
                pairSet.insert(it, num);
                return false;
            }
            return true;
        }
    };

    namespace global
    {
        inline DynamicCollisionData DY_COLL_DATA{};
    }
} // namespace magique

#endif //MAGIQUE_DYNAMIC_COLLISION_DATA_H