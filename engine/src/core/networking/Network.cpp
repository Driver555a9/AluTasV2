#include "core/networking/Network.h"

namespace CoreEngine
{
    /*Client*/
    Client::Client(const int& _port, const char* _server_ip) 
    {
        if (!Network::InitializeENet()) {
            return;
        }
        m_host_ptr = enet_host_create(NULL, 1, Channels::AMOUNT_CHANNELS, 0, 0);

        if(m_host_ptr == nullptr) {
            std::cout << "{Client} ERROR: failed to init client" << std::endl;
            return;
        }

        enet_address_set_host(&m_address, _server_ip);
        m_address.port = _port;

        m_peer_ptr = enet_host_connect(m_host_ptr, &m_address, Channels::AMOUNT_CHANNELS, 0);
        if(m_peer_ptr == nullptr) {
            std::cout << "{Client} ERROR: no avaiable peer to connect to" << std::endl;
            return;
        }
    }

    /* Queue received shall store elements received by peer. Queue shall store elements to the peer.*/
    void Client::Loop(std::atomic<bool>& stop_flag, NetworkQueue<NetworkPacket>& queue_received, NetworkQueue<NetworkPacket>& queue_send) 
    {
        if (enet_host_service(m_host_ptr, &m_event, 5000) > 0 && m_event.type == ENET_EVENT_TYPE_CONNECT) {
            queue_received.EmplaceBack(Channels::CHAT_MESSAGES, "Connected to session");
        } else {
            queue_received.EmplaceBack(Channels::CHAT_MESSAGES, "Failed to connect to session");
            return;
        }

        std::thread send_messages_thread([&] () {
            SendQueueToPeer(std::ref(stop_flag), std::ref(queue_send));
        });

        while(!stop_flag.load(std::memory_order_relaxed))
        {
            while(enet_host_service(m_host_ptr, &m_event, 100) > 0) 
            {
                switch(m_event.type)
                {
                    case ENET_EVENT_TYPE_RECEIVE:
                    {
                        {
                            queue_received.EmplaceBack(m_event.channelID, std::string(reinterpret_cast<char*>(m_event.packet->data)));
                        }
                        enet_packet_destroy(m_event.packet);
                        break;
                    }

                    case ENET_EVENT_TYPE_DISCONNECT:
                    {
                        std::lock_guard<std::mutex> lock_peer(m_peer_mutex);
                        queue_received.EmplaceBack(Channels::CHAT_MESSAGES, std::string("Disconnect: " + GetIPfromAddress(&m_event.peer->address)));
                        if (m_event.peer == m_peer_ptr) {
                            m_peer_ptr = nullptr;
                        }
                        enet_peer_reset(m_event.peer);
                    }
                    break;
                    case ENET_EVENT_TYPE_NONE: default: break;
                }
            }
        }
        send_messages_thread.join();
    }

    /*Server*/
    Server::Server(const int& _port, const int& _max_amount_peers)
    {
        if (!Network::InitializeENet()) {
            return;
        }

        m_address.host = ENET_HOST_ANY;
        m_address.port = _port;

        m_host_ptr = enet_host_create(&m_address, _max_amount_peers, Channels::AMOUNT_CHANNELS, 0, 0);

        if(m_host_ptr == nullptr) {
            std::cout << "{SERVER} ERROR: failed to init Server" << std::endl;
            return;
        }

        m_peer_ptr = nullptr;
    }

    void Server::Loop(std::atomic<bool>& stop_flag, NetworkQueue<NetworkPacket>& queue_received, NetworkQueue<NetworkPacket>& queue_send) 
    {
        std::thread send_messages_thread([&] () {
            SendQueueToPeer(std::ref(stop_flag), std::ref(queue_send));
        });

        while(!stop_flag.load(std::memory_order_relaxed))
        {
            while(enet_host_service(m_host_ptr, &m_event, 100) > 0) 
            {
                switch(m_event.type)
                {
                    case ENET_EVENT_TYPE_CONNECT:
                    {
                        {
                            queue_received.EmplaceBack(Channels::CHAT_MESSAGES, std::string("Connection: ") + GetIPfromAddress(&m_event.peer->address));
                        }
                        {
                            std::lock_guard<std::mutex> lock_peer(m_peer_mutex);
                            m_peer_ptr = m_event.peer;
                            m_peer_vec.push_back(m_event.peer);
                        }
                        break;
                    }

                    case ENET_EVENT_TYPE_RECEIVE:
                    {
                        {
                            queue_received.EmplaceBack(m_event.channelID, std::string(reinterpret_cast<char*>(m_event.packet->data)));
                        }
                        enet_packet_destroy(m_event.packet);
                        break;
                    }

                    case ENET_EVENT_TYPE_DISCONNECT:
                    {
                        {
                            queue_received.EmplaceBack(Channels::CHAT_MESSAGES, std::string("Disconnect: ") + GetIPfromAddress(&m_event.peer->address));
                        }
                        std::lock_guard<std::mutex> lock_peer(m_peer_mutex);
                        if (m_event.peer == m_peer_ptr) {
                            m_peer_ptr = nullptr;
                        }
                        m_peer_vec.erase(std::remove(m_peer_vec.begin(), m_peer_vec.end(), m_event.peer), m_peer_vec.end());
                        enet_peer_reset(m_event.peer);
                        break;
                    }

                    case ENET_EVENT_TYPE_NONE: default: break;
                }
            }
        }
        send_messages_thread.join();
    }


    //Network
    bool Network::s_network_is_initialized = false;

    Network::~Network() 
    {
        if (m_host_ptr) 
        {
            if (m_peer_ptr) 
            {
                enet_peer_disconnect(m_peer_ptr, 0);
            }
            enet_host_destroy(m_host_ptr);
        }
    }

    void Network::SendQueueToPeer(std::atomic<bool>& stop_flag, NetworkQueue<NetworkPacket>& queue_send)
    {
        while(!stop_flag.load(std::memory_order_relaxed))
        {

            NetworkPacket front {Channels::INVALID_CHANNEL, ""};
            if(!m_peer_ptr || !queue_send.TryPop(front)) 
            {
                std::this_thread::yield();
                continue;
            }

            std::lock_guard<std::mutex> lock_peer(m_peer_mutex);
            SendPacket(m_peer_ptr, front.m_data, front.m_channel);
        }
    }

    void Network::SendPacket(ENetPeer* _peer, const std::string& _message, const int _channel)
    {
        ENetPacket* packet = enet_packet_create(_message.c_str(), _message.size(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(_peer, _channel, packet);
    }

    std::string Network::GetIPfromAddress(const ENetAddress* _address) const
    {
        char ip[32];
        if (enet_address_get_host_ip(_address, ip, sizeof(ip)) == 0) {
            return ip;
        } 
        return "";
    }

    bool Network::IsPeerNull() const noexcept
    {
        return !m_peer_ptr;
    }

    bool Network::IsHostNull() const noexcept 
    {
        return !m_host_ptr;
    }

    bool Network::InitializeENet()
    {
        if(s_network_is_initialized) return true;
        s_network_is_initialized = true;

        if(enet_initialize() != 0) {
            std::cout << "{Initialize} Failed to init Enet." << std::endl;
            return false;
        }
        atexit(enet_deinitialize);
        return true;
    }
}