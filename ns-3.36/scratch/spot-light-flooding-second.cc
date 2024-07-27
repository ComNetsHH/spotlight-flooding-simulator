#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/seq-ts-header.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/wifi-standards.h"
#include "ns3/rectangle.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/application-container.h"
#include "ns3/pure-flooding-application.h"
#include "ns3/flooding-helper.h"
#include "ns3/contention-based-flooding-header.h"
#include "ns3/spot-light-flooding-application.h"
#include "CsvLogger.h"
#include "KpiLogger.h"
#include <vector>
#include <set>


using namespace ns3;
using namespace std;
using namespace std::string_literals;

NS_LOG_COMPONENT_DEFINE("SLF");

CsvLogger resLogger = CsvLogger();
CsvLogger courseLogger = CsvLogger();
KpiLogger kpiLogger = KpiLogger();

/*void CourseChange(Ptr<const MobilityModel> mobility, uint32_t nodeId)
{
  courseLogger.CreateCourse(nodeId, mobility);
  Simulator::Schedule(Seconds(0.1), &CourseChange, mobility, nodeId);
}*/

// Global vectors to store unique control sequence numbers
std::set<std::string> uniqueSentControlSeqNos;
std::set<std::string> uniqueReceivedControlSeqNos;

void OnPacketReceive(std::string context, Ptr<const Packet> pkt, uint32_t nodeId)
{
  ContentionBasedFloodingHeader header;
  pkt->PeekHeader(header);

  double current = Simulator::Now().GetSeconds();
  double sent = header.GetTs().GetSeconds();
  double delay = (current - sent) * 1000;

  uint32_t src = header.GetSrc();
  uint32_t dest = header.GetDest(); // Get the destination node ID
  uint32_t lastHop = header.GetLastHop();
  uint32_t numHops = header.GetNumHops();
  //if (nodeId == 4 ) {
  string seqNo = to_string(src) + "-" + to_string(header.GetSeq());
  string controlSeqNo = to_string(src) + "-" + to_string(header.GetControlSeq());

  string type = "PktRcvd";

 

  resLogger.CreateEntry(nodeId, seqNo,controlSeqNo, type, to_string(src), to_string(lastHop), to_string(delay), to_string(numHops), to_string(dest)  );
  //resLogger.CreateEntry(nodeId, seqNo, type, to_string(src), to_string(dest), to_string(lastHop), to_string(delay), to_string(numHops));
 


}

void OnPacketSent(std::string context, Ptr<const Packet> pkt, uint32_t nodeId)
{
  ContentionBasedFloodingHeader header;
  pkt->PeekHeader(header);

  uint32_t src = header.GetSrc();
  uint32_t dest = header.GetDest(); // Get the destination node ID
  uint32_t lastHop = header.GetLastHop();
  uint32_t numHops = header.GetNumHops();
  
  // Check if the node ID is equal to 0
  // if (nodeId == 0) {

  string seqNo = to_string(src) + "-" + to_string(header.GetSeq());
  string controlSeqNo = to_string(src) + "-" + to_string(header.GetControlSeq());

  string type = "PktSent";

    // Only add unique control sequence numbers for node 0
    //if (nodeId == 0 && dest == 4)  {
        //uniqueSentControlSeqNos.insert(controlSeqNo);
     //}
  
  resLogger.CreateEntry(nodeId, seqNo,controlSeqNo, type, to_string(src), to_string(lastHop), "-1", to_string(numHops), to_string(dest));
  //resLogger.CreateEntry(nodeId, seqNo, type, to_string(src), to_string(dest), to_string(lastHop), "-1", to_string(numHops));

  

}


void OnPacketForward(std::string context, Ptr<const Packet> pkt, uint32_t nodeId)
{
  ContentionBasedFloodingHeader header;
  pkt->PeekHeader(header);

  double current = Simulator::Now().GetSeconds();
  double sent = header.GetTs().GetSeconds();
  double delay = (current - sent) * 1000;

  uint32_t src = header.GetSrc();
  uint32_t dest = header.GetDest(); // Get the destination node ID
  uint32_t lastHop = header.GetLastHop();
  uint32_t numHops = header.GetNumHops();

  string seqNo = to_string(src) + "-" + to_string(header.GetSeq());
  string controlSeqNo = to_string(src) + "-" + to_string(header.GetControlSeq());

  string type = "PktFwd";
  //resLogger.CreateEntry(nodeId, seqNo, type, to_string(src), to_string(dest), to_string(lastHop), to_string(delay), to_string(numHops));
  resLogger.CreateEntry(nodeId, seqNo,controlSeqNo, type, to_string(src), to_string(lastHop), to_string(delay), to_string(numHops),to_string(dest));
}

void LogProgress()
{
  NS_LOG_UNCOND(Simulator::Now().As(Time::S));
  Simulator::Schedule(Seconds(5), &LogProgress);
}

void GetKPIs(NodeContainer c, int numNodes)
{
    double sumSent = 0;
    double sumRcvd = 0;
    double sumFwd = 0;
    double sumNodesSeen = 0;
    double sumInTime = 0;
    double sumLate = 0;

    // Loop through each node and retrieve SLF application statistics
    for (int i = 0; i < numNodes; i++)
    {
        auto slfApp = c.Get(i)->GetApplication(0)->GetObject<SLFFloodingApp>(); // Assuming SLFFloodingApp is the class for your SLF application

        
        // If SLFFloodingApp has additional statistics, accumulate them here
        // Example:
        sumNodesSeen += slfApp->GetNumSeenNodes();
        sumInTime += slfApp->GetNumUpdatesReceivedInTime();
        sumLate += slfApp->GetNumUpdatesReceivedLate();
        
        
        // Accumulate statistics
        sumSent += slfApp->GetNumSent();
        sumRcvd += slfApp->GetNumRcvd();
        sumFwd += slfApp->GetNumFwd();

        
    }
    
    // Calculate packet delivery ratio (P_D)
    double pd = sumNodesSeen / (numNodes * (numNodes - 1));

    // Calculate packet error rate (P_EX) for a 500ms delay
    double pe500 = sumLate / (sumInTime + sumLate);
    //double pdr = sumRcvd / sumSent;

    // Calculate PDR
    //double pdr = (uniqueSentControlSeqNos.size() > 0) ? static_cast<double>(uniqueReceivedControlSeqNos.size()) / uniqueSentControlSeqNos.size() : 0;


    // Log the KPIs
    //kpiLogger.CreateEntry(pd, pe500, sumSent, sumRcvd, sumFwd);
    NS_LOG_UNCOND(Simulator::Now().As(Time::S) << " P_D = " << pd << ", P_EX = " << pe500);
}

void ResetStats(Ptr<SLFFloodingApp> app)
{
    app->ResetStats();
}

int main(int argc, char *argv[])
{
    NS_LOG_UNCOND("START");
    double simTime = 10;      // seconds
    uint32_t packetSize = 100; // bytes
    uint32_t seed = 0;
    int numNodes = 300;
    int version = 12;
    double interval = 0.12; // seconds
    double decayFactor = 2.0;
    double Threshold = 150;
    double size = 5000;
    double speedMin = 22;
    double speedMax = 33;
    bool tracing = true;
    uint32_t nodesPerContainer = 5; // Default value

    CommandLine cmd(__FILE__);
    cmd.AddValue("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue("interval", "interval (seconds) between packets", interval);
    cmd.AddValue("seed", "seed", seed);
    cmd.AddValue("numNodes", "numNodes", numNodes);
    cmd.AddValue("size", "size", size);
    cmd.AddValue("decayFactor", "decayFactor", decayFactor);
    cmd.AddValue("Threshold", "Threshold for SLF protocol", Threshold);
    cmd.AddValue("v", "v", version);
    cmd.AddValue("simTime", "simTime", simTime);
    cmd.AddValue("speedMax", "speedMax", speedMax);
    cmd.AddValue("speedMin", "speedMin", speedMin);
    cmd.AddValue("tracing", "tracing", tracing);
    cmd.AddValue("nodesPerContainer", "Number of nodes per container A, B, and C", nodesPerContainer);
    cmd.Parse(argc, argv);

    //kpiLogger.SetFile("res/v" + to_string(version) + "/kpi_rdf_n" + to_string(numNodes) + "_i" + to_string(int(interval * 1000)) + "_q" + to_string(int(decayFactor * 100)) + "_r" + to_string(seed) + ".csv");
    ns3::SeedManager::SetSeed(seed + 10);
  
    resLogger.SetFile("res/e/slfs_" + to_string(numNodes) + "_i" + to_string(int(interval * 1000)) + "_q" + to_string(int(decayFactor * 100)) + "_r" + to_string(seed) +"_a_"+ to_string(Threshold)+"_nc_"+to_string(nodesPerContainer)+".csv");
    //courseLogger.SetFile("res/v" + to_string(version) + "/course_rdf_n" + to_string(numNodes) + "_i" + to_string(int(interval * 1000)) + "_q" + to_string(int(decayFactor * 100)) + "_r" + to_string(seed) + ".csv");
  

    

    // Convert to time object
    Time interPacketInterval = Seconds(interval);

    NodeContainer c;
    c.Create(numNodes);
  
  

    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;

    string phyMode = "OfdmRate3MbpsBW10MHz"; // OfdmRate3MbpsBW10MHz
    wifi.SetStandard(WIFI_STANDARD_80211p);

    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    YansWifiPhyHelper wifiPhy;
    wifiPhy.Set("RxGain", DoubleValue(0));
    wifiPhy.Set("RxSensitivity", DoubleValue(-85));
    wifiPhy.Set("ChannelWidth", UintegerValue(10));
    wifiPhy.Set("TxPowerStart", DoubleValue(20));
    wifiPhy.Set("TxPowerEnd", DoubleValue(20));
    // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    // wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue(5.90e9));
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode), "ControlMode", StringValue(phyMode));
    // Set it to adhoc mode
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);
    
  
   
    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + to_string(size) + "]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + to_string(size) + "]"));

    Ptr<PositionAllocator> posAlloc = pos.Create()->GetObject<PositionAllocator>();

    MobilityHelper mobility;

    mobility.SetMobilityModel("ns3::RandomDirection2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(0, size, 0, size)),
                              "Speed", StringValue("ns3::UniformRandomVariable[Min=" + to_string(speedMin) + "|Max=" + to_string(speedMax) + "]"),
                              "Pause", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

    mobility.SetPositionAllocator(posAlloc);

    

NodeContainer mobileNodes;
for (uint32_t i = 0; i < c.GetN(); ++i) {
    if (i != 297 && i != 298 && i != 299) { // Correctly exclude nodes 297, 298, and 299
        mobileNodes.Add(c.Get(i));
    }
}

mobility.Install(mobileNodes);
  

//'size' is the length of the sides of square area

// Assuming 'c' is a NodeContainer with at least 300 nodes, where IDs 297, 298, and 299 correspond to ground stations A, B, and C respectively
 std::vector<uint32_t> groundStationIds = {297,298,299}; // Vector to store node IDs

    MobilityHelper fixedmobility;
    fixedmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    NodeContainer groundstationA, groundstationB, groundstationC;

    // Add nodes to individual containers using the vector
    groundstationA.Add(c.Get(groundStationIds[0])); // Node A
    groundstationB.Add(c.Get(groundStationIds[1])); // Node B
    groundstationC.Add(c.Get(groundStationIds[2])); // Node C



    // Calculate positions for A, B, and C
    double d = size / 3.0; // Distance from the center to a vertex of the triangle
    double centerX = size / 2.0;
    double centerY = size / 2.0;

    Vector posA(centerX, centerY + d, 0); // New position for A
    Vector posB(centerX + d * std::sqrt(3) / 2, centerY - d / 2, 0); // New position for B
    Vector posC(centerX - d * std::sqrt(3) / 2, centerY - d / 2, 0); // New position for C

    // Create a position allocator and assign positions to A, B, and C
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(posA); // Position for A
    positionAlloc->Add(posB); // Position for B
    positionAlloc->Add(posC); // Position for C

    // Setup mobility model
    fixedmobility.SetPositionAllocator(positionAlloc);

    // Install mobility model on each ground station
    fixedmobility.Install(groundstationA);
    fixedmobility.Install(groundstationB);
    fixedmobility.Install(groundstationC);

    


    
    
    // Add nodes with fixed positions


   // Create nodes


// Create a mobility helper


/*
MobilityHelper mobility;

// Create a position allocator
Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

// Add positions for nodes
positionAlloc->Add(Vector(0.0, 0.0, 0.0));        // Node 1 position (0, 0, 0)
positionAlloc->Add(Vector(100.0, 0.0, 0.0));     // Node 2 position (100, 0, 0)
positionAlloc->Add(Vector(120, 0.0, 0.0));     // Node 3 position (470, 0, 0)
positionAlloc->Add(Vector(140.0, 0.0, 0.0));     // Node 4 position (200, 0, 0)
positionAlloc->Add(Vector(600.0, 0.0, 0.0)); // Node 5 position (9000000, 0, 0)

// Set position allocator for mobility helper
mobility.SetPositionAllocator(positionAlloc);

// Install mobility models for each node
for (uint32_t i = 0; i < c.GetN(); ++i) {
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(c.Get(i));
}

*/


    InternetStackHelper internet;
    internet.Install(c);

    Ipv4AddressHelper ipv4;
    NS_LOG_INFO("Assign IP Addresses.");
    ipv4.SetBase("10.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    SLFFloodingAppHelper client(3000, interPacketInterval, Seconds(0.01), packetSize, 509.003, decayFactor, Threshold, nodesPerContainer);
    

    Ptr<UniformRandomVariable> startTimeRNG = CreateObject<UniformRandomVariable>();

    for (int i = 0; i < numNodes; i++)
    {
        ApplicationContainer apps = client.Install(c.Get(i));
        auto appG = c.Get(i)->GetApplication(0)->GetObject<SLFFloodingApp>();
        appG->SetGroundStationIds(groundStationIds);
        apps.Start(Seconds(startTimeRNG->GetValue(0.0, 5.0))); //
        Simulator::ScheduleWithContext(c.Get(i)->GetId(), Seconds(5.0), &ResetStats, c.Get(i)->GetApplication(0)->GetObject<SLFFloodingApp>());
        /*if (tracing)
        {
            Simulator::ScheduleWithContext(c.Get(i)->GetId(), Seconds(0), &CourseChange, c.Get(i)->GetObject<MobilityModel>(), c.Get(i)->GetId());
        }*/

    }

    
        Config::Connect("/NodeList/*/ApplicationList/0/$ns3::SLFFloodingApp/Rx", MakeCallback(&OnPacketReceive));
        Config::Connect("/NodeList/*/ApplicationList/0/$ns3::SLFFloodingApp/Tx", MakeCallback(&OnPacketSent));
        Config::Connect("/NodeList/*/ApplicationList/0/$ns3::SLFFloodingApp/Fwd", MakeCallback(&OnPacketForward));
    
    // Enable logging for SLFFloodingApp at INFO level
    LogComponentEnable("SLFFloodingApp", LOG_LEVEL_INFO);
    
  



    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    GetKPIs(c, numNodes);

    Simulator::Destroy();
    NS_LOG_UNCOND("END");

    return 0;
}








