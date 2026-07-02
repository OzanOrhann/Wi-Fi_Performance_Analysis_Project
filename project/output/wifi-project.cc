/*
 * BLM 4140 - Wireless & Mobile Networks - Spring 2026
 * Wi-Fi Performance Analysis Project
 *
 * IEEE 802.11n 2.4 GHz, 20 MHz, 1x1 MIMO, short GI
 * (Project text says "802.11ac" but the instructor's correction (14 May post)
 *  states it must be 802.11n 2.4 GHz PHY using 11n rates.)
 *
 * Topology : 1 AP + N STAs (BSS), all STAs within 1 m of AP.
 * Traffic  : Uplink TCP, OnOff at a configured aggregate offered load.
 * MAC      : EDCA + A-MPDU (frame aggregation) + Block ACK (auto with AMPDU)
 *            + explicit TXOP (BE TxopLimit = 3.2 ms).
 * Rate Mgr : MinstrelHt (highest MCS reached since STAs are 1 m from AP).
 * Build for : NS-3.40 / 3.41
 *
 * Output: one CSV line on stdout:
 *   numSTA,macMechanism,totalLoadPercent,throughputMbps
 */

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
    // ---------------- Parameters ----------------
    uint32_t    numSTA           = 4;       // 4, 8, 12, 20
    std::string macMechanism     = "EDCA";  // "EDCA" or "RTSCTS"
    double      totalLoadPercent = 50.0;    // 50, 80, 90
    double      simTime          = 10.0;    // seconds (measurement window)
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

    // Raw PHY rate for 802.11n, 2.4 GHz, 20 MHz, 1 spatial stream,
    // short GI (400 ns), MCS7  ==>  72.2 Mbps.  This is the reference
    // used to compute the aggregate offered load.
    const double rawPhyMbps      = 72.2;
    const double totalOfferedMbps = rawPhyMbps * totalLoadPercent / 100.0;
    const double perStaMbps       = totalOfferedMbps / static_cast<double>(numSTA);

    std::ostringstream rateStream;
    rateStream << perStaMbps << "Mbps";
    const std::string perStaRate = rateStream.str();

    // ---------------- TCP tuning ----------------
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TcpNewReno::GetTypeId()));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::SndBufSize",  UintegerValue(1 << 22)); // 4 MB
    Config::SetDefault("ns3::TcpSocket::RcvBufSize",  UintegerValue(1 << 22));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));

    // ---------------- RTS/CTS ----------------
    // Force RTS/CTS for nearly every data frame by setting a very small
    // threshold; otherwise disable RTS/CTS by using the max threshold.
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

    // ---------------- Nodes ----------------
    NodeContainer apNode;
    apNode.Create(1);
    NodeContainer staNodes;
    staNodes.Create(numSTA);

    // ---------------- Channel & PHY ----------------
    YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    YansWifiPhyHelper     phy;
    phy.SetChannel(channelHelper.Create());
    // {channel number, channel width MHz, band, primary channel index}
    // Channel 1 of the 2.4 GHz band, 20 MHz wide.
    phy.Set("ChannelSettings", StringValue("{1, 20, BAND_2_4GHZ, 0}"));

    // ---------------- WiFi (802.11n) ----------------
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    WifiMacHelper mac;
    Ssid          ssid = Ssid("wifi-project-bss");

    // Enable A-MPDU (frame aggregation) with the standard maximum size for
    // 802.11n (65 535 B).  Block ACK is mandatory when A-MPDU is used,
    // so it is enabled automatically.  EDCA is the default 802.11n MAC.
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

    // Enable HT short GI on every WifiNetDevice.
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/"
                "ShortGuardIntervalSupported",
                BooleanValue(true));

    // Explicit TXOP for AC_BE.  The IEEE 802.11n default TxopLimit for
    // AC_BE is 0, which means each channel access carries exactly one
    // PPDU.  Setting it to 3.2 ms lets the AP/STA burst several
    // aggregated PPDUs inside one transmission opportunity, which is
    // the canonical use of TXOP in the modern 802.11n EDCA profile.
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_Txop/"
                "TxopLimits",
                StringValue("3200us"));

    // ---------------- Mobility ----------------
    // AP at origin, STAs uniformly arranged on a 1 m circle around it.
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

    // ---------------- Internet stack ----------------
    InternetStackHelper internet;
    internet.Install(apNode);
    internet.Install(staNodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apIface  = ipv4.Assign(apDevice);
    Ipv4InterfaceContainer staIface = ipv4.Assign(staDevices);

    // ---------------- Applications: uplink TCP ----------------
    const uint16_t port = 50000;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(apNode.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime + 2.0));

    const double appStart = 1.0;                  // s
    const double appStop  = appStart + simTime;   // s  (measurement window)

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

    // ---------------- Run ----------------
    Simulator::Stop(Seconds(simTime + 2.0));
    Simulator::Run();

    Ptr<PacketSink> sink       = DynamicCast<PacketSink>(sinkApp.Get(0));
    const uint64_t  rxBytes    = sink->GetTotalRx();
    const double    thrMbps    = (rxBytes * 8.0) / simTime / 1e6;

    // Single CSV record on stdout (the run_all script aggregates these).
    std::cout << numSTA << "," << macMechanism << ","
              << totalLoadPercent << ","
              << std::fixed << std::setprecision(3) << thrMbps << std::endl;

    Simulator::Destroy();
    return 0;
}
