/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// SPDX-License-Identifier: NIST-Software

#ifndef NR_SL_TRACE_HELPER_H
#define NR_SL_TRACE_HELPER_H

#include <ns3/node-container.h>
#include <ns3/nstime.h>
#include <ns3/output-stream-wrapper.h>

namespace ns3
{

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

  private:
    /**
     * \brief Write a trace line and schedule recurrence of this method
     *
     * \param n Node pointer to trace
     * \param interval Time interval between tracing events
     */
    void TraceUePosition(Ptr<Node> n, Time interval);

    Ptr<OutputStreamWrapper> m_uePositionStream;
};

} // namespace ns3

#endif // NR_SL_TRACE_HELPER_H
