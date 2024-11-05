/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// SPDX-License-Identifier: NIST-Software

#ifndef NR_SL_TRACE_HELPER_H
#define NR_SL_TRACE_HELPER_H

#include <ns3/callback.h>
#include <ns3/node-container.h>
#include <ns3/nr-sl-phy-mac-common.h>
#include <ns3/nr-sl-ue-mac-scheduler-fixed-mcs.h>
#include <ns3/nr-sl-ue-mac-scheduler.h>
#include <ns3/nr-sl-ue-mac.h>
#include <ns3/nstime.h>
#include <ns3/output-stream-wrapper.h>

#include <ns3/ptr.h>
#include <ns3/sfnsf.h>

#include <string>
#include <unordered_map>

namespace ns3
{

class NrSlUeMac;
class OutputStreamWrapper;

class NrSlTraceHelper
{
  public:
    /**
     * \brief NrSlTraceHelper
     */
    NrSlTraceHelper();
    /**
     * \brief Generate NrSlUePositionTrace.txt that polls UE positions each second
     *
     * By default, the traced file will be named "NrSlUePositionTrace.txt" unless
     * another trace file name is provided.
     *
     * \param n NodeContainer with the UE nodes to trace
     * \param interval Time interval between tracing
     * \param traceFileName Optional trace file name for output
     */
    void TraceUePositions(NodeContainer n,
                          Time interval = Seconds(1),
                          std::string traceFileName = "");
    /**
     * \brief Generate a trace file of the operation of the sensing algorithm
     *
     * \param mac The NrSlUeMac instance to trace
     * \param traceFileName Trace file name
     */
    void TraceSensingAlgorithm(Ptr<NrSlUeMac> mac, std::string traceFileName);
    /**
     * \brief Generate a trace file of the operation of the scheduling algorithm
     *
     * \param mac The NrSlUeMac instance to trace
     * \param traceFileName Trace file name
     */
    void TraceSchedulingAlgorithm(Ptr<NrSlUeMac> mac, std::string traceFileName);
  private:
    /**
     * \brief Write a trace line and schedule recurrence of this method
     *
     * \param n Node pointer to trace
     * \param interval Time interval between tracing events
     */
    void TraceUePosition(Ptr<Node> n, Time interval);
    /**
     * Trace sink for sensing algorithm
     * \param context context for this trace
     * \param report sensing report.
     * \param candidateResources candidates found by the algorithm.
     * \param sensingData sensing input data.
     * \param transmitHistory transmit history.
     */
    void TraceSensing(std::string context,
                      const struct NrSlUeMac::SensingTraceReport& report,
                      const std::list<SlResourceInfo>& candidateResources,
                      const std::list<SensingData>& sensingData,
                      const std::list<SfnSf>& transmitHistory);

    /**
     * Trace sink for scheduling report
     * \param report the SchedulingReport
     * \param candidateResources Candidates returned from sensing algorithm
     * \param params TransmissionParams used as input to sensing algorithm
     * \param publishedResources Published grants
     * \param existingGrants Existing grants
     * \param newGrant New grant
     */
    void TraceScheduling(
        std::string context,
        const struct NrSlUeMacSchedulerFixedMcs::SchedulingReport& report,
        const std::list<SlResourceInfo>& candidateResources,
        const struct NrSlUeMac::NrSlTransmissionParams& params,
        const std::vector<SlGrantResource>& publishedResources,
        const std::map<uint32_t, std::vector<NrSlUeMacScheduler::GrantInfo>>& existingGrants,
        const struct NrSlUeMacScheduler::GrantInfo& newGrant);

    std::unordered_map<std::string, Ptr<OutputStreamWrapper>>
        m_traceSensingStreams; //!< container for output streams
    std::unordered_map<std::string, Ptr<OutputStreamWrapper>>
        m_traceSchedulingStreams; //!< container for output streams

    Ptr<OutputStreamWrapper> m_uePositionStream;
};

} // namespace ns3

#endif // NR_SL_TRACE_HELPER_H
