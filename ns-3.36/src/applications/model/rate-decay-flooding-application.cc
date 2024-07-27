/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/mobility-module.h"

#include "rate-decay-flooding-application.h"

using namespace std;

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("RateDecayFloodingApp");

    NS_OBJECT_ENSURE_REGISTERED(RateDecayFloodingApp);

    TypeId
    RateDecayFloodingApp::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::RateDecayFloodingApp")
                                .SetParent<Application>()
                                .SetGroupName("Applications")
                                .AddConstructor<RateDecayFloodingApp>()
                                .AddAttribute("Port", "Port on which we listen for incoming packets.",
                                              UintegerValue(9),
                                              MakeUintegerAccessor(&RateDecayFloodingApp::m_port),
                                              MakeUintegerChecker<uint16_t>())
                                .AddAttribute("SendInterval", "Time between the sending of two packets",
                                              TimeValue(Seconds(1)),
                                              MakeTimeAccessor(&RateDecayFloodingApp::m_sendInterval),
                                              MakeTimeChecker())
                                .AddAttribute("ForwardingJitter", "Time between the sending of two packets",
                                              TimeValue(Seconds(0.11)),
                                              MakeTimeAccessor(&RateDecayFloodingApp::m_forwardingJitter),
                                              MakeTimeChecker())
                                .AddAttribute("PacketSize", "The size of packets transmitted.",
                                              UintegerValue(100),
                                              MakeUintegerAccessor(&RateDecayFloodingApp::m_dataSize),
                                              MakeUintegerChecker<uint32_t>(1))
                                .AddAttribute("MaxDistance", "Maximum Communication range",
                                              DoubleValue(500.0),
                                              MakeDoubleAccessor(&RateDecayFloodingApp::m_maxDistance),
                                              MakeDoubleChecker<double>(0.0))
                                .AddAttribute("DecayFactor", "The decay factor",
                                              DoubleValue(1.0),
                                              MakeDoubleAccessor(&RateDecayFloodingApp::m_decayFactor),
                                              MakeDoubleChecker<double>(0.0))
                                .AddTraceSource("Rx", "A packet has been received",
                                                MakeTraceSourceAccessor(&RateDecayFloodingApp::m_rxTrace),
                                                "ns3::Packet::TracedCallback")
                                .AddTraceSource("RxWithAddresses", "A packet has been received",
                                                MakeTraceSourceAccessor(&RateDecayFloodingApp::m_rxTraceWithAddresses),
                                                "ns3::Packet::TwoAddressTracedCallback")
                                .AddTraceSource("Tx", "A packet has been Sent",
                                                MakeTraceSourceAccessor(&RateDecayFloodingApp::m_txTrace),
                                                "ns3::Packet::TracedCallback")
                                .AddTraceSource("Fwd", "A packet has been Forwarded",
                                                MakeTraceSourceAccessor(&RateDecayFloodingApp::m_fwdTrace),
                                                "ns3::Packet::TracedCallback");
        return tid;
    }

    RateDecayFloodingApp::RateDecayFloodingApp()
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
    }

    RateDecayFloodingApp::~RateDecayFloodingApp()
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
    }

    void
    RateDecayFloodingApp::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    void
    RateDecayFloodingApp::StartApplication(void)
    {
        NS_LOG_FUNCTION(this);

        if (m_socket == 0)
        {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
            if (m_socket->Bind(local) == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            if (addressUtils::IsMulticast(m_local))
            {
                Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);
                if (udpSocket)
                {
                    // equivalent to setsockopt (MCAST_JOIN_GROUP)
                    udpSocket->MulticastJoinGroup(0, m_local);
                }
                else
                {
                    NS_FATAL_ERROR("Error: Failed to join multicast group");
                }
            }

            InetSocketAddress remote = InetSocketAddress(Ipv4Address("255.255.255.255"), 3000);
            m_socket->SetAllowBroadcast(true);
            m_socket->Connect(remote);
            ScheduleTransmit(m_sendInterval);
        }

        m_socket->SetRecvCallback(MakeCallback(&RateDecayFloodingApp::HandleRead, this));
    }

    void
    RateDecayFloodingApp::StopApplication()
    {
        NS_LOG_FUNCTION(this);

        if (m_socket != 0)
        {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        }
    }

    void
    RateDecayFloodingApp::ScheduleTransmit(Time dt)
    {
        NS_LOG_FUNCTION(this << dt);
        m_sendEvent = Simulator::Schedule(dt, &RateDecayFloodingApp::Send, this);
    }

    void RateDecayFloodingApp::Forward(uint32_t src, string pkt_id)
    {
        Ptr<Packet> packet = packetsToForward[src];
        if (std::find(twiceSeenSeqNos.begin(), twiceSeenSeqNos.end(), pkt_id) == twiceSeenSeqNos.end())
        {
            m_socket->Send(packet);
            m_fwdTrace(packet, GetNode()->GetId());
            numForwarded++;
        }
    }

    void
    RateDecayFloodingApp::Send(void)
    {
        NS_LOG_FUNCTION(this);

        NS_ASSERT(m_sendEvent.IsExpired());
        Address localAddress;
        m_socket->GetSockName(localAddress);

        Ptr<Packet> p = Create<Packet>(m_dataSize);
        uint32_t nodeId = GetNode()->GetId();

        Vector nodePos = GetNode()->GetObject<MobilityModel>()->GetPosition();

        ContentionBasedFloodingHeader header;
        header.SetSeq(this->seqNo++);
        header.SetSrc(nodeId);
        header.SetLastHop(nodeId);
        header.SetLastPos(nodePos);
        header.SetStartPos(nodePos);
        p->AddHeader(header);
        m_txTrace(p, GetNode()->GetId());
        numSent++;
        m_socket->Send(p);
        ScheduleTransmit(m_sendInterval);

        string pkt_id = to_string(GetNode()->GetId()) + "-" + to_string(header.GetSeq());
        seenSeqNos.push_back(pkt_id);
    }

    void
    RateDecayFloodingApp::HandleRead(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);

        Ptr<Packet> packet;
        Address from;
        Address localAddress;
        while ((packet = socket->RecvFrom(from)))
        {
            socket->GetSockName(localAddress);
            Ptr<Packet> packetCopy = packet->Copy();
            packetCopy->RemoveAllPacketTags();
            packetCopy->RemoveAllByteTags();
            ContentionBasedFloodingHeader header;
            packetCopy->RemoveHeader(header);

            uint32_t src = header.GetSrc();
            uint32_t numHops = header.GetNumHops();
            Vector nodePos = GetNode()->GetObject<MobilityModel>()->GetPosition();
            double dist_sender = CalculateDistance(header.GetStartPos(), nodePos);
            double dist_sender_lastHop = CalculateDistance(header.GetStartPos(), header.GetLastPos());
            double advance = dist_sender - dist_sender_lastHop;

            header.SetNumHops(numHops + 1);
            header.SetLastHop(GetNode()->GetId());
            header.SetLastPos(nodePos);

            packetCopy->AddHeader(header);

            string pkt_id = to_string(src) + "-" + to_string(header.GetSeq());

            if (std::find(seenSeqNos.begin(), seenSeqNos.end(), pkt_id) == seenSeqNos.end())
            {

                seenNodes.insert(src);
                if (lastReceived.count(src) > 0 && dist_sender <= 509.003)
                {
                    Time aoi = Simulator::Now() - lastReceived[src];
                    if (aoi > m_aoiThreshold)
                    {
                        numUpdatesReceivedLate++;
                    }
                    else
                    {
                        numUpdatesReceivedInTime++;
                    }
                }
                lastReceived[src] = header.GetTs();
                m_rxTrace(packet, GetNode()->GetId());
                m_rxTraceWithAddresses(packet, from, localAddress);
                numReceived++;
                double scale = (1.0 - (advance / m_maxDistance));

                if (scale < 0.0)
                {
                    scale = 0.0;
                }

                Time cbfDelay = m_forwardingJitter * scale;
                Time rdfDelay = Seconds(0);
                if (lastForwarded.find(src) != lastForwarded.end())
                {
                    Time lastForwardedForSrc = lastForwarded[src];
                    // rdfDelay = lastForwardedForSrc + m_sendInterval * pow(m_decayFactor, numHops + 1) - Simulator::Now();
                    rdfDelay = lastForwardedForSrc + m_sendInterval * pow(numHops + 1, m_decayFactor) - Simulator::Now();
                    if (rdfDelay < Seconds(0))
                    {
                        rdfDelay = Seconds(0);
                    }
                }

                Time delay = cbfDelay + rdfDelay;
                packetsToForward[src] = packetCopy;

                if (advance > 0)
                {
                    Simulator::Schedule(delay, &RateDecayFloodingApp::Forward, this, src, pkt_id);
                }
                seenSeqNos.push_back(pkt_id);
                lastForwarded[src] = Simulator::Now() + rdfDelay;
            }
            else
            {
                if (std::find(twiceSeenSeqNos.begin(), twiceSeenSeqNos.end(), pkt_id) == twiceSeenSeqNos.end())
                {
                    twiceSeenSeqNos.push_back(pkt_id);
                }
            }
        }
    }

    int RateDecayFloodingApp::GetNumUpdatesReceivedInTime()
    {
        return numUpdatesReceivedInTime;
    }

    int RateDecayFloodingApp::GetNumUpdatesReceivedLate()
    {
        return numUpdatesReceivedLate;
    }

    int RateDecayFloodingApp::GetNumSeenNodes()
    {
        return seenNodes.size();
    }

    int RateDecayFloodingApp::GetNumSent()
    {
        return numSent;
    }

    int RateDecayFloodingApp::GetNumFwd()
    {
        return numForwarded;
    }

    int RateDecayFloodingApp::GetNumRcvd()
    {
        return numReceived;
    }

    void RateDecayFloodingApp::ResetStats()
    {
        numUpdatesReceivedInTime = 0;
        numUpdatesReceivedLate = 0;
        seenNodes.clear();
        numSent = 0;
        numReceived = 0;
        numForwarded = 0;
    }

} // Namespace ns3
