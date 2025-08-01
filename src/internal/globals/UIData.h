// SPDX-License-Identifier: zlib-acknowledgement
#ifndef MAGIQUE_UI_DATA_H
#define MAGIQUE_UI_DATA_H

#include <raylib/raylib.h>

#include <magique/ui/UIObject.h>
#include <magique/util/Logging.h>

#include "internal/datastructures/VectorType.h"
#include "internal/datastructures/HashTypes.h"
#include "internal/utils/STLUtil.h"

inline bool initialized = false;

namespace magique
{
    struct UIData final
    {
        vector<UIObject*> objects;
        vector<UIObject*> containers;
        HashSet<UIObject*> objectsSet;
        Point dragStart{-1, -1};
        Point targetRes{1280, 720};
        Point sourceRes{1920, 1080};
        Point mouse{0, 0};
        Point scaling{1.0F, 1.0F};
        bool inputConsumed = false;

        UIData()
        {
            initialized = true;
        }

        // Called at the end of the update tick
        void update()
        {
            // Using fori to support deletions in the update methods
            for (int i = 0; i < containers.size(); ++i)
            {
                auto& container = *containers[i];
                container.onUpdate(container.getBounds(), container.wasDrawnLastTick);
            }

            for (int i = 0; i < objects.size(); ++i)
            {
                auto& obj = *objects[i];
                obj.onUpdate(obj.getBounds(), obj.wasDrawnLastTick);
            }
        }

        void updateDrawTick()
        {
            if (targetRes == 0.0F)
            {
                targetRes.x = static_cast<float>(GetScreenWidth());
                targetRes.x = static_cast<float>(GetScreenHeight());
            }
            scaling = targetRes / sourceRes;
            const auto [mx, my] = GetMousePosition();
            mouse = {mx, my};
            inputConsumed = false;

            if (dragStart.x == -1 && dragStart.y == -1 && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                dragStart = {mx, my};
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                dragStart = {-1, -1};
            }

            // Using fori to support deletions in the update methods
            for (int i = 0; i < containers.size(); ++i)
            {
                auto& container = *containers[i];
                container.onDrawUpdate(container.getBounds());
                container.wasDrawnLastTick = false;
            }
            for (int i = 0; i < objects.size(); ++i)
            {
                auto& obj = *objects[i];
                obj.onDrawUpdate(obj.getBounds());
                obj.wasDrawnLastTick = false;
            }
        }

        // All objects are registered in their ctor
        void registerObject(UIObject* object, const bool isContainer = false)
        {
            if (!initialized)
            {
                *this = {};
                initialized = true;
            }

            objectsSet.insert(object);
            if (isContainer)
            {
                object->isContainer = true;
                containers.push_back(object);
                objects.pop_back(); // Is added as an object as well
            }
            else
                objects.push_back(object);
        }

        // All objects are un-registered in the dtor
        void unregisterObject(const UIObject* object) { UnorderedDelete(objects, object); }

        void registerDrawCall(UIObject* object, const bool isContainer)
        {
            object->wasDrawnLastTick = true;
            if (!objectsSet.contains(object)) [[unlikely]]
                registerObject(object, object->isContainer);

            // Moving 3 to the front
            // [1][2][3][4][5]
            // [3]              -> assign temporary
            // [0][1][2][4][5]  -> copy everything before 1 back
            // [3][1][2][4][5]
            // or using std::iterators ... :(

            auto sortUpfront = [](vector<UIObject*>& objects, UIObject* obj)
            {
                auto* it = std::ranges::find(objects, obj);
                objects.erase(it);
                objects.insert(objects.begin(), obj);
            };

            if (isContainer)
            {
                sortUpfront(containers, object);
            }
            else
            {
                sortUpfront(objects, object);
            }
        }
    };

    namespace global
    {
        inline UIData UI_DATA{};
    }
} // namespace magique

#endif //MAGIQUE_UI_DATA_H