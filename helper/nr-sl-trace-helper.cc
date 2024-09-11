/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// SPDX-License-Identifier: NIST-Software

#include "nr-sl-trace-helper.h"

#include <ns3/log.h>
#include <ns3/mobility-model.h>
#include <ns3/simulator.h>
#include <ns3/trace-helper.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrSlTraceHelper");

NrSlTraceHelper::NrSlTraceHelper()
{
    NS_LOG_FUNCTION(this);
}

void
NrSlTraceHelper::TraceUePositions(NodeContainer n, Time interval, std::string traceFileName)
{
    NS_LOG_FUNCTION(this << &n << interval.As(Time::S) << traceFileName);
    AsciiTraceHelper ascii;
    if (traceFileName.empty())
    {
        m_uePositionStream = ascii.CreateFileStream("NrSlUePositionTrace.txt");
    }
    else
    {
        m_uePositionStream = ascii.CreateFileStream(traceFileName);
    }
    // UE positions tracing
    *m_uePositionStream->GetStream() << "Time (s)\tnodeId\tx\ty\tz" << std::endl;
    for (uint32_t i = 0; i < n.GetN(); ++i)
    {
        TraceUePosition(n.Get(i), interval);
    }
}

void
NrSlTraceHelper::TraceUePosition(Ptr<Node> n, Time interval)
{
    NS_LOG_FUNCTION(this);
    const auto mobility = n->GetObject<MobilityModel>();
    if (mobility)
    {
        const auto position = mobility->GetPosition();
        *m_uePositionStream->GetStream()
            << "\t" << Simulator::Now().GetSeconds() << "\t" << n->GetId() << "\t" << position.x
            << "\t" << position.y << "\t" << position.z << std::endl;
        Simulator::Schedule(interval, &NrSlTraceHelper::TraceUePosition, this, n, interval);
    }
}

} // namespace ns3
