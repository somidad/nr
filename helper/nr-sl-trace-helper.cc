/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// SPDX-License-Identifier: NIST-Software

#include "nr-sl-trace-helper.h"

#include <ns3/log.h>
#include <ns3/mobility-model.h>
#include <ns3/nr-sl-ue-mac.h>
#include <ns3/output-stream-wrapper.h>
#include <ns3/pointer.h>
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

void
NrSlTraceHelper::TraceSensingAlgorithm(Ptr<NrSlUeMac> mac, std::string traceFileName)
{
    NS_LOG_FUNCTION(this << mac << traceFileName);
    // Each Mac can be uniquely identified by the tuple of its IMSI and its BwpId
    std::string context =
        std::to_string(mac->GetImsi()) + std::string{":"} + std::to_string(mac->GetBwpId());
    NS_ABORT_MSG_IF(m_traceSensingStreams.contains(context),
                    "This Mac already enabled for sensing trace");
    AsciiTraceHelper ascii;
    m_traceSensingStreams.emplace(context, ascii.CreateFileStream(traceFileName));
    mac->TraceConnect("SensingAlgorithm",
                      context,
                      MakeCallback(&NrSlTraceHelper::TraceSensing, this));
}

void
NrSlTraceHelper::TraceSchedulingAlgorithm(Ptr<NrSlUeMac> mac, std::string traceFileName)
{
    NS_LOG_FUNCTION(this << mac << traceFileName);
    // Each Mac can be uniquely identified by the tuple of its IMSI and its BwpId
    std::string context =
        std::to_string(mac->GetImsi()) + std::string{":"} + std::to_string(mac->GetBwpId());
    NS_ABORT_MSG_IF(m_traceSchedulingStreams.contains(context),
                    "This Mac already enabled for scheduling trace");
    AsciiTraceHelper ascii;
    m_traceSchedulingStreams.emplace(context, ascii.CreateFileStream(traceFileName));
    PointerValue schedPtr;
    mac->GetAttribute("NrSlUeMacScheduler", schedPtr);
    auto sched = schedPtr.GetObject()->GetObject<NrSlUeMacSchedulerFixedMcs>();
    sched->TraceConnect("SchedulingReport",
                        context,
                        MakeCallback(&NrSlTraceHelper::TraceScheduling, this));
}

void
NrSlTraceHelper::TraceSensing(std::string context,
                              const struct NrSlUeMac::SensingTraceReport& report,
                              const std::list<SlResourceInfo>& candidateResources,
                              const std::list<SensingData>& sensingData,
                              const std::list<SfnSf>& transmitHistory)
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_UNLESS(m_traceSensingStreams.contains(context), "Context not found");
    auto stream = m_traceSensingStreams[context]->GetStream();

    struct ResourceRecord
    {
        bool m_transmitted{false};
        bool m_sensed{false};
        double m_rsrp{-200};
        bool m_candidate{false};
    };

    // The zeroth entry is at slot (report.m_sfn - report.m_t0 + 1)
    uint32_t zerothSlot = report.m_sfn.Normalize() - report.m_t0 + 1;
    // The length of the window is (report.m_sfn - report.m_t0 + 1) to (report.m_sfn + report.m_t2)
    // or report.m_t0 + report.m_t2 - 1
    uint32_t window = report.m_t0 + report.m_t2 - 1;
    std::vector<std::vector<ResourceRecord>> resources(
        window,
        std::vector<ResourceRecord>(report.m_subchannels));
    NS_LOG_DEBUG("Time " << Now().GetSeconds() << " Node " << context);
    NS_LOG_DEBUG("    Sensing report");
    NS_LOG_DEBUG("      Sfn " << report.m_sfn.Normalize());
    NS_LOG_DEBUG("      zeroth " << zerothSlot);
    NS_LOG_DEBUG("      Subch " << report.m_subchannels);
    NS_LOG_DEBUG("      T0 " << report.m_t0);
    NS_LOG_DEBUG("      T2 " << report.m_t2);
    NS_LOG_DEBUG("      Tproc0 " << +report.m_tProc0);
    NS_LOG_DEBUG("    Candidates");
    for (const auto& it : candidateResources)
    {
        uint32_t index = it.sfn.Normalize() - zerothSlot;
        NS_ASSERT_MSG(index < window, "Index out of bounds error");

        NS_LOG_DEBUG("      " << it.sfn.Normalize() << " " << index << " " << +it.slSubchannelStart
                              << " " << +it.slSubchannelLength);
        for (uint32_t j = it.slSubchannelStart; j < it.slSubchannelStart + it.slSubchannelLength;
             j++)
        {
            resources[index][j].m_candidate = true;
        }
    }
    NS_LOG_DEBUG("    Sensing data");
    for (const auto& it : sensingData)
    {
        NS_LOG_DEBUG("      " << it.sfn.Normalize() << " " << it.slRsrp << " " << +it.sbChStart
                              << " " << +it.sbChLength);
        uint32_t index = it.sfn.Normalize() - zerothSlot;
        NS_ASSERT_MSG(index < window, "Index out of bounds error");
        for (uint32_t j = it.sbChStart; j < (it.sbChStart + it.sbChLength); j++)
        {
            resources[index][j].m_sensed = true;
            resources[index][j].m_rsrp = it.slRsrp;
        }
    }
    NS_LOG_DEBUG("    Tx history");
    for (const auto& it : transmitHistory)
    {
        NS_LOG_DEBUG("      " << it.Normalize());
        uint32_t index = it.Normalize() - zerothSlot;
        NS_ASSERT_MSG(index < window, "Index out of bounds error");
        for (uint32_t j = 0; j < report.m_subchannels; j++)
        {
            resources[index][j].m_transmitted = true;
        }
    }
    *stream << "#Trace,time=" << Simulator::Now().GetSeconds()
            << ",slot=" << report.m_sfn.Normalize() << ",node=" << context << ",len=" << window
            << ",subch=" << report.m_subchannels << ",LsubCH=" << report.m_lSubch
            << ",t0=" << report.m_t0 << ",t1=" << +report.m_t1 << ",t2=" << report.m_t2
            << std::endl;
    for (uint32_t subch = report.m_subchannels; subch > 0; subch--)
    {
        for (uint32_t slot = 0; slot < window; slot++)
        {
            if (slot > 0)
            {
                *stream << ",";
            }
            if (resources[slot][subch - 1].m_sensed)
            {
                *stream << resources[slot][subch - 1].m_rsrp;
            }
            else if (resources[slot][subch - 1].m_transmitted)
            {
                *stream << "-200";
            }
            else if (resources[slot][subch - 1].m_candidate)
            {
                *stream << "200";
            }
            else
            {
                *stream << "0";
            }
        }
        *stream << std::endl;
    }
}

void
NrSlTraceHelper::TraceScheduling(
    std::string context,
    const struct NrSlUeMacSchedulerFixedMcs::SchedulingReport& report,
    const std::list<SlResourceInfo>& candidateResources,
    const struct NrSlUeMac::NrSlTransmissionParams& params,
    const std::vector<SlGrantResource>& publishedResources,
    const std::map<uint32_t, std::vector<NrSlUeMacScheduler::GrantInfo>>& existingGrants,
    const struct NrSlUeMacScheduler::GrantInfo& newGrant)
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_UNLESS(m_traceSchedulingStreams.contains(context), "Context not found");
    auto stream = m_traceSchedulingStreams[context]->GetStream();

    struct ResourceRecord
    {
        bool m_existingGrant{false};
        bool m_newGrant{false};
        bool m_unusedCandidate{false};
    };

    // window to plot ranges from [report.m_sfn to (report.m_sfn + report.m_t2)]
    uint32_t window = report.m_t2 + 1;
    std::vector<std::vector<ResourceRecord>> resources(
        window,
        std::vector<ResourceRecord>(report.m_subchannels));
    NS_LOG_DEBUG("Time " << Now().GetSeconds() << " Node " << context);
    NS_LOG_DEBUG("    Scheduling report");
    NS_LOG_DEBUG("      Sfn " << report.m_sfn.Normalize());
    NS_LOG_DEBUG("      subch " << report.m_subchannels);
    NS_LOG_DEBUG("      psfchPeriod " << report.m_psfchPeriod);
    NS_LOG_DEBUG("      cReselCounte " << newGrant.cReselCounter);
    NS_LOG_DEBUG("      slResoReselCounter " << +newGrant.slResoReselCounter);
    NS_LOG_DEBUG("      harqId " << +newGrant.harqId);
    NS_LOG_DEBUG("      harqEnabled " << newGrant.harqEnabled);
    NS_LOG_DEBUG("      nSelected " << +newGrant.nSelected);
    NS_LOG_DEBUG("      rri " << newGrant.rri.As(Time::MS));
    NS_LOG_DEBUG("    Candidates");
    for (const auto& it : candidateResources)
    {
        NS_ASSERT_MSG(report.m_sfn.Normalize() <= it.sfn.Normalize(), "Index out of bounds error");
        uint32_t index = it.sfn.Normalize() - report.m_sfn.Normalize();
        NS_ASSERT_MSG(index < window, "Index out of bounds error");

        NS_LOG_DEBUG("      " << it.sfn.Normalize() << " " << index << " " << +it.slSubchannelStart
                              << " " << +it.slSubchannelLength);
        for (uint32_t j = it.slSubchannelStart; j < it.slSubchannelStart + it.slSubchannelLength;
             j++)
        {
            resources[index][j].m_unusedCandidate = true;
        }
    }
    NS_LOG_DEBUG("    Published grants");
    for (const auto& it : publishedResources)
    {
        NS_ASSERT_MSG(report.m_sfn.Normalize() <= it.sfn.Normalize(), "Index out of bounds error");
        uint32_t index = it.sfn.Normalize() - report.m_sfn.Normalize();
        NS_LOG_DEBUG("      " << it.sfn.Normalize() << " " << index << " " << +it.slPsschSubChStart
                              << " " << +it.slPsschSubChLength);
        for (uint16_t j = it.slPsschSubChStart; j < it.slPsschSubChStart + it.slPsschSubChLength;
             j++)
        {
            resources[index][j].m_existingGrant = true;
        }
    }
    // Each item in published grants map is a dstL2Id and a grant.  The grant is a std::set of
    // SlGrantResources (slot allocations)
    NS_LOG_DEBUG("    Unpublished grants");
    for (const auto& it : existingGrants)
    {
        for (const auto& it2 : it.second)
        {
            for (const auto& it3 : it2.slotAllocations)
            {
                uint32_t index = it3.sfn.Normalize() - report.m_sfn.Normalize();
                if (index > window)
                {
                    continue;
                }
                NS_LOG_DEBUG("      " << it3.sfn.Normalize() << " " << index << " "
                                      << +it3.slPsschSubChStart << " " << +it3.slPsschSubChLength);
                for (uint16_t j = it3.slPsschSubChStart;
                     j < it3.slPsschSubChStart + it3.slPsschSubChLength;
                     j++)
                {
                    NS_ASSERT_MSG(!resources[index][j].m_existingGrant,
                                  "Overlap between published/unpublished grant");
                    resources[index][j].m_existingGrant = true;
                }
            }
        }
    }
    NS_LOG_DEBUG("    New grant");
    uint16_t lSubCh;
    for (const auto& it : newGrant.slotAllocations)
    {
        uint32_t index = it.sfn.Normalize() - report.m_sfn.Normalize();
        if (index > window)
        {
            continue;
        }
        NS_LOG_DEBUG("      " << it.sfn.Normalize() << " " << index << " " << +it.slPsschSubChStart
                              << " " << +it.slPsschSubChLength);
        for (uint16_t j = it.slPsschSubChStart; j < it.slPsschSubChStart + it.slPsschSubChLength;
             j++)
        {
            NS_ASSERT_MSG(!resources[index][j].m_existingGrant,
                          "Overlap between existing and new grant");
            lSubCh = it.slPsschSubChLength;
            resources[index][j].m_newGrant = true;
        }
    }
    *stream << "#Trace,time=" << Simulator::Now().GetSeconds()
            << ",slot=" << report.m_sfn.Normalize() << ",node=" << context << ",len=" << window
            << ",subch=" << report.m_subchannels << ",lSubCh=" << lSubCh
            << ",PsfchPeriod=" << report.m_psfchPeriod << ",nSelected=" << +newGrant.nSelected
            << ",rri=" << newGrant.rri.GetMilliSeconds() << ",harqEnabled=" << newGrant.harqEnabled
            << std::endl;
    for (uint32_t subch = report.m_subchannels; subch > 0; subch--)
    {
        for (uint32_t slot = 0; slot < window; slot++)
        {
            if (slot > 0)
            {
                *stream << ",";
            }
            if (resources[slot][subch - 1].m_newGrant)
            {
                *stream << "75";
            }
            else if (resources[slot][subch - 1].m_existingGrant)
            {
                *stream << "-75";
            }
            else if (resources[slot][subch - 1].m_unusedCandidate)
            {
                *stream << "-175";
            }
            else
            {
                *stream << "0";
            }
        }
        *stream << std::endl;
    }
}

} // namespace ns3
