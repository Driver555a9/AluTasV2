#include "tas/common/ReplaySerializer.h"

#include "core/utility/CommonUtility.h"
#include "core/utility/Assert.h"

#include <cstring>
#include <fstream>

namespace AsphaltTas
{
    void ReplaySerializer::SaveBinary(const Replay& replay, const std::string& path) noexcept
    {
        try
        {
            const auto& frames = replay.GetFrameVectorConstReference();
            const size_t byte_size = frames.size() * sizeof(Replay::Frame);

            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file)
                throw std::runtime_error("Failed to create file at path: " + path);

            file.write(reinterpret_cast<const char*>(frames.data()),
                static_cast<std::streamsize>(byte_size));
        }
        catch (const std::exception& e) 
        {
            ENGINE_ERROR_PRINT("Failed to save replay: " << e.what());
        }
    }

    Replay ReplaySerializer::LoadBinary(const std::string& path) noexcept
    {
        Replay replay_out;
        try
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) 
            {
                throw std::runtime_error("Failed to open file at path: " + path);
            }

            const std::streamsize size = file.tellg();
            if (size < 0 || size % sizeof(Replay::Frame) != 0)
                return replay_out;

            const size_t frame_count = size / sizeof(Replay::Frame);

            std::vector<Replay::Frame> frames(frame_count);

            file.seekg(0);
            file.read(reinterpret_cast<char*>(frames.data()), size);
            if (!file) return replay_out;

            replay_out.SetFrameData(std::move(frames));
        }
        catch (const std::exception& e) 
        {
            ENGINE_ERROR_PRINT("Failed to load replay: " << e.what());
        }

        return replay_out;
    }
}