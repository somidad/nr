// Copyright (c) 2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "ul-scheduling-test.h"

#include "ns3/applications-module.h"
#include "ns3/config.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/nr-module.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UlSchedulingTestCase");

UlSchedulingTestSuite::UlSchedulingTestSuite()
    : TestSuite("nr-ul-scheduling-test", Type::SYSTEM)
{
    AddTestCase(new UlSchedulingTest(MilliSeconds(800), false), Duration::QUICK);
}

/**
 * @ingroup nr-test
 * Static variable for test initialization
 */
static UlSchedulingTestSuite m_UlSchedulingTestSuite; //!< Nr test suite

UlSchedulingTest::UlSchedulingTest(Time reverseTime, bool harqActive)
    : TestCase("UL transmissions Test Case")
{
    m_reverseTime = reverseTime;
    m_harqActive = harqActive;
}

UlSchedulingTest::~UlSchedulingTest()
{
}

void
UlSchedulingTest::ScheduleNextPacketTransmission(Ptr<Node> ue, uint32_t ueNum, Time nextTime)
{
    Simulator::Schedule(MilliSeconds(50),
                        &UlSchedulingTest::ScheduleNextPacketTransmission,
                        this,
                        ue,
                        ueNum,
                        nextTime + MilliSeconds(50));
}

void
UlSchedulingTest::ReverseUeDirection(Ptr<Node> ueNode, double speed)
{
    ueNode->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, -speed, 0));
}

void
UlSchedulingTest::DoRun()
{
    // Simulation parameters //
    Time simTime = MilliSeconds(2000); // 1100
    Time udpAppStartTimeUl = MilliSeconds(400);

    // Create base stations and mobile terminals //
    NodeContainer gNbNode;
    NodeContainer ueNode;
    gNbNode.Create(1);
    ueNode.Create(1);

    // Add Mobility
    Ptr<ConstantPositionMobilityModel> mobility = CreateObject<ConstantPositionMobilityModel>();
    Ptr<Node> gnb = gNbNode.Get(0);
    gnb->AggregateObject(mobility);
    mobility->SetPosition(Vector(0, 0, 10));

    MobilityHelper ueMobility;
    double speed = 150; // m/s
    ueMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install(ueNode);
    Ptr<Node> ue = ueNode.Get(0);
    ue->GetObject<MobilityModel>()->SetPosition(Vector(116, 116, 1.5));
    ue->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, speed, 0));

    Simulator::Schedule(m_reverseTime, &UlSchedulingTest::ReverseUeDirection, this, ue, speed);
    Simulator::Schedule(MilliSeconds(400),
                        &UlSchedulingTest::ScheduleNextPacketTransmission,
                        this,
                        ue,
                        1,
                        MilliSeconds(400));

    // Configure BandwidthParts //
    OperationBandInfo band0;
    band0.m_bandId = 0;
    // uint8_t bandMask = NrChannelHelper::INIT_PROPAGATION;
    auto totalBandwidth = 50e6;
    CcBwpCreator ccBwpCreator;
    CcBwpCreator::SimpleOperationBandConf bandConf(28e9, totalBandwidth, 1);
    band0 = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);

    // Setup NR //
    Ptr<NrPointToPointEpcHelper> nrEpcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(999999999));
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(QuasiOmniDirectPathBeamforming::GetTypeId()));
    nrHelper->SetEpcHelper(nrEpcHelper);

    // Error Model
    std::string errorModel = "ns3::NrEesmIrT2";
    nrHelper->SetUlErrorModel(errorModel);
    nrHelper->SetDlErrorModel(errorModel);

    nrHelper->SetSchedulerAttribute("EnableHarqReTx", BooleanValue(m_harqActive));

    // Setup Channel //
    Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper>();
    channelHelper->ConfigureFactories("RMa", "Default");
    channelHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false));
    ObjectFactory distanceBasedChannelFactory;
    distanceBasedChannelFactory.SetTypeId(
        DistanceBasedThreeGppSpectrumPropagationLossModel::GetTypeId());
    auto distanceBased3gpp =
        distanceBasedChannelFactory.Create<DistanceBasedThreeGppSpectrumPropagationLossModel>();
    distanceBased3gpp->SetChannelModelAttribute(
        "Frequency",
        DoubleValue(band0.GetBwpAt(0, 0)->m_centralFrequency));
    distanceBased3gpp->SetChannelModelAttribute("Scenario", StringValue("RMa"));
    auto specChannelBand0 = channelHelper->CreateChannel(NrChannelHelper::INIT_PROPAGATION);
    band0.GetBwpAt(0, 0)->SetChannel(specChannelBand0);

    // Install devices //
    NetDeviceContainer gnbDevices;
    NetDeviceContainer ueDevices;

    auto allBwps = CcBwpCreator::GetAllBwps({band0});

    NetDeviceContainer gnbDevice = nrHelper->InstallGnbDevice(gNbNode, allBwps);
    gnbDevices.Add(gnbDevice);
    nrHelper->GetGnbPhy(gnbDevices.Get(0), 0)->SetAttribute("TxPower", DoubleValue(35));

    NetDeviceContainer ueDevice = nrHelper->InstallUeDevice(ueNode, allBwps);
    ueDevices.Add(ueDevice);

    // Setup Internet //
    Ipv4InterfaceContainer ueVoiceIpIface;

    Ptr<Node> pgw = nrEpcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);

    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // connect a remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.000)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    internet.Install(ueNode);

    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(ueNode.Get(0)->GetObject<Ipv4>());
    ueStaticRouting->SetDefaultRoute(nrEpcHelper->GetUeDefaultGatewayAddress(), 1);

    ueVoiceIpIface = nrEpcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevices));

    // Attach UEs to the closest gNB //
    nrHelper->AttachToClosestGnb(ueDevices, gnbDevices);

    // Configure Traffic //
    Ptr<NrEpcTft> voiceTft = Create<NrEpcTft>();
    UdpClientHelper ulClient;
    ApplicationContainer serverApps;
    ApplicationContainer clientApps;

    // UL data
    uint16_t ulPort = 20000;

    // The server, that is the application which
    // is listening, is installed in the remote host (UL)
    UdpServerHelper ulPacket(ulPort);
    serverApps.Add(ulPacket.Install(remoteHost));

    // Voice configuration and object creation:
    ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));
    ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(50))); // 1
    ulClient.SetAttribute("PacketSize", UintegerValue(10000));

    // The filter for the UL traffic (if it is DL this would be localPort)
    NrEpcTft::PacketFilter ulpf;
    ulpf.remotePortStart = ulPort;
    ulpf.remotePortEnd = ulPort;
    voiceTft->Add(ulpf);

    // The client, who is transmitting, is installed in the UE (UL data),
    // with destination address set to the address of the remoteHost
    ulClient.SetAttribute("RemoteAddress", AddressValue(remoteHostAddr));
    clientApps.Add(ulClient.Install(ueNode.Get(0)));

    // Activate a dedicated bearer for the traffic type
    // The bearer that will carry voice traffic
    NrEpsBearer voiceBearer(NrEpsBearer::GBR_CONV_VOICE);
    nrHelper->ActivateDedicatedEpsBearer(ueDevices.Get(0), voiceBearer, voiceTft);

    serverApps.Start(udpAppStartTimeUl);
    clientApps.Start(udpAppStartTimeUl);
    serverApps.Stop(simTime);
    clientApps.Stop(simTime);

    Simulator::Stop(simTime);
    Simulator::Run();

    Simulator::Destroy();
}

} // namespace ns3
