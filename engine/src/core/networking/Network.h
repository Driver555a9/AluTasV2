#pragma once

#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <queue>
#include <algorithm>
#include <mutex>

namespace CoreEngine
{
    struct NetworkPacket
    {
        int m_channel;
        std::string m_data;

        NetworkPacket(const int& channel, const std::string& data) : m_channel(channel), m_data(data) {}
    };

    template <typename T>
    class NetworkQueue
    {
    private:
        mutable std::mutex m_mutex;
        std::queue<T> m_queue;

    public:
        void Push(T value)
        {
            std::lock_guard lock(m_mutex);
            m_queue.push(std::move(value));
        }

        template <typename... Args>
        void EmplaceBack(Args&&... args)
        {
            std::lock_guard lock(m_mutex);
            m_queue.emplace(std::forward<Args>(args)...);
        }

        bool TryPop(T& out)
        {
            std::lock_guard lock(m_mutex);
            if (m_queue.empty())
                return false;

            out = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        [[nodiscard]] bool IsEmpty() const
        {
            std::lock_guard lock(m_mutex);
            return m_queue.empty();
        }

        [[nodiscard]] size_t Size() const
        {
            std::lock_guard lock(m_mutex);
            return m_queue.size();
        }

        void Clear()
        {
            std::lock_guard lock(m_mutex);
            std::queue<T> empty;
            m_queue.swap(empty);
        }
    };

    enum Channels : int {
        AMOUNT_CHANNELS = 2,

        CHAT_MESSAGES = 0,
        WORLD_SYNC    = 1,

        INVALID_CHANNEL = 999999
    };

    enum NetworkTypes : int {
        CLIENT = 0,
        SERVER = 1
    };

    class Network 
    {
        private:
            static bool s_network_is_initialized;

        protected:
            ENetHost* m_host_ptr;
            ENetAddress m_address;
            ENetEvent m_event;
            ENetPeer* m_peer_ptr;

            std::mutex m_peer_mutex;

            Network() {}

            void SendPacket(ENetPeer* _peer, const std::string& message, const int _channel);

            [[nodiscard]] std::string GetIPfromAddress(const ENetAddress* _address) const;

            virtual void SendQueueToPeer(std::atomic<bool>& stop_flag, NetworkQueue<NetworkPacket>& queue_send);

            static bool InitializeENet();

        public:
            virtual ~Network();
            virtual void Loop(std::atomic<bool>& stop_flag, NetworkQueue<NetworkPacket>& queue_received, NetworkQueue<NetworkPacket>& queue_send){}
            [[nodiscard]] bool IsPeerNull() const noexcept;
            [[nodiscard]] bool IsHostNull() const noexcept;
    };

    class Client : public Network {
        public:
            Client(const int& _port, const char* _server_ip);
            void Loop(std::atomic<bool>& stop_flag, NetworkQueue<NetworkPacket>& queue_received, NetworkQueue<NetworkPacket>& queue_send) override;
    };

    class Server : public Network {
        std::vector<ENetPeer*> m_peer_vec;
        public:
            Server(const int& _port, const int& _max_amount_peers);
            void Loop(std::atomic<bool>& stop_flag, NetworkQueue<NetworkPacket>& queue_received, NetworkQueue<NetworkPacket>& queue_send) override;
    };
}