#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

void PrintQueueContents(Ptr<Queue<Packet>> queue) {
    uint32_t nPackets = queue->GetNPackets();
    NS_LOG_UNCOND("Number of packets in the queue: " << nPackets);
    
    NS_LOG_UNCOND("Packets in the queue:");
    for (uint32_t i = 0; i < nPackets; ++i) {
        Ptr<Packet> packet = queue->Dequeue();
        NS_LOG_UNCOND("Packet " << i << ": " << *packet);
        queue->Enqueue(packet);
    }
}

int main (int argc, char *argv[]) {
    CommandLine cmd (__FILE__);
    cmd.Parse (argc, argv);
    
    Time::SetResolution (Time::NS);
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    // Get the queue associated with the sender's net device
    Ptr<Queue<Packet>> queue = DynamicCast<PointToPointNetDevice>(devices.Get (0))->GetQueue ();

    UdpEchoServerHelper echoServer (9);

    ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));

    // Print the packets in the queue every second
    Simulator::Schedule (Seconds (1.0), &PrintQueueContents, queue);

    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
