// SPDX-License-Identifier: zlib-acknowledgement
#ifndef MAGIQUE_PERFDATA_H
#define MAGIQUE_PERFDATA_H

#include "internal/datastructures/VectorType.h"
#include "external/raylib-compat/rlgl_compat.h"

#include "internal/utils/OSUtil.h"
#if defined(MAGIQUE_LAN) || defined(MAGIQUE_STEAM)
#include "internal/globals/MultiplayerData.h"
#endif

namespace magique
{
    struct PerformanceBlock final
    {
        float width = 0;
        char text[20]{};
    };

    enum TickType
    {
        UPDATE,
        DRAW
    };

    struct PerformanceData final
    {
        uint32_t logicTickTime = 0;
        uint32_t drawTickTime = 0;
        int tickCounter = 0;
        int updateDelayTicks = 15;
        PerformanceBlock blocks[7]{}; // 7 blocks for FPS, CPU, GPU, DrawCalls, Upload, Download, Ping

#if MAGIQUE_PROFILING == 1
        vector<uint32_t> logicTimes;
        vector<uint32_t> drawTimes;
        uint64_t maxMemoryBytes = 0;
#endif

        PerformanceData()
        {
#if MAGIQUE_PROFILING == 1
            // Reserve much upfront to not impede benchmarks
            logicTimes.reserve(10000);
            drawTimes.reserve(10000);
#endif
        }

        void draw()
        {
            const auto& config = global::ENGINE_CONFIG;
            if (!config.showPerformanceOverlay)
                return;

            const auto drawBlock = [](const char* text, const Font& f, const float fs, const Vector2 pos, const float w)
            {
                const auto& theme = global::ENGINE_CONFIG.theme;
                const float blockHeight = fs * 1.15F;
                const float borderWidth = fs * 0.1F;
                const Vector2 textPosition = {pos.x + w * 0.07F, pos.y + (blockHeight - fs) / 2};

                const Rectangle container = {pos.x, pos.y, w, blockHeight};
                DrawRectangleRec(container, theme.backLight);
                DrawRectangleLinesEx(container, borderWidth, theme.backDark);
                DrawTextEx(f, text, textPosition, fs, 0.5F, theme.textPassive);
                return w;
            };

            Vector2 position = {0, 0};
            const auto& font = config.font;
            const auto fs = config.fontSize;

            for (const auto& block : blocks)
            {
                if (block.width == 0)
                    continue;
                position.x += drawBlock(block.text, font, fs, position, block.width);

                if (!config.showPerformanceOverlayExt) // Only draw FPS in simple mode
                    break;
            }
        }

        void updateValues()
        {
            tickCounter++;
            // Update values only every 10 ticks
            if (tickCounter != updateDelayTicks) [[likely]]
                return;

            const auto& font = global::ENGINE_CONFIG.font;
            const auto fs = global::ENGINE_CONFIG.fontSize;

            int block = 0;
            const int fps = GetFPS();
            snprintf(blocks[block].text, 32, "FPS: %d", fps);
            blocks[block].width = MeasureTextEx(font, blocks[block].text, fs, 1.0F).x * 1.1F;

            block++;
            auto val = static_cast<float>(logicTickTime) / 1'000'000.0F; // nanos
            snprintf(blocks[block].text, 32, "Tick: %.1f", val);
            blocks[block].width = MeasureTextEx(font, blocks[block].text, fs, 1.0F).x * 1.1F;

            block++;
            val = static_cast<float>(drawTickTime) / 1'000'000.0F;
            snprintf(blocks[block].text, 32, "Draw: %.1f", val);
            blocks[block].width = MeasureTextEx(font, blocks[block].text, fs, 1.0F).x * 1.1F;

            block++;
            const int calls = GetPrevDrawCalls();
            snprintf(blocks[block].text, 32, "Draw Calls: %.1d", calls);
            blocks[block].width = MeasureTextEx(font, blocks[block].text, fs, 1.0F).x * 1.1F;

#if defined(MAGIQUE_STEAM) || defined(MAGIQUE_LAN)
            const auto& mp = global::MP_DATA;
            if (mp.isInSession)
            {
                float inBytes = 0;
                float outBytes = 0;
                float ping = 0.0F;
                if (!mp.isHost)
                {
                    const auto conn = static_cast<HSteamNetConnection>(mp.connections[0]);
                    SteamNetConnectionRealTimeStatus_t info{};
                    SteamNetworkingSockets()->GetConnectionRealTimeStatus(conn, &info, 0, nullptr);
                    inBytes = info.m_flInBytesPerSec;
                    outBytes = info.m_flOutBytesPerSec;
                    ping = (float)info.m_nPing;
                }
                else if (GetIsActiveHost())
                {
                    for (const auto conn : mp.connections) // Only contains valid connections
                    {
                        SteamNetConnectionRealTimeStatus_t info{};
                        SteamNetworkingSockets()->GetConnectionRealTimeStatus(static_cast<HSteamNetConnection>(conn),
                                                                              &info, 0, nullptr);
                        inBytes += info.m_flInBytesPerSec;
                        outBytes += info.m_flOutBytesPerSec;
                        ping += (float)info.m_nPing;
                    }
                    ping /= (float)mp.connections.size();
                }

                block++;
                val = inBytes;
                snprintf(blocks[block].text, 32, "In: %d", (int)val);
                blocks[block].width = MeasureTextEx(font, blocks[block].text, fs, 1.0F).x * 1.1F;

                block++;
                val = outBytes;
                snprintf(blocks[block].text, 32, "Out: %d", (int)val);
                blocks[block].width = MeasureTextEx(font, blocks[block].text, fs, 1.0F).x * 1.1F;

                block++;
                val = ping;
                snprintf(blocks[block].text, 32, "Ping: %d", (int)val);
                blocks[block].width = MeasureTextEx(font, blocks[block].text, fs, 1.0F).x * 1.1F;
            }
            else
#endif
            {
                block++;
                blocks[block].width = 0;
                block++;
                blocks[block].width = 0;
                block++;
                blocks[block].width = 0;
            }
            tickCounter = 0;

#if MAGIQUE_PROFILING == 1
            const auto currentMemory = GetMemoryWorkingSet();
            if (currentMemory > maxMemoryBytes)
                maxMemoryBytes = currentMemory;
#endif
        }

        void saveTickTime(const TickType t, const uint32_t time)
        {
            if (t == UPDATE)
            {
                logicTickTime = time;
#if MAGIQUE_PROFILING == 1
                logicTimes.push_back(time);
#endif
            }
            else if (t == DRAW)
            {
                drawTickTime = time;
#if MAGIQUE_PROFILING == 1
                drawTimes.push_back(time);
#endif
            }
        }

        void printPerformanceStats()
        {
#if MAGIQUE_PROFILING == 0
            return;
#endif
            LOG_INFO("Performance Stats:");
            LOG_INFO("%-10s: CPU: %.2f ms | GPU: %.2f ms", "Avg Ticks", getAverageTime(UPDATE) / 1'000'000,
                     getAverageTime(DRAW) / 1'000'000);
            LOG_INFO("%-10s: Max: %.2f mb", "Memory", maxMemoryBytes / 1'000'000.0F);
        }

    private:
        [[nodiscard]] float getAverageTime(const TickType t)
        {
            vector<uint32_t>* times;
            if (t == UPDATE)
            {
                times = &logicTimes;
            }
            else if (t == DRAW)
            {
                times = &drawTimes;
            }
            else
            {
                return 0.0f;
            }

            if (times->empty())
            {
                return 0.0f;
            }
            times->pop_back();

            uint64_t sum = 0;
            for (const auto num : *times)
            {
                sum += num;
            }
            return static_cast<float>(sum) / static_cast<float>(times->size());
        }
    };

    namespace global
    {
        inline PerformanceData PERF_DATA;
    }

} // namespace magique

#endif //MAGIQUE_PERFDATA_H