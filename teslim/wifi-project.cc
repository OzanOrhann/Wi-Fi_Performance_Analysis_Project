#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ssid.h"
#include "ns3/wifi-module.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPerformanceProject");

int
main(int argc, char* argv[])
{
    // The three things the project asks us to change, plus seed and time.
    uint32_t    numSTA           = 4;       // 4, 8, 12, 20
    std::string macMechanism     = "EDCA";  // "EDCA" or "RTSCTS"
    double      totalLoadPercent = 50.0;    // 50, 80, 90
    double      simTime          = 10.0;    // we measure for 10 seconds
    uint32_t    seed             = 1;
    bool        verbose          = false;

    CommandLine cmd;
    cmd.AddValue("numSTA",           "Number of STAs (4|8|12|20)",      numSTA);
    cmd.AddValue("macMechanism",     "MAC mechanism: EDCA or RTSCTS",   macMechanism);
    cmd.AddValue("totalLoadPercent", "Aggregate offered load in %",     totalLoadPercent);
    cmd.AddValue("simTime",          "Measurement duration (s)",        simTime);
    cmd.AddValue("seed",             "RNG seed",                        seed);
    cmd.AddValue("verbose",          "Verbose log",                     verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("WifiPerformanceProject", LOG_LEVEL_INFO);
    }

    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(1);

    // 72.2 Mbps is the top PHY rate for 802.11n at 20 MHz / 1 antenna
    // with short guard interval (MCS7). The load is a percentage of this.
    const double rawPhyMbps      = 72.2;
    const double totalOfferedMbps = rawPhyMbps * totalLoadPercent / 100.0;
    const double perStaMbps       = totalOfferedMbps / static_cast<double>(numSTA);

    std::ostringstream rateStream;
    rateStream << perStaMbps << "Mbps";
    const std::string perStaRate = rateStream.str();

    // TCP settings. NewReno is the classic flavor. Big buffers so
    // TCP can fill the Wi-Fi link.
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TcpNewReno::GetTypeId()));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::SndBufSize",  UintegerValue(1 << 22)); // 4 MB
    Config::SetDefault("ns3::TcpSocket::RcvBufSize",  UintegerValue(1 << 22));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));

    // Switch RTS/CTS on or off by changing the byte threshold.
    // Small threshold = almost every frame uses RTS/CTS.
    // Huge threshold  = nothing uses RTS/CTS.
    if (macMechanism == "RTSCTS")
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                           UintegerValue(100));
    }
    else
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                           UintegerValue(65535));
    }

    // Make the AP and the STA nodes.
    NodeContainer apNode;
    apNode.Create(1);
    NodeContainer staNodes;
    staNodes.Create(numSTA);

    // Wireless channel and PHY. Use 2.4 GHz, channel 1, 20 MHz wide.
    YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    YansWifiPhyHelper     phy;
    phy.SetChannel(channelHelper.Create());
    phy.Set("ChannelSettings", StringValue("{1, 20, BAND_2_4GHZ, 0}"));

    // Turn on 802.11n with the Minstrel rate algorithm (the project
    // asks for Minstrel; for 11n we need the HT version).
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    WifiMacHelper mac;
    Ssid          ssid = Ssid("wifi-project-bss");

    // Frame aggregation (A-MPDU): pack many small frames into one
    // big PPDU. 65535 bytes is the 802.11n maximum. Block-ACK comes
    // along for free (the standard says A-MPDU must use Block-ACK).
    // EDCA is the default MAC in 802.11n, so we don't need to set it.
    const uint32_t maxAmpdu = 65535;
    const uint16_t maxAmsdu = 7935;

    mac.SetType("ns3::StaWifiMac",
                "Ssid",            SsidValue(ssid),
                "BE_MaxAmpduSize", UintegerValue(maxAmpdu),
                "BE_MaxAmsduSize", UintegerValue(maxAmsdu));
    NetDeviceContainer staDevices = wifi.Install(phy, mac, staNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid",                SsidValue(ssid),
                "EnableBeaconJitter",  BooleanValue(false),
                "BE_MaxAmpduSize",     UintegerValue(maxAmpdu),
                "BE_MaxAmsduSize",     UintegerValue(maxAmsdu));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, apNode);

    // Turn on the short guard interval (400 ns instead of 800 ns).
    // This is what gives us 72.2 Mbps at MCS7.
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/"
                "ShortGuardIntervalSupported",
                BooleanValue(true));

    // Open up the TXOP. By default it's 0, meaning one PPDU per turn.
    // With 3.2 ms the device can send a few PPDUs back-to-back when
    // it gets the channel, which is what TXOP is really for.
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_Txop/"
                "TxopLimits",
                StringValue("3200us"));

    // Put the AP at (0,0) and spread the STAs evenly on a 1-meter
    // circle around it. Being so close means they all reach MCS7.
    MobilityHelper           mobility;
    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();
    pos->Add(Vector(0.0, 0.0, 0.0)); // AP
    for (uint32_t i = 0; i < numSTA; ++i)
    {
        const double angle = 2.0 * M_PI * static_cast<double>(i) / numSTA;
        pos->Add(Vector(std::cos(angle), std::sin(angle), 0.0));
    }
    mobility.SetPositionAllocator(pos);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode);
    mobility.Install(staNodes);

    // Standard IP stack on every node, give them 10.1.1.x addresses.
    InternetStackHelper internet;
    internet.Install(apNode);
    internet.Install(staNodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apIface  = ipv4.Assign(apDevice);
    Ipv4InterfaceContainer staIface = ipv4.Assign(staDevices);

    // Uplink TCP traffic: each STA sends data to the AP. The AP
    // runs a "sink" that just receives and counts the bytes.
    const uint16_t port = 50000;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(apNode.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime + 2.0));

    // Start the senders at t=1s to give Wi-Fi time to associate,
    // and stop them after the 10-second measurement window.
    const double appStart = 1.0;
    const double appStop  = appStart + simTime;

    for (uint32_t i = 0; i < numSTA; ++i)
    {
        OnOffHelper onoff("ns3::TcpSocketFactory",
                          InetSocketAddress(apIface.GetAddress(0), port));
        onoff.SetAttribute("OnTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute("DataRate",   DataRateValue(DataRate(perStaRate)));
        onoff.SetAttribute("PacketSize", UintegerValue(1448));
        ApplicationContainer app = onoff.Install(staNodes.Get(i));
        app.Start(Seconds(appStart));
        app.Stop(Seconds(appStop));
    }

    // Run the simulation.
    Simulator::Stop(Seconds(simTime + 2.0));
    Simulator::Run();

    // Total bytes the AP received / 10 seconds = throughput in Mbps.
    Ptr<PacketSink> sink       = DynamicCast<PacketSink>(sinkApp.Get(0));
    const uint64_t  rxBytes    = sink->GetTotalRx();
    const double    thrMbps    = (rxBytes * 8.0) / simTime / 1e6;

    // Print one CSV line. run_all.sh collects these from all 120 runs.
    std::cout << numSTA << "," << macMechanism << ","
              << totalLoadPercent << ","
              << std::fixed << std::setprecision(3) << thrMbps << std::endl;

    Simulator::Destroy();
    return 0;
}
