/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2024 [Your Organization]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SLF_FLOODING_APP_H
#define SLF_FLOODING_APP_H

#include <algorithm>
#include <math.h>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/seq-ts-header.h"
#include "ns3/traced-callback.h"
#include "ns3/contention-based-flooding-header.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

namespace ns3
{

class Socket;
class Packet;

class SLFFloodingApp : public Application
{
public:
    static TypeId GetTypeId(void);
    SLFFloodingApp();
    virtual ~SLFFloodingApp();
    void LogPerformance() {
      NS_LOG_UNCOND("PERFORMANCE: " << seenNodes.size() << " " << numUpdatesReceivedInTime << " " << numUpdatesReceivedLate);
    };

    void SetGroundStationIds (const std::vector<uint32_t>& ids);

    int GetNumUpdatesReceivedInTime();
    int GetNumUpdatesReceivedLate();
    int GetNumSeenNodes();
    int GetNumSent();
    int GetNumFwd();
    int GetNumRcvd();
    void ResetStats();

protected:
    virtual void DoDispose(void);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    void ScheduleTransmit(Time dt);
    
    void Send(void);
    void Forward(uint32_t src, std::string pkt_id, double a, uint32_t dest);
    void HandleRead(Ptr<Socket> socket);
    //void SetGroundStationPosition();
    //void HandleControlPacket(Ptr<Packet> packet, const Address& from, const Address& localAddress);
    //void HandlePositionUpdatePacket(Ptr<Packet> packet, const Address& from, const Address& localAddress);
    

    int seqNo = 0;
    int controlPacketCounter = 0;// New variable added for control packet
    int controlSeqNo = 0;// New variable added for control packet
    double m_maxDistance = 509.003;
    double m_decayFactor = 1.0;
    Time m_aoiThreshold = Seconds(0.73573573573);
    Time m_sendInterval = Seconds(1);
    Time m_forwardingJitter = Seconds(0.1);
    
    double m_threshold = 200.0;
    uint32_t m_dataSize = 100;
    uint16_t m_port;
    uint32_t DestinationNodeId=0;
    uint32_t m_nodesPerContainer=5;
    Ptr<Node> destinationNode; 
    Ptr<Socket> m_socket;
    Address m_local;
    EventId m_sendEvent;
    
    Address m_peerAddress = Ipv4Address("255.255.255.255");
    uint16_t m_peerPort = 300;
    std::vector<std::string> seenSeqNos;
    std::vector<std::string> twiceSeenSeqNos;
    std::vector<std::string> duplicateSeenSeqNos; // New vector added for duplicate packets
    std::vector<uint32_t> m_groundStationIds; // Vector for ground station IDs
    std::map<uint32_t, Time> lastForwarded;
    std::map<uint32_t, Ptr<Packet>> packetsToForward;
    std::map<uint32_t, Time> lastReceived;
    std::set<uint32_t> seenNodes;
    int numUpdatesReceivedInTime = 0;
    int numUpdatesReceivedLate = 0;
    int numSent = 0;
    int numReceived = 0;
    int numForwarded = 0;
    TracedCallback<Ptr<const Packet>, uint32_t> m_rxTrace;
    TracedCallback<Ptr<const Packet>, uint32_t> m_txTrace;
    TracedCallback<Ptr<const Packet>, uint32_t> m_fwdTrace;
    TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* SLF_FLOODING_APP_H */
