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
#include "ns3/spot-light-flooding-application.h"
#include "ns3/node-list.h"
#include "ns3/position-allocator.h"
#include <cmath> // new for exponential 
#include <random>
#include <iostream>
#include <vector>

using namespace std;

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("SLFFloodingApp");

    NS_OBJECT_ENSURE_REGISTERED(SLFFloodingApp);

    TypeId
    SLFFloodingApp::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::SLFFloodingApp")
                                .SetParent<Application>()
                                .SetGroupName("Applications")
                                .AddConstructor<SLFFloodingApp>()
                                .AddAttribute("Port", "Port on which we listen for incoming packets.",
                                              UintegerValue(9),
                                              MakeUintegerAccessor(&SLFFloodingApp::m_port),
                                              MakeUintegerChecker<uint16_t>())
                                .AddAttribute("SendInterval", "Time between the sending of two packets",
                                              TimeValue(Seconds(1)),
                                              MakeTimeAccessor(&SLFFloodingApp::m_sendInterval),
                                              MakeTimeChecker())
                                .AddAttribute("ForwardingJitter", "Time between the sending of two packets",
                                              TimeValue(Seconds(0.11)),
                                              MakeTimeAccessor(&SLFFloodingApp::m_forwardingJitter),
                                              MakeTimeChecker())
                                .AddAttribute("PacketSize", "The size of packets transmitted.",
                                              UintegerValue(100),
                                              MakeUintegerAccessor(&SLFFloodingApp::m_dataSize),
                                              MakeUintegerChecker<uint32_t>(1))
                                .AddAttribute("MaxDistance", "Maximum Communication range",
                                              DoubleValue(500.0),
                                              MakeDoubleAccessor(&SLFFloodingApp::m_maxDistance),
                                              MakeDoubleChecker<double>(0.0))
                                .AddAttribute("DecayFactor", "The decay factor",
                                              DoubleValue(1.0),
                                              MakeDoubleAccessor(&SLFFloodingApp::m_decayFactor),
                                              MakeDoubleChecker<double>(0.0))
                                .AddAttribute("Threshold", "Threshold for SLF protocol",
                                              DoubleValue(1.0),
                                              MakeDoubleAccessor(&SLFFloodingApp::m_threshold),
                                              MakeDoubleChecker<double>(0.0))
                                .AddTraceSource("Rx", "A packet has been received",
                                                MakeTraceSourceAccessor(&SLFFloodingApp::m_rxTrace),
                                                "ns3::Packet::TracedCallback")
                                .AddTraceSource("RxWithAddresses", "A packet has been received",
                                                MakeTraceSourceAccessor(&SLFFloodingApp::m_rxTraceWithAddresses),
                                                "ns3::Packet::TwoAddressTracedCallback")
                                .AddTraceSource("Tx", "A packet has been Sent",
                                                MakeTraceSourceAccessor(&SLFFloodingApp::m_txTrace),
                                                "ns3::Packet::TracedCallback")
                                .AddTraceSource("Fwd", "A packet has been Forwarded",
                                                MakeTraceSourceAccessor(&SLFFloodingApp::m_fwdTrace),
                                                "ns3::Packet::TracedCallback")
                                .AddAttribute("nodesPerContainer", "Number of nodes per container A, B, and C.",
                                                UintegerValue(5),
                                                MakeUintegerAccessor(&SLFFloodingApp::m_nodesPerContainer),
                                                MakeUintegerChecker<uint32_t>(1));
                                         
                                          

        return tid;
    }

    SLFFloodingApp::SLFFloodingApp()
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
                
        
    }

    SLFFloodingApp::~SLFFloodingApp()
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
    }

    void SLFFloodingApp::SetGroundStationIds (const std::vector<uint32_t>& ids)
    {
       m_groundStationIds = ids ;
    
    }

    void
    SLFFloodingApp::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    void
    SLFFloodingApp::StartApplication(void)
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

        m_socket->SetRecvCallback(MakeCallback(&SLFFloodingApp::HandleRead, this));
    }

    void
    SLFFloodingApp::StopApplication()
    {
        NS_LOG_FUNCTION(this);

        if (m_socket != 0)
        {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        }
    }

    void
    SLFFloodingApp::ScheduleTransmit(Time dt)
    {
        NS_LOG_FUNCTION(this << dt);

        // Schedule the next packet transmission
        m_sendEvent = Simulator::Schedule(dt, &SLFFloodingApp::Send, this);
    }

 void SLFFloodingApp::Forward(uint32_t src, string pkt_id, double a, uint32_t dest)
    {
     
      if (std::find(m_groundStationIds.begin(), m_groundStationIds.end(), GetNode()->GetId()) != m_groundStationIds.end()) {
             // If the current node is the destination node, simply return without forwarding
            return;
             }



        Ptr<Packet> packet = packetsToForward[src];

    //If it is control packet
    if (dest != 0)
    {
        
      if (a >= m_threshold)
      {
    
         // If the packet is not seen in duplicates, forward it

        if ((std::find(duplicateSeenSeqNos.begin(), duplicateSeenSeqNos.end(), pkt_id) == duplicateSeenSeqNos.end()))
         {
        
            m_socket->Send(packet);
            m_fwdTrace(packet, GetNode()->GetId());
            numForwarded++;

         
        
            duplicateSeenSeqNos.push_back(pkt_id); // Inorder to allow every node to forward the packet one time we are updating this vector inside here.
            seenSeqNos.push_back(pkt_id);
            twiceSeenSeqNos.push_back(pkt_id);

        } 
      
      }
      else if ((a < 0) || (0 <= a && a < m_threshold)) 
      {
          if ((std::find(twiceSeenSeqNos.begin(), twiceSeenSeqNos.end(), pkt_id) == twiceSeenSeqNos.end()))
                {
                   // If the packet is not seen in twice seen, forward it
           
                  //NS_LOG_INFO("value of a: " << a);
                   m_socket->Send(packet);
                   m_fwdTrace(packet, GetNode()->GetId());

                   numForwarded++;

                 //NS_LOG_INFO("value of a: " << a << "value of dest: " << dest << "value of src: " << src << "value of pkt_id: " << pkt_id << "value of lastForwarded: " << lastForwarded[src] << "value of now: " << Simulator::Now() << "value of delay: " << m_forwardingJitter);
                }
        }
    }
    
    else 
    {
      if (std::find(twiceSeenSeqNos.begin(), twiceSeenSeqNos.end(), pkt_id) == twiceSeenSeqNos.end())
       {
          // If the packet is not seen in twice seen, forward it
         // NS_LOG_INFO("Received packet with destination node ID: " << std::to_string(dest));

          m_socket->Send(packet);
          m_fwdTrace(packet, GetNode()->GetId());
           numForwarded++;

           
        }
    }

    }
  

void SLFFloodingApp::Send(void) {
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_sendEvent.IsExpired());
    Address localAddress;
    m_socket->GetSockName(localAddress);

    // Create a new packet and set its header
    Ptr<Packet> packet = Create<Packet>();
    uint32_t nodeId = GetNode()->GetId();

    // Check if the current node is a ground station
    if (!m_groundStationIds.empty() && std::find(m_groundStationIds.begin(), m_groundStationIds.end(), nodeId) != m_groundStationIds.end()) {
        // If the node is a ground station, don't send any packets
        ScheduleTransmit(m_sendInterval);
        return; // Exit the function
    }

    Vector nodePos = GetNode()->GetObject<MobilityModel>()->GetPosition();

    // Create a new header for the packet
    ContentionBasedFloodingHeader header;

    // Set the source node ID and sequence number
    header.SetSeq(seqNo++);
    header.SetSrc(nodeId);

    // Determine the destination node ID based on the current node's ID
    uint32_t destNodeId = 0; // Default destination ID

    if (!m_groundStationIds.empty()) {
        if (nodeId < m_nodesPerContainer) {
            destNodeId = m_groundStationIds[0];
        } else if (nodeId >= m_nodesPerContainer && nodeId < 2 * m_nodesPerContainer) {
            if (m_groundStationIds.size() > 1) {
                destNodeId = m_groundStationIds[1];
            }
        } else if (nodeId >= 2 * m_nodesPerContainer && nodeId < 3 * m_nodesPerContainer) {
            if (m_groundStationIds.size() > 2) {
                destNodeId = m_groundStationIds[2];
            }
        }
    }

    header.SetDest(destNodeId);
    header.SetLastHop(nodeId);
    header.SetLastPos(nodePos);
    header.SetStartPos(nodePos);

    // Increment control packet counter and update control sequence number
    controlPacketCounter++;
    if (controlPacketCounter > 8) {
        controlPacketCounter = 1;
        controlSeqNo++;
    }
    header.SetControlSeq(controlSeqNo);

    // Add header to the packet
    packet->AddHeader(header);

    // Log detailed packet information for debugging
    m_txTrace(packet, GetNode()->GetId());

    // Increment the counter for sent packets
    numSent++;

    // Send the packet through the socket
    m_socket->Send(packet);

    // Schedule the next packet transmission
    ScheduleTransmit(m_sendInterval);

    // Store the packet ID in the list of seen sequence numbers
    std::string pkt_id = std::to_string(GetNode()->GetId()) + "-" + std::to_string(header.GetSeq());
    seenSeqNos.push_back(pkt_id);
}



void SLFFloodingApp::HandleRead(Ptr<Socket> socket)
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
        uint32_t dest = header.GetDest();
        
        
        

        //NS_LOG_INFO("Received packet with destination node ID: " << std::to_string(dest));


        
        uint32_t numHops = header.GetNumHops();
        Vector nodePos = GetNode()->GetObject<MobilityModel>()->GetPosition();

        Ptr<Node> destinationNode = NodeList::GetNode(dest);

        //NS_LOG_DEBUG("value of dest=" << dest);


        //NS_LOG_DEBUG("Time: "<<Simulator::Now ().GetSeconds ()<<"Node ID: " << GetNode()->GetId() << " Position: x=" << nodePos.x << ", y=" << nodePos.y << ", z=" << nodePos.z);

        Vector destNodePos = destinationNode->GetObject<MobilityModel>()->GetPosition();
        double dist_sender = CalculateDistance(header.GetStartPos(), nodePos);
        double dist_sender_lastHop = CalculateDistance(header.GetStartPos(), header.GetLastPos());
        double advance = dist_sender - dist_sender_lastHop;
        
        //NS_LOG_DEBUG("Node ID: " << destinationNode->GetId() << " Position: x=" << destNodePos.x << ", y=" << destNodePos.y << ", z=" << destNodePos.z);

        // Handle control packet
        double dist_sender_lastHop_new = CalculateDistance(header.GetLastPos(), destNodePos);
        double dist_sender_new = CalculateDistance(nodePos, destNodePos);
        double a = dist_sender_lastHop_new - dist_sender_new;
             
        //NS_LOG_DEBUG("value of a=" << dist_sender_new);


        header.SetNumHops(numHops + 1);
        header.SetLastHop(GetNode()->GetId());
        header.SetLastPos(nodePos);

        // Log detailed packet information for debugging
    //NS_LOG_DEBUG("Detailed packet information: src=" << header.GetSrc() << ", dest=" << header.GetDest()<<", seq=" << header.GetSeq()<< ", packetSize=" << packet->GetSize());



        packetCopy->AddHeader(header);

        string pkt_id = to_string(src) + "-" + to_string(header.GetSeq());

        uint32_t nodeId = GetNode()->GetId();

      // forward control packet if a is greater than threshold  
     
     
      if ((a >= m_threshold) && (dest != 0))

        {
            if (std::find(seenSeqNos.begin(), seenSeqNos.end(), pkt_id) == seenSeqNos.end()) 
            {
                       seenNodes.insert(src);
                       if (lastReceived.count(src) > 0 && dist_sender <= 509.003) 
                   {
                            Time aoi = Simulator::Now() - lastReceived[src];
                        if (aoi > m_aoiThreshold) 
                         {
                              numUpdatesReceivedLate++;
                        } else 
                            {
                                numUpdatesReceivedInTime++;
                            }
                    }
                         lastReceived[src] = header.GetTs();
                         m_rxTrace(packet, GetNode()->GetId());
                         m_rxTraceWithAddresses(packet, from, localAddress);
                         numReceived++;





                    auto p = CreateObject<UniformRandomVariable>();

    
          p->SetAttribute("Min", DoubleValue(0));//to create random jitter with uniform distribution which produce less redundant packet compare to exponentialFactorForA
          p->SetAttribute("Max", DoubleValue(0.27));

  


    double random_value = p->GetValue();
    

    Time c_n; //small jitter for control packet which has a greater than threshold.

            c_n = m_forwardingJitter*random_value;
            //NS_LOG_DEBUG("value of a=" << c_n);
            



                    packetsToForward[src] = packetCopy;

                     //forward the packet 
                     if (advance > 0)
                        {

                           Simulator::Schedule(c_n, &SLFFloodingApp::Forward, this, src, pkt_id, a, dest);

                        }


        }

        } 
      

     else if (std::find(seenSeqNos.begin(), seenSeqNos.end(), pkt_id) == seenSeqNos.end()) 
 
 {
    // The packet ID is not seen before
    seenNodes.insert(src);
    if (lastReceived.count(src) > 0 && dist_sender <= 509.003) 
    {
        Time aoi = Simulator::Now() - lastReceived[src];
        if (aoi > m_aoiThreshold) 
        {
            numUpdatesReceivedLate++;
        } else 
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
    
    double scale_c = (1.0 - (a / m_maxDistance));
    if (scale_c < 0.0) 
    {
        scale_c = 0.0;
    }


    Time cbfDelay = m_forwardingJitter * scale;
    Time cbfDelay_c = m_forwardingJitter * scale_c;


    Time rdfDelay = Seconds(0);
    if (lastForwarded.find(src) != lastForwarded.end()) 
    {
        Time lastForwardedForSrc = lastForwarded[src];
        rdfDelay = lastForwardedForSrc + m_sendInterval * pow(numHops + 1, m_decayFactor) - Simulator::Now();
        if (rdfDelay < Seconds(0))
         {
            rdfDelay = Seconds(0);
        }
    }
    Time delay = cbfDelay + rdfDelay;


    //NS_LOG_DEBUG("value of delay=" << c_n);
    packetsToForward[src] = packetCopy;

    // Now procedure begin for chossing forward method

    if (dest == 0) 
    {
        // Handle position update packet
        if (advance > 0) 
        {
            Simulator::Schedule(delay, &SLFFloodingApp::Forward, this, src, pkt_id, a,dest);
        }

         seenSeqNos.push_back(pkt_id);
         lastForwarded[src] = Simulator::Now() + rdfDelay;
          

    } 


        // handle control packet which has a less than threshold
          else {
                        if (a < 0) 
                    {
                          if (advance > 0)
                        {      
                             Simulator::Schedule(delay, &SLFFloodingApp::Forward, this, src, pkt_id, a,dest);
                        }
                          seenSeqNos.push_back(pkt_id);
                          lastForwarded[src] = Simulator::Now() + rdfDelay;
                        

                    } else  if (0 <= a && a < m_threshold)
                       {
                             
                            if (advance > 0)
                            { 
                                  Simulator::Schedule(cbfDelay_c, &SLFFloodingApp::Forward, this, src, pkt_id, a,dest);
                            
                                    seenSeqNos.push_back(pkt_id);
                                    //duplicateSeenSeqNos.push_back(pkt_id);
                                    lastForwarded[src] = Simulator::Now() + cbfDelay_c;
                            
                            }
                         }

                         

                }
   }       
   
   else
           {
                      // The packet ID is seen before
                  if (std::find(twiceSeenSeqNos.begin(), twiceSeenSeqNos.end(), pkt_id) == twiceSeenSeqNos.end()) 
                {
                  twiceSeenSeqNos.push_back(pkt_id);
                 }
             }


        



        
      
    }
}





int SLFFloodingApp::GetNumUpdatesReceivedInTime()
    {
        return numUpdatesReceivedInTime;
    }

    int SLFFloodingApp::GetNumUpdatesReceivedLate()
    {
        return numUpdatesReceivedLate;
    }

    int SLFFloodingApp::GetNumSeenNodes()
    {
        return seenNodes.size();
        

    }

    int SLFFloodingApp::GetNumSent()
    {
        return numSent;
    }

    int SLFFloodingApp::GetNumFwd()
    {
        return numForwarded;
    }

    int SLFFloodingApp::GetNumRcvd()
    {
        return numReceived;
    }

    void SLFFloodingApp::ResetStats()
    {
        numUpdatesReceivedInTime = 0;
        numUpdatesReceivedLate = 0;
        seenNodes.clear();
        numSent = 0;
        numReceived = 0;
        numForwarded = 0;
    }





} // Namespace ns3
      