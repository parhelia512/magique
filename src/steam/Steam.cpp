// SPDX-License-Identifier: zlib-acknowledgement
#define _CRT_SECURE_NO_WARNINGS
#include <fstream>

#include <magique/steam/Steam.h>
#include <magique/util/Logging.h>
#include <magique/internal/Macros.h>

#include "internal/globals/SteamData.h"

namespace magique
{

    static void SteamDebugCallback(int isWarning, const char* text)
    {
        if (isWarning == 1)
        {
            LOG_WARNING("Steam: %s", text);
        }
        else
        {
            LOG_INFO("Steam: %s", text);
        }
    }

    bool InitSteam(const bool createFile)
    {
        auto& steamData = global::STEAM_DATA;
        if (createFile)
        {
            constexpr auto* filename = "steam_appid.txt";
            constexpr auto* id = "480";

            const std::ifstream file(filename);
            if (!file.good())
            {
                const auto newFile = fopen(filename, "wb");
                if (newFile != nullptr)
                {
                    fwrite(id, 3, 1, newFile);
                    fclose(newFile);
                    LOG_INFO("Created steam_appid.txt with test id 480");
                }
                else
                {
                    LOG_ERROR("Unable to create steam_appid.txt file with test id 480 - Do it manually!");
                }
            }
        }

        MAGIQUE_ASSERT(steamData.isInitialized == false, "Steam already has been initialized");
        SteamErrMsg errMsg;
        if (SteamAPI_InitEx(&errMsg) != k_ESteamAPIInitResult_OK)
        {
            LOG_ERROR(errMsg);
            SteamAPI_Shutdown();
            return false;
        }

        SteamUtils()->SetWarningMessageHook(SteamDebugCallback);
        steamData.init();

        // Cache the steam id - will stay the same
        const auto id = SteamUser()->GetSteamID();
        memcpy(&steamData.userID, &id, sizeof(id));

        steamData.isInitialized = true;
        LOG_INFO("Successfully initialized steam");
        return true;
    }

    SteamID GetUserSteamID()
    {
        auto& steamData = global::STEAM_DATA;
        MAGIQUE_ASSERT(steamData.isInitialized, "Steam is not initialized");
        return static_cast<SteamID>(steamData.userID.ConvertToUint64());
    }

    const char* GetSteamUserName() { return SteamFriends()->GetPersonaName(); }

    void SetSteamOverlayCallback(SteamOverlayCallback steamOverlayCallback)
    {
        global::STEAM_DATA.overlayCallback = steamOverlayCallback;
    }

    static char TEMP[256]{};

    const char* GetSteamUserDataLocation()
    {
        MAGIQUE_ASSERT(global::STEAM_DATA.isInitialized, "Steam is not initialized");
        SteamUser()->GetUserDataFolder(TEMP, 512);
        return TEMP;
    }

} // namespace magique