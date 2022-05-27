#include "network_interface.hh"

#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

ARPMessage NetworkInterface::CreateARPMessage(uint16_t type, IP target_ip, const EthernetAddress& target_mac) noexcept {
    ARPMessage arp_message; 

    arp_message.opcode = type;

    arp_message.sender_ethernet_address = _ethernet_address;
    arp_message.sender_ip_address = _ip_address.ipv4_numeric();

    arp_message.target_ethernet_address = target_mac;
    arp_message.target_ip_address = target_ip;
    
    return arp_message;
}


//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    
    EthernetFrame frame;
    frame.header().src = _ethernet_address;

    // if cached, send ipv4 datagram
    if (_arp_cache.find(next_hop_ip) != _arp_cache.end()) {
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.header().dst = _arp_cache[next_hop_ip].second;
        frame.payload() = dgram.serialize();
    
    } else {
        auto iter = _arp_request.find(next_hop_ip);
        if (iter != _arp_request.end() && _arp_request[next_hop_ip].first < 5000) {
            return;
        }
        _arp_request[next_hop_ip] = std::make_pair(0, dgram);

        // set ethnet frame header
        frame.header().type = EthernetHeader::TYPE_ARP;
        frame.header().dst = Board_Mac_Adress;

        // create arp message
        ARPMessage arp_message = CreateARPMessage(ARPMessage::OPCODE_REQUEST, next_hop_ip, {});

        frame.payload() = arp_message.serialize();
    }

    _frames_out.push(std::move(frame));
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != _ethernet_address && frame.header().dst != Board_Mac_Adress) {
        return nullopt;
    }

    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram datagram;
        if (datagram.parse(frame.payload()) != ParseResult::NoError) {
            return nullopt;
        }
        return datagram;

    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage received_arp_message;

        if (received_arp_message.parse(frame.payload()) != ParseResult::NoError) {
            return nullopt;
        }
  
        // const EthernetHeader& head = frame.header();
        const IP target_ip = received_arp_message.target_ip_address;
        const EthernetAddress& target_mac = received_arp_message.target_ethernet_address;

        if (target_ip != _ip_address.ipv4_numeric()) {
            return nullopt;
        }

        const IP source_ip = received_arp_message.sender_ip_address;
        const EthernetAddress& source_mac = received_arp_message.sender_ethernet_address;

        _arp_cache[source_ip] = std::make_pair(0, source_mac);

        if (received_arp_message.opcode == ARPMessage::OPCODE_REPLY && target_mac == _ethernet_address) {
         // if is arp reply, update apt route table
            IPv4Datagram datagram = _arp_request[source_ip].second;

            EthernetFrame ether_frame;
            ether_frame.header() = {source_mac, _ethernet_address, EthernetHeader::TYPE_IPv4};

            ether_frame.payload() = datagram.serialize();

            _frames_out.push(ether_frame);

            _arp_request.erase(source_ip);
            
            // return datagram;
            return nullopt;

        } else if(received_arp_message.opcode == ARPMessage::OPCODE_REQUEST and 
            (_arp_cache.find(target_ip) != _arp_cache.end() || _ip_address.ipv4_numeric() == target_ip)){

        // if is arp request
            EthernetFrame ether_frame;
            ether_frame.header() = {source_mac, _ethernet_address, EthernetHeader::TYPE_ARP};

            ARPMessage arp_reply_message = CreateARPMessage(ARPMessage::OPCODE_REPLY, source_ip, source_mac);

            ether_frame.payload() = arp_reply_message.serialize();

            _frames_out.push(ether_frame);
        }
    }
    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
    auto table_iter = _arp_cache.begin();
    while (table_iter != _arp_cache.end()) {
        table_iter->second.first += ms_since_last_tick;

        if (table_iter->second.first > 30000) {
            table_iter = _arp_cache.erase(table_iter);
        } else {
            ++table_iter;
        }
    }

    for (auto &iter : _arp_request) {
        
        iter.second.first += ms_since_last_tick;

        if (iter.second.first > 5000) {
            ARPMessage arp_request = CreateARPMessage(ARPMessage::OPCODE_REQUEST, iter.first, {});
            EthernetFrame frame;
            frame.header() = {{Board_Mac_Adress}, _ethernet_address, EthernetHeader::TYPE_ARP};
            frame.payload() = std::move(arp_request.serialize());

            _frames_out.push(frame);

            iter.second.first = 0;
        }
    }
}
