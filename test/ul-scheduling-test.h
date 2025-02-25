// Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "ns3/mobility-model.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/nr-module.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/sfnsf.h"
#include "ns3/test.h"

namespace ns3
{

/**
 * @file ul-scheduling-test.h
 * @ingroup test
 *
 * This test evaluates the uplink packet transmission from a UE to a gNB when the UE has mobility.
 * The UE starts close to the gNB, and under favorable conditions (no other UEs causing
 * interference), it moves away from the gNB and then returns. The purpose of the test is to check
 * whether the gNB continues to receive packets from the UE during this mobility, even after the UE
 * returns close to the gNB, or if the system stops receiving them.
 *
 */

/**
 * @brief Test suite for
 *
 * @sa ns3::UlSchedulingTestCase
 */
class UlSchedulingTestSuite : public TestSuite
{
  public:
    UlSchedulingTestSuite();
};

/**
 * @ingroup test
 * @brief Testing UL transmissions
 */
class UlSchedulingTest : public TestCase
{
  public:
    /**
     * @brief UlSchedulingTest is a test constructor which is used to initialise
     *        the test parameters.
     * @param reverseTime the time instant when the UE starts approaching the gNB
     * @param harqActive true when HARQ is active
     * @param startUEPosY the starting position of the UE
     * @param simTime the simulation time
     * @param speed the speed at which the UE moves
     * @param packetPeriod the packet transmission periodicity of the UE
     * @param packetSize the size of the transmitted packet
     */
    UlSchedulingTest(Time reverseTime,
                     bool harqActive,
                     uint32_t startUEPosY,
                     Time simTime,
                     double speed,
                     Time packetPeriod,
                     uint32_t packetSize);
    /**
     * @brief ~SystemSchedulerTest
     */
    ~UlSchedulingTest() override;

  private:
    void DoRun() override;

    void ReverseUeDirection(Ptr<Node> ueNode);
    void ShowScheduledNextPacketTransmission(Ptr<Node> ue, uint32_t ueNum);

    void UeMacStateMachine(SfnSf sfn,
                           uint16_t nodeId,
                           uint16_t rnti,
                           uint8_t ccId,
                           NrUeMac::SrBsrMachine m_srState);

    Time m_simTime;     ///< the simulation time (milliseconds)
    double m_speed;     ///< the speed at which the UE moves (meters/s)
    Time m_reverseTime; ///< time instant when the UE starts approaching the gNB (ms)

    uint32_t m_startUEPosY; ///< the starting position of the UE (meters)
    Time m_packetPeriod;    ///< the periodicity of packet transmission (ms)
    uint32_t m_packetSize;  ///< the size of the transmitted packet

    bool m_harqActive; ///< true if HARQ is active

    Time m_nextTime; ///< the next packet transmission time
};

} // namespace ns3
