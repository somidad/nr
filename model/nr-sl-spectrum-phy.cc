/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-sl-spectrum-phy.h"

#include "nr-lte-mi-error-model.h"
#include "nr-sl-mac-pdu-tag.h"
#include "nr-sl-sci-f1a-header.h"
#include "nr-sl-sci-f2a-header.h"
#include "nr-ue-net-device.h"
#include "nr-ue-phy.h"

#include <ns3/lte-radio-bearer-tag.h>
#include <ns3/node.h>

#include <unordered_set>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrSlSpectrumPhy");
NS_OBJECT_ENSURE_REGISTERED(NrSlSpectrumPhy);

NrSlSpectrumPhy::NrSlSpectrumPhy()
    : NrSpectrumPhy()
{
    NS_LOG_FUNCTION(this);
    m_slInterference = CreateObject<NrSlInterference>();
}

void
NrSlSpectrumPhy::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_phySlHarqFeedbackCallback = MakeNullCallback<void, const SlHarqInfo&>();
    m_slInterference->Dispose();
    m_slInterference = nullptr;
    m_slAmc = nullptr;
    m_nrPhyRxPscchEndOkCallback = nullptr;
    m_nrPhyRxPsschEndOkCallback = nullptr;
    m_nrPhyRxPsschEndErrorCallback = nullptr;
    NrSpectrumPhy::DoDispose();
}

TypeId
NrSlSpectrumPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrSlSpectrumPhy")
            .SetParent<NrSpectrumPhy>()
            .AddConstructor<NrSlSpectrumPhy>()
            .AddAttribute("SlErrorModelType",
                          "Type of the Error Model to be used for NR sidelink",
                          TypeIdValue(NrLteMiErrorModel::GetTypeId()),
                          MakeTypeIdAccessor(&NrSlSpectrumPhy::SetSlErrorModelType),
                          MakeTypeIdChecker())
            .AddAttribute("SlDataErrorModelEnabled",
                          "Activate/Deactivate the error model for the Sidelink PSSCH "
                          "decodification [by default is active].",
                          BooleanValue(true),
                          MakeBooleanAccessor(&NrSlSpectrumPhy::SetSlDataErrorModelEnabled),
                          MakeBooleanChecker())
            .AddAttribute("SlCtrlErrorModelEnabled",
                          "Activate/Deactivate the error model for the Sidelink PSCCH "
                          "decodification [by default is active].",
                          BooleanValue(true),
                          MakeBooleanAccessor(&NrSlSpectrumPhy::SetSlCtrlErrorModelEnabled),
                          MakeBooleanChecker())
            .AddAttribute(
                "DropTbOnRbOnCollision",
                "Activate/Deactivate the dropping colliding RBs regardless of SINR value.",
                BooleanValue(false),
                MakeBooleanAccessor(&NrSlSpectrumPhy::DropTbOnRbOnCollision),
                MakeBooleanChecker())
            .AddTraceSource("RxPscchTraceUe",
                            "The PSCCH transmission received by the User Device",
                            MakeTraceSourceAccessor(&NrSlSpectrumPhy::m_rxPscchTraceUe),
                            "ns3::SlRxCtrlPacketTraceParams::TracedCallback")
            .AddTraceSource("RxPsschTraceUe",
                            "The PSSCH transmission received by the User Device",
                            MakeTraceSourceAccessor(&NrSlSpectrumPhy::m_rxPsschTraceUe),
                            "ns3::SlRxDataPacketTraceParams::TracedCallback")
            .AddTraceSource("SlPscchDecodeFailure",
                            "The sidelink PSCCH transmission failed to decode",
                            MakeTraceSourceAccessor(&NrSlSpectrumPhy::m_slPscchDecodeFailures),
                            "ns3::TracedValueCallback::Uint64")
            .AddTraceSource("SlSci2aDecodeFailure",
                            "The sidelink SCI-2a transmission failed to decode",
                            MakeTraceSourceAccessor(&NrSlSpectrumPhy::m_slSci2aDecodeFailures),
                            "ns3::TracedValueCallback::Uint64")
            .AddTraceSource("SlTbDecodeFailure",
                            "The sidelink TB transmission failed to decode",
                            MakeTraceSourceAccessor(&NrSlSpectrumPhy::m_slTbDecodeFailures),
                            "ns3::TracedValueCallback::Uint64");
    return tid;
}

void
NrSlSpectrumPhy::SetPhySlHarqFeedbackCallback(const NrPhySlHarqFeedbackCallback& c)
{
    NS_LOG_FUNCTION(this);
    m_phySlHarqFeedbackCallback = c;
}

void
NrSlSpectrumPhy::SetNoisePowerSpectralDensity(const Ptr<const SpectrumValue>& noisePsd)
{
    NS_LOG_FUNCTION(this << noisePsd);
    NrSpectrumPhy::SetNoisePowerSpectralDensity(noisePsd);
    m_slInterference->SetNoisePowerSpectralDensity(noisePsd);
}

void
NrSlSpectrumPhy::StartRx(Ptr<SpectrumSignalParameters> params)
{
    NS_LOG_FUNCTION(this);
    Ptr<const SpectrumValue> rxPsd = params->psd;
    Time duration = params->duration;
    NS_LOG_INFO("Start receiving signal: " << rxPsd << " duration= " << duration);

    Ptr<NrSpectrumSignalParametersSlFrame> nrSlRxParams =
        DynamicCast<NrSpectrumSignalParametersSlFrame>(params);

    Ptr<NrSpectrumSignalParametersSlFeedback> nrSlRxFbParams =
        DynamicCast<NrSpectrumSignalParametersSlFeedback>(params);

    // pass it to Sidelink interference calculations regardless of the type (SL or non-Sl)
    m_slInterference->AddSignal(params->psd, params->duration);
    if (nrSlRxParams)
    {
        if (GetState() != TX)
        {
            // Half duplex SL
            StartRxSlFrame(nrSlRxParams);
        }
        else
        {
            NS_LOG_DEBUG("Ignoring the reception. Sidelink is half duplex. State : " << GetState());
        }
    }
    else if (nrSlRxFbParams)
    {
        if (GetState() != TX)
        {
            // Half duplex SL
            StartRxSlFrame(nrSlRxFbParams);
        }
        else
        {
            NS_LOG_DEBUG("Ignoring the reception. Sidelink is half duplex. State : " << GetState());
        }
    }
    NrSpectrumPhy::StartRx(params);
}

void
NrSlSpectrumPhy::SetSlErrorModelType(TypeId errorModelType)
{
    NS_LOG_FUNCTION(this);
    m_slErrorModelType = errorModelType;
}

void
NrSlSpectrumPhy::SetSlErrorModel(Ptr<NrErrorModel> slErrorModel)
{
    NS_LOG_FUNCTION(this << slErrorModel);
    m_slErrorModel = slErrorModel;
}

void
NrSlSpectrumPhy::DropTbOnRbOnCollision(bool drop)
{
    NS_LOG_FUNCTION(this << drop);
    m_dropTbOnRbCollisionEnabled = drop;
}

void
NrSlSpectrumPhy::SetSlDataErrorModelEnabled(bool slDataErrorModelEnabled)
{
    NS_LOG_FUNCTION(this << slDataErrorModelEnabled);
    m_slDataErrorModelEnabled = slDataErrorModelEnabled;
}

void
NrSlSpectrumPhy::SetSlCtrlErrorModelEnabled(bool slCtrlErrorModelEnabled)
{
    NS_LOG_FUNCTION(this << slCtrlErrorModelEnabled);
    m_slCtrlErrorModelEnabled = slCtrlErrorModelEnabled;
}

void
NrSlSpectrumPhy::AddSlSinrChunkProcessor(Ptr<NrSlChunkProcessor> p)
{
    NS_LOG_FUNCTION(this << p);
    m_slInterference->AddSinrChunkProcessor(p);
}

void
NrSlSpectrumPhy::AddSlSignalChunkProcessor(Ptr<NrSlChunkProcessor> p)
{
    NS_LOG_FUNCTION(this << p);
    m_slInterference->AddRsPowerChunkProcessor(p);
}

void
NrSlSpectrumPhy::UpdateSlSinrPerceived(std::vector<SpectrumValue> sinr)
{
    NS_LOG_FUNCTION(this);
    m_slSinrPerceived = sinr;
}

void
NrSlSpectrumPhy::UpdateSlSignalPerceived(std::vector<SpectrumValue> sig)
{
    NS_LOG_FUNCTION(this);
    m_slSigPerceived = sig;
}

void
NrSlSpectrumPhy::StartTxSlCtrlFrames(const Ptr<PacketBurst>& pb, Time duration)
{
    NS_LOG_FUNCTION(this << " state: " << GetState());

    switch (GetState())
    {
    case RX_DATA:
        /* no break */
    case RX_DL_CTRL:
        /* no break */
    case RX_UL_CTRL:
        NS_FATAL_ERROR("Cannot TX while RX.");
        break;
    case CCA_BUSY:
        NS_LOG_WARN("Start transmitting NR SL CTRL while in CCA_BUSY state.");
        /* no break */
    case IDLE: {
        NS_ASSERT(GetTxPowerSpectralDensity());

        ChangeState(TX, duration);

        Ptr<NrSpectrumSignalParametersSlCtrlFrame> txParams =
            Create<NrSpectrumSignalParametersSlCtrlFrame>();
        txParams->duration = duration;
        txParams->txPhy = GetObject<SpectrumPhy>();
        // ConstCast is needed because SpectrumSignalParameters needs a non-const Ptr
        txParams->psd = ConstCast<SpectrumValue>(GetTxPowerSpectralDensity());
        txParams->nodeId = GetDevice()->GetNode()->GetId();
        txParams->packetBurst = pb;

        NotifyTxCtrlTrace(duration);

        if (GetChannel())
        {
            GetChannel()->StartTx(txParams);
        }
        else
        {
            NS_LOG_WARN("Working without channel (i.e., under test)");
        }

        Simulator::Schedule(duration, &NrSlSpectrumPhy::EndTx, this);
        IncrementActiveTransmissions();
    }
    break;
    case TX: {
        NS_LOG_DEBUG("Start transmitting NR SL DATA while already in TX state.");
        NS_ASSERT(GetTxPowerSpectralDensity());

        // Do not change state to TX

        Ptr<NrSpectrumSignalParametersSlDataFrame> txParams =
            Create<NrSpectrumSignalParametersSlDataFrame>();
        txParams->duration = duration;
        txParams->txPhy = GetObject<SpectrumPhy>();
        // ConstCast is needed because SpectrumSignalParameters needs a non-const Ptr
        txParams->psd = ConstCast<SpectrumValue>(GetTxPowerSpectralDensity());
        txParams->nodeId = GetDevice()->GetNode()->GetId();
        txParams->packetBurst = pb;

        NotifyTxDataTrace(duration);

        if (GetChannel())
        {
            GetChannel()->StartTx(txParams);
        }
        else
        {
            NS_LOG_WARN("Working without channel (i.e., under test)");
        }

        // Do not schedule a new NrSpectrumPhy::EndTx
    }
    break;
    default:
        NS_FATAL_ERROR("Unknown state " << GetState() << " Code should not reach this point");
    }
}

void
NrSlSpectrumPhy::StartTxSlFeedback(const std::list<Ptr<NrSlHarqFeedbackMessage>>& feedbackList,
                                   const Time& duration)
{
    NS_LOG_FUNCTION(this << " state: " << GetState());

    switch (GetState())
    {
    case RX_DATA:
        /* no break */
    case RX_DL_CTRL:
        /* no break */
    case RX_UL_CTRL:
        NS_FATAL_ERROR("Cannot TX while RX.");
        break;
    case TX:
        NS_FATAL_ERROR("Cannot TX while already TX.");
        break;
    case CCA_BUSY:
        NS_LOG_WARN("Start transmitting NR SL FB while in CCA_BUSY state.");
        /* no break */
    case IDLE: {
        NS_ASSERT(GetTxPowerSpectralDensity());

        ChangeState(TX, duration);

        Ptr<NrSpectrumSignalParametersSlFeedback> txParams =
            Create<NrSpectrumSignalParametersSlFeedback>();
        txParams->duration = duration;
        txParams->txPhy = GetObject<SpectrumPhy>();
        // ConstCast is needed because SpectrumSignalParameters needs a non-const Ptr
        txParams->psd = ConstCast<SpectrumValue>(GetTxPowerSpectralDensity());
        txParams->nodeId = GetDevice()->GetNode()->GetId();
        txParams->feedbackList = feedbackList;

        NotifyTxFeedbackTrace(duration);

        if (GetChannel())
        {
            GetChannel()->StartTx(txParams);
        }
        else
        {
            NS_LOG_WARN("Working without channel (i.e., under test)");
        }

        Simulator::Schedule(duration, &NrSlSpectrumPhy::EndTx, this);
        IncrementActiveTransmissions();
    }
    break;
    default:
        NS_FATAL_ERROR("Unknown state " << GetState() << " Code should not reach this point");
    }
}

void
NrSlSpectrumPhy::StartTxSlDataFrames(const Ptr<PacketBurst>& pb, Time duration)
{
    NS_LOG_FUNCTION(this << " state: " << GetState());

    switch (GetState())
    {
    case RX_DATA:
        /* no break */
    case RX_DL_CTRL:
        /* no break */
    case RX_UL_CTRL:
        NS_FATAL_ERROR("Cannot TX while RX.");
        break;
    case TX:
        NS_FATAL_ERROR("Cannot TX while already TX.");
        break;
    case CCA_BUSY:
        NS_LOG_WARN("Start transmitting NR SL DATA while in CCA_BUSY state.");
        /* no break */
    case IDLE: {
        NS_ASSERT(GetTxPowerSpectralDensity());

        ChangeState(TX, duration);

        Ptr<NrSpectrumSignalParametersSlDataFrame> txParams =
            Create<NrSpectrumSignalParametersSlDataFrame>();
        txParams->duration = duration;
        txParams->txPhy = GetObject<SpectrumPhy>();
        // ConstCast is needed because SpectrumSignalParameters needs a non-const Ptr
        txParams->psd = ConstCast<SpectrumValue>(GetTxPowerSpectralDensity());
        txParams->nodeId = GetDevice()->GetNode()->GetId();
        txParams->packetBurst = pb;

        NotifyTxDataTrace(duration);

        if (GetChannel())
        {
            GetChannel()->StartTx(txParams);
        }
        else
        {
            NS_LOG_WARN("Working without channel (i.e., under test)");
        }

        Simulator::Schedule(duration, &NrSlSpectrumPhy::EndTx, this);
        IncrementActiveTransmissions();
    }
    break;
    default:
        NS_FATAL_ERROR("Unknown state " << GetState() << " Code should not reach this point");
    }
}

void
NrSlSpectrumPhy::StartRxSlFrame(Ptr<NrSpectrumSignalParametersSlFrame> params)
{
    NS_LOG_FUNCTION(this << " state: " << GetState());

    Ptr<NrSpectrumSignalParametersSlDataFrame> nrSlRxDataParams =
        DynamicCast<NrSpectrumSignalParametersSlDataFrame>(params);
    if (nrSlRxDataParams)
    {
        NotifyRxDataTrace(GetNrPhy()->GetCurrentSfnSf(),
                          params->psd,
                          params->duration,
                          GetNrPhy()->GetBwpId(),
                          GetNrPhy()->GetCellId());
    }

    switch (GetState())
    {
    case TX:
        NS_FATAL_ERROR("Cannot RX NR Sidelink frame while TX.");
        break;
    case RX_UL_CTRL:
        NS_FATAL_ERROR("Cannot RX NR Sidelink frame while receiving UL CTRL.");
        break;
    case RX_DL_CTRL:
        NS_FATAL_ERROR("Cannot RX NR Sidelink frame while receiving DL CTRL.");
        break;
    case CCA_BUSY:
        NS_LOG_WARN("Start receiving NR Sidelink frame while channel in CCA_BUSY state.");
        /* no break */
    case RX_DATA:
        /* no break */
    case IDLE: {
        // the behavior is similar when we're IDLE or in RX because we can
        // receive more signals simultaneously (e.g., at the gNB).
        NS_LOG_DEBUG("SL Signal is from Node id = " << params->nodeId);
        if (m_slRxSigParamInfo.empty())
        {
            NS_ASSERT(GetState() == IDLE);
            // first transmission, i.e., we're IDLE and we start RX
            SetFirstRxStart(Simulator::Now());
            SetFirstRxDuration(params->duration);
            NS_LOG_LOGIC("Scheduling EndRxSlFrame with delay " << params->duration.GetSeconds()
                                                               << "s");
            Simulator::Schedule(params->duration, &NrSlSpectrumPhy::EndRxSlFrame, this);
        }
        else
        {
            NS_ASSERT(GetState() == RX_DATA);
            // sanity check: if there are multiple RX events, they
            // should occur at the same time and have the same
            // duration, otherwise the interference calculation
            // won't be correct
            NS_ASSERT((GetFirstRxStart() == Simulator::Now()) &&
                      (GetFirstRxDuration() == params->duration));
        }
        ChangeState(RX_DATA, params->duration);
        m_slInterference->StartRx(params->psd);
        // keeping track of all the received signal, which received
        // at the same time.
        std::vector<int> rbMap;
        int rbI = 0;
        for (Values::const_iterator it = params->psd->ConstValuesBegin();
             it != params->psd->ConstValuesEnd();
             it++, rbI++)
        {
            if (*it != 0)
            {
                NS_LOG_INFO("NR Sidelink message arriving on RB " << rbI);
                rbMap.push_back(rbI);
            }
        }
        SlRxSigParamInfo signalInfo;
        signalInfo.params = params;
        signalInfo.rbBitmap = rbMap;
        m_slRxSigParamInfo.push_back(signalInfo);
        break;
    }
    default: {
        NS_FATAL_ERROR("unknown state");
        break;
    }
    }
}

void
NrSlSpectrumPhy::EndRxSlFrame()
{
    NS_LOG_FUNCTION(this << " state: " << GetState());

    m_slInterference->EndRx();

    // Extract the various types of NR Sidelink messages received
    std::vector<uint32_t> pscchIndexes;
    std::vector<uint32_t> psschIndexes;
    std::vector<uint32_t> psfchIndexes;

    for (std::size_t i = 0; i < m_slRxSigParamInfo.size(); i++)
    {
        Ptr<NrSpectrumSignalParametersSlFrame> params = m_slRxSigParamInfo.at(i).params;
        Ptr<NrSpectrumSignalParametersSlCtrlFrame> nrSlCtrlRxParams =
            DynamicCast<NrSpectrumSignalParametersSlCtrlFrame>(params);
        Ptr<NrSpectrumSignalParametersSlDataFrame> nrSlDataRxParams =
            DynamicCast<NrSpectrumSignalParametersSlDataFrame>(params);
        Ptr<NrSpectrumSignalParametersSlFeedback> nrSlFbRxParams =
            DynamicCast<NrSpectrumSignalParametersSlFeedback>(params);

        if (nrSlCtrlRxParams)
        {
            pscchIndexes.push_back(i);
        }
        else if (nrSlDataRxParams)
        {
            psschIndexes.push_back(i);
        }
        else if (nrSlFbRxParams)
        {
            psfchIndexes.push_back(i);
        }
        else
        {
            NS_FATAL_ERROR("Invalid NR Sidelink signal parameter type");
        }
    }

    if (!pscchIndexes.empty())
    {
        RxSlPscch(pscchIndexes);
    }
    if (!psschIndexes.empty())
    {
        RxSlPssch(psschIndexes);
    }
    if (psfchIndexes.size() > 0)
    {
        RxSlPsfch(psfchIndexes);
    }

    // clear received packets
    ChangeState(IDLE, Seconds(0));
    m_slRxSigParamInfo.clear();
}

void
NrSlSpectrumPhy::RxSlPscch(std::vector<uint32_t> paramIndexes)
{
    NS_LOG_FUNCTION(this << "Number of PSCCH messages:" << paramIndexes.size());

    Ptr<NrUeNetDevice> ueRx = DynamicCast<NrUeNetDevice>(GetDevice());

    // When control messages collide in the PSCCH, the receiver cannot know how many transmissions
    // occurred we sort the messages by SINR and try to decode the ones with highest average SINR
    // per RB first.
    std::list<PscchPduInfo> rxControlMessageOkList;
    bool error = true;
    std::multiset<SlCtrlSigParamInfo> sortedControlMessages;
    // container to store the RB indices of the collided TBs
    std::set<int> collidedRbBitmap;
    // container to store the RB indices of the decoded TBs
    std::set<int> rbDecodedBitmap;

    for (uint32_t i = 0; i < paramIndexes.size(); i++)
    {
        uint32_t paramIndex = paramIndexes.at(i);
        Ptr<NrSpectrumSignalParametersSlCtrlFrame> params =
            DynamicCast<NrSpectrumSignalParametersSlCtrlFrame>(
                m_slRxSigParamInfo.at(paramIndex).params);
        NS_ASSERT(params);
        Ptr<PacketBurst> pb [[maybe_unused]] = params->packetBurst;
        NS_LOG_LOGIC("Received PSCCH burst with " << pb->GetNPackets() << " packet(s)");
        auto sinrStats = GetSinrStats(m_slSinrPerceived[paramIndexes[i]],
                                      m_slRxSigParamInfo.at(paramIndex).rbBitmap);
        SlCtrlSigParamInfo sigInfo;
        sigInfo.sinrAvg = sinrStats.sinrAvg;
        sigInfo.sinrMin = sinrStats.sinrMin;
        sigInfo.index = paramIndex;
        sortedControlMessages.insert(sigInfo);
    }

    if (m_dropTbOnRbCollisionEnabled)
    {
        NS_LOG_DEBUG(this << "NR SL Ctrl DropTbOnRbOnCollision");
        // Add new loop to make one pass and identify which RB have collisions
        std::set<int> collidedRbBitmapTemp;

        for (std::multiset<SlCtrlSigParamInfo>::iterator it = sortedControlMessages.begin();
             it != sortedControlMessages.end();
             it++)
        {
            uint32_t pktIndex = (*it).index;
            for (std::vector<int>::const_iterator rbIt =
                     m_slRxSigParamInfo.at(pktIndex).rbBitmap.begin();
                 rbIt != m_slRxSigParamInfo.at(pktIndex).rbBitmap.end();
                 rbIt++)
            {
                if (collidedRbBitmapTemp.find(*rbIt) != collidedRbBitmapTemp.end())
                {
                    // collision, update the bitmap
                    collidedRbBitmap.insert(*rbIt);
                    break;
                }
                else
                {
                    // store resources used by the packet to detect collision
                    collidedRbBitmapTemp.insert((*rbIt));
                }
            }
        }
    }

    for (auto& ctrlMsgIt : sortedControlMessages)
    {
        uint32_t paramIndex = ctrlMsgIt.index;

        bool corrupt = false;
        bool corruptDecode = false;
        uint8_t pscchMcs = 0 /*using QPSK*/;
        Ptr<NrErrorModelOutput> outputEmForCtrl;

        if (m_slCtrlErrorModelEnabled)
        {
            for (std::vector<int>::const_iterator rbIt =
                     m_slRxSigParamInfo.at(paramIndex).rbBitmap.begin();
                 rbIt != m_slRxSigParamInfo.at(paramIndex).rbBitmap.end();
                 rbIt++)
            {
                // if m_dropTbOnRbCollisionEnabled == false, collidedRbBitmap will remain empty
                // and we move to the second "if" to check if the TB with similar RBs has already
                // been decoded. If m_dropTbOnRbCollisionEnabled == true, the collided TB
                // is marked corrupt and this for loop will break in the first "if" condition
                if (collidedRbBitmap.find(*rbIt) != collidedRbBitmap.end())
                {
                    corrupt = true;
                    NS_LOG_DEBUG(this << " RB " << *rbIt << " has collided");
                    break;
                }
                // the purpose of rbDecodedBitmap and the following "if" is to decode
                // only one SCI 1 msg among multiple SCIs using same or partially
                // overlapping RBs
                if (rbDecodedBitmap.find(*rbIt) != rbDecodedBitmap.end())
                {
                    NS_LOG_DEBUG(*rbIt << " TB with the similar RB has already been decoded. Avoid "
                                          "to decode it again!");
                    corrupt = true;
                    break;
                }
            }

            // We need to call GetTbDecodificationStats for SCI 1 outside
            // of "if (!corrupt && !corruptDecode)" because in the trace we
            // retrieve tbler using
            // traceParams.m_tbler = outputEmForCtrl->m_tbler;
            // if we will do it inside "if (!corrupt && !corruptDecode)"
            // outputEmForData will remain null.
            if (!m_slErrorModel)
            {
                ObjectFactory emFactory;
                emFactory.SetTypeId(m_slErrorModelType);
                m_slErrorModel = DynamicCast<NrErrorModel>(emFactory.Create());
                NS_ABORT_IF(m_slErrorModel == nullptr);
            }
            uint8_t slRank{1}; /// XXX need to set from MIMO config.
            outputEmForCtrl = m_slErrorModel->GetTbDecodificationStats(
                m_slSinrPerceived.at(paramIndex),
                m_slRxSigParamInfo.at(paramIndex).rbBitmap,
                m_slAmc->CalculateTbSize(pscchMcs,
                                         slRank,
                                         m_slRxSigParamInfo.at(paramIndex).rbBitmap.size()),
                pscchMcs,
                NrErrorModel::NrErrorModelHistory());
            corruptDecode = GetErrorModelRv()->GetValue() <= outputEmForCtrl->m_tbler;

            NS_LOG_DEBUG("SCI 1 number of RBs "
                         << m_slRxSigParamInfo.at(paramIndex).rbBitmap.size());
            NS_LOG_DEBUG("SCI 1 TB size " << m_slAmc->CalculateTbSize(
                             pscchMcs,
                             1, /* MIMO rank; see issue #181 */
                             m_slRxSigParamInfo.at(paramIndex).rbBitmap.size()));

            if (!corrupt && !corruptDecode)
            {
                NS_LOG_DEBUG(this << " PSCCH Decoding successful, errorRate "
                                  << outputEmForCtrl->m_tbler << " error " << corrupt);
            }
            else
            {
                NS_LOG_DEBUG(this << " PSCCH Decoding failed, errorRate "
                                  << outputEmForCtrl->m_tbler << " error " << corrupt);
                m_slPscchDecodeFailures++;
                corrupt = true;
            }
        }
        else
        {
            // No error model enabled. If m_dropRbOnCollisionEnabled == true, it will just label the
            // TB as corrupted if the two TBs received at the same time using same RBs. Note: At
            // this stage PSCCH occupies all the RBs of a subchannel. On the other hand, if
            // m_dropRbOnCollisionEnabled == false, all the TBs are considered as not corrupted.
            if (m_dropTbOnRbCollisionEnabled)
            {
                for (std::vector<int>::const_iterator rbIt =
                         m_slRxSigParamInfo.at(paramIndex).rbBitmap.begin();
                     rbIt != m_slRxSigParamInfo.at(paramIndex).rbBitmap.end();
                     rbIt++)
                {
                    if (collidedRbBitmap.find(*rbIt) != collidedRbBitmap.end())
                    {
                        corrupt = true;
                        NS_LOG_DEBUG(this << " RB " << *rbIt << " has collided");
                        break;
                    }
                }
            }
        }

        Ptr<NrSpectrumSignalParametersSlCtrlFrame> ctrlParams =
            DynamicCast<NrSpectrumSignalParametersSlCtrlFrame>(
                m_slRxSigParamInfo.at(paramIndex).params);
        Ptr<Packet> packet = ctrlParams->packetBurst->GetPackets().front();
        if (!corrupt)
        {
            error = false; // at least one control packet is OK
            SpectrumValue psd = m_slSigPerceived.at(paramIndex);
            PscchPduInfo pduInfo;
            pduInfo.packet = packet;
            pduInfo.psd = psd;
            rxControlMessageOkList.push_back(pduInfo);
            // Store the indices of the decoded RBs
            rbDecodedBitmap.insert(m_slRxSigParamInfo.at(paramIndex).rbBitmap.begin(),
                                   m_slRxSigParamInfo.at(paramIndex).rbBitmap.end());
        }

        // Add PSCCH trace.
        NrSlSciF1aHeader sciHeader;
        packet->PeekHeader(sciHeader);
        NrSlMacPduTag tag;
        bool tagFound = packet->PeekPacketTag(tag);
        NS_ABORT_MSG_IF(!tagFound, "Did not find NrSlMacPduTag");
        SlRxCtrlPacketTraceParams traceParams;
        traceParams.m_timeMs = Simulator::Now().GetSeconds() * 1000.0;
        traceParams.m_cellId = ueRx->GetPhy(GetBwpId())->GetCellId();
        traceParams.m_rnti = ueRx->GetPhy(GetBwpId())->GetRnti();
        traceParams.m_tbSize =
            m_slAmc->CalculateTbSize(pscchMcs,
                                     1, /* MIMO rank; see issue #181 */
                                     m_slRxSigParamInfo.at(paramIndex).rbBitmap.size());
        traceParams.m_frameNum = tag.GetSfn().GetFrame();
        traceParams.m_subframeNum = tag.GetSfn().GetSubframe();
        traceParams.m_slotNum = tag.GetSfn().GetSlot();
        traceParams.m_txRnti = tag.GetRnti(); // this is the RNTI of the TX UE
        traceParams.m_mcs = pscchMcs;
        traceParams.m_sinr = ctrlMsgIt.sinrAvg;
        traceParams.m_sinrMin = ctrlMsgIt.sinrMin;
        if (m_slCtrlErrorModelEnabled)
        {
            traceParams.m_tbler = outputEmForCtrl->m_tbler;
        }
        else
        {
            traceParams.m_tbler = 0;
        }
        traceParams.m_corrupt = corrupt;
        traceParams.m_symStart = tag.GetSymStart(); // DATA symbol start
        traceParams.m_numSym = tag.GetNumSym();     // DATA symbol length
        traceParams.m_bwpId = GetBwpId();
        traceParams.m_indexStartSubChannel = sciHeader.GetIndexStartSubChannel();
        traceParams.m_lengthSubChannel = sciHeader.GetLengthSubChannel();
        traceParams.m_slResourceReservePeriod = sciHeader.GetSlResourceReservePeriod();
        traceParams.m_maxNumPerReserve = sciHeader.GetSlMaxNumPerReserve();
        traceParams.m_dstL2Id = tag.GetDstL2Id();
        uint32_t rbBitmapSize =
            static_cast<uint32_t>(m_slRxSigParamInfo.at(paramIndex).rbBitmap.size());
        traceParams.m_rbStart = m_slRxSigParamInfo.at(paramIndex).rbBitmap.at(0);
        traceParams.m_rbEnd = m_slRxSigParamInfo.at(paramIndex).rbBitmap.at(rbBitmapSize - 1);
        traceParams.m_rbAssignedNum = rbBitmapSize;
        m_rxPscchTraceUe(traceParams);
    }

    if (!paramIndexes.empty())
    {
        if (!error)
        {
            NS_LOG_DEBUG(this << " PSCCH OK");
            std::list<PscchPduInfo>::iterator it;
            for (it = rxControlMessageOkList.begin(); it != rxControlMessageOkList.end(); it++)
            {
                m_nrPhyRxPscchEndOkCallback(it->packet, it->psd);
            }
        }
    }
}

void
NrSlSpectrumPhy::RxSlPssch(std::vector<uint32_t> paramIndexes)
{
    NS_LOG_FUNCTION(this << "Number of PSSCH messages:" << paramIndexes.size());

    Ptr<NrUeNetDevice> ueRx = DynamicCast<NrUeNetDevice>(GetDevice());

    NS_ASSERT(GetState() == RX_DATA);

    NS_LOG_DEBUG("Expected TBs (NR SL communication) " << m_slTransportBlocks.size());

    // Compute error on PSSCH
    // Create a mapping between the packet tag and the index of the packet bursts.
    for (uint32_t i = 0; i < paramIndexes.size(); i++)
    {
        uint32_t pktIndex = paramIndexes[i];

        Ptr<NrSpectrumSignalParametersSlDataFrame> dataParams =
            DynamicCast<NrSpectrumSignalParametersSlDataFrame>(
                m_slRxSigParamInfo.at(pktIndex).params);
        std::list<Ptr<Packet>>::const_iterator j = dataParams->packetBurst->Begin();
        // Even though there may be multiple data packets, they all have
        // the same tag, however, there is SCI-stage 2 packet in this burst which
        // does not have the tag. We do not expect any other packet type in this
        // burst. Let's make sure.
        LteRadioBearerTag tag;
        if (!(*j)->PeekPacketTag(tag))
        {
            NrSlSciF2aHeader sciF2a;
            if ((*j)->PeekHeader(sciF2a) != 5 /*5 bytes is the fixed size of SCI format 2a*/)
            {
                NS_FATAL_ERROR("Invalid PSSCH packet type! I didn't find any radio bearer tag "
                               "neither any NrSlSciF2aHeader");
            }
        }
        SlTransportBlocks::iterator itTb = m_slTransportBlocks.find(tag.GetRnti());
        // Note: m_slRxSigParamInfo, contains all the received
        // PSSCH transmissions. On the other hand, m_slTransportBlocks contains
        // the info of only those transmissions, which the receiving UE is
        // interested in to listen and had decoded SCI stage 1. So, it might happen
        // that we would not find the RNTI present in m_slRxSigParamInfo
        // in m_slTransportBlocks. This would happen
        // in the following cases:
        // 1.RX UE failed to decode all the SCI stage 1
        // 2.RX UE decoded only non collided, one or multiple, SCI stage 1 but not from the RNTI we
        // are looking. 3.RX UE wants to listen to some selective destinations or transmitting UEs.
        // In any of the above case, there is no need to create a mapping between
        // the packet tag and the index of the packet bursts.
        if (itTb == m_slTransportBlocks.end())
        {
            NS_LOG_DEBUG("Continuing because RNTI " << tag.GetRnti() << " not found");
            continue;
        }
        itTb->second.m_sinrPerceived = m_slSinrPerceived.at(pktIndex);
        itTb->second.m_pktIndex = pktIndex;
        NS_LOG_DEBUG("Updating SINR for RNTI " << tag.GetRnti());
        itTb->second.m_sinrUpdated = true;
        auto sinrStats =
            GetSinrStats(itTb->second.m_sinrPerceived, itTb->second.m_expected.m_rbBitmap);
        itTb->second.m_sinrAvg = sinrStats.sinrAvg;
        itTb->second.m_sinrMin = sinrStats.sinrMin;

        NS_LOG_INFO("Finishing RX, sinrAvg = " << itTb->second.m_sinrAvg << " sinrMin = "
                                               << itTb->second.m_sinrMin << " SinrAvg (dB) "
                                               << 10 * log(itTb->second.m_sinrAvg) / log(10));
    }

    std::unordered_set<int> collidedRbBitmap;
    if (m_dropTbOnRbCollisionEnabled)
    {
        NS_LOG_DEBUG(this << " PSSCH DropTbOnRbOnCollision: Identifying RB Collisions");
        std::unordered_set<int> collidedRbBitmapTemp;
        for (SlTransportBlocks::iterator itTb = m_slTransportBlocks.begin();
             itTb != m_slTransportBlocks.end();
             itTb++)
        {
            for (std::vector<int>::iterator rbIt = (*itTb).second.m_expected.m_rbBitmap.begin();
                 rbIt != (*itTb).second.m_expected.m_rbBitmap.end();
                 rbIt++)
            {
                if (collidedRbBitmapTemp.find(*rbIt) != collidedRbBitmapTemp.end())
                {
                    // collision, update the bitmap
                    collidedRbBitmap.insert(*rbIt);
                }
                else
                {
                    // store resources used by the packet to detect collision
                    collidedRbBitmapTemp.insert(*rbIt);
                }
            }
        }
    }

    // Compute the error and check for collision for each expected TB
    for (auto& tbIt : m_slTransportBlocks)
    {
        if (tbIt.second.m_sinrUpdated == false)
        {
            // The below abort seems too strict; it could be the case that SCI stage 1
            // was not decoded.
            // NS_ABORT_MSG_IF (tbIt.second.sinrUpdated == false, "SINR not updated for the expected
            // TB from RNTI " << tbIt.first);
            NS_LOG_WARN("SINR not updated for the expected TB from RNTI " << tbIt.first);
            continue;
        }
        Ptr<Packet> sci2Pkt = RetrieveSci2FromPktBurst(tbIt.second.m_pktIndex);
        NrSlSciF2aHeader sciF2a;
        sci2Pkt->PeekHeader(sciF2a);

        bool sciF2aCorrupted = false;
        bool rbCollided =
            false; // no need of this boolean since we track TB info. I might get rid of it later

        Ptr<NrErrorModel> em;
        if (m_slDataErrorModelEnabled)
        {
            NS_LOG_DEBUG("Trying to decode the PSSCH SCI-2A from RNTI : " << tbIt.first);
            ObjectFactory emFactory;
            emFactory.SetTypeId(m_slErrorModelType);
            em = DynamicCast<NrErrorModel>(emFactory.Create());
            NS_ABORT_MSG_UNLESS(em, "No NR error model");
            if (m_dropTbOnRbCollisionEnabled)
            {
                NS_LOG_DEBUG(this << " PSSCH DropTbOnRbOnCollision and error model enabled: "
                                     "Checking for RB collision");
                // Check if any of the RBs have been decoded
                for (std::vector<int>::iterator rbIt = tbIt.second.m_expected.m_rbBitmap.begin();
                     rbIt != tbIt.second.m_expected.m_rbBitmap.end();
                     rbIt++)
                {
                    if (collidedRbBitmap.find(*rbIt) != collidedRbBitmap.end())
                    {
                        NS_LOG_DEBUG(*rbIt << " collided, labeled as corrupted!");
                        rbCollided = true;
                        tbIt.second.m_isSci2Corrupted = true;
                        tbIt.second.m_isCorrupted = true;
                        break;
                    }
                }
            }

            // We need to call GetTbDecodificationStats for SCI 2 and data outside
            // of "if (!rbCollided)" because in the trace we retrieve tbler using
            // traceParams.m_tbler = tbIt.second.outputEmForData->m_tbler;
            // traceParams.m_tblerSci2 = tbIt.second.outputEmForSci2->m_tbler;
            // if we will do it inside "if (!rbCollided)" outputEmForData will remain
            // null.
            uint8_t Sci2Mcs = 0 /*using QPSK*/;
            tbIt.second.m_outputEmForSci2 = m_slErrorModel->GetTbDecodificationStats(
                tbIt.second.m_sinrPerceived,
                tbIt.second.m_expected.m_rbBitmap,
                sciF2a.GetSerializedSize() /*5 bytes is the fixed size of SCI-stage 2 Format 2A*/,
                Sci2Mcs,
                NrErrorModel::NrErrorModelHistory());
            // check the decodification of SCI stage 2 with a random probability
            sciF2aCorrupted = GetErrorModelRv()->GetValue() > tbIt.second.m_outputEmForSci2->m_tbler
                                  ? false
                                  : true;
            tbIt.second.m_isSci2Corrupted = sciF2aCorrupted;
            NS_LOG_DEBUG(this << " SCI stage 2 decoding, errorRate "
                              << tbIt.second.m_outputEmForSci2->m_tbler << " corrupt "
                              << tbIt.second.m_isSci2Corrupted);
            if (sciF2aCorrupted)
            {
                NS_LOG_DEBUG(this << " PSSCH SCI stage 2 decoding failed, errorRate "
                                  << tbIt.second.m_outputEmForSci2->m_tbler);
                m_slSci2aDecodeFailures++;
                // If SCI stage 2 is corrupted, data is also corrupted.
                tbIt.second.m_isCorrupted = true;

                // Trace
                SlRxDataPacketTraceParams traceParams;
                traceParams.m_timeMs = Simulator::Now().GetSeconds() * 1000.0;
                traceParams.m_cellId = ueRx->GetPhy(GetBwpId())->GetCellId();
                traceParams.m_rnti = ueRx->GetPhy(GetBwpId())->GetRnti();
                traceParams.m_tbSize = tbIt.second.m_expected.m_tbSize;
                traceParams.m_frameNum = tbIt.second.m_expected.m_sfn.GetFrame();
                traceParams.m_subframeNum = tbIt.second.m_expected.m_sfn.GetSubframe();
                traceParams.m_slotNum = tbIt.second.m_expected.m_sfn.GetSlot();
                traceParams.m_txRnti = tbIt.first; // this is the RNTI of the TX UE
                traceParams.m_mcs = tbIt.second.m_expected.m_mcs;
                traceParams.m_sinr = tbIt.second.m_sinrAvg;
                traceParams.m_sinrMin = tbIt.second.m_sinrMin;
                traceParams.m_tblerSci2 = tbIt.second.m_outputEmForSci2->m_tbler;
                traceParams.m_rv = sciF2a.GetRv();
                traceParams.m_ndi = sciF2a.GetNdi();
                traceParams.m_corrupt = tbIt.second.m_isCorrupted;
                traceParams.m_sci2Corrupted = tbIt.second.m_isSci2Corrupted;
                traceParams.m_symStart = tbIt.second.m_expected.m_symStart;
                traceParams.m_numSym = tbIt.second.m_expected.m_numSym;
                traceParams.m_bwpId = GetBwpId();
                traceParams.m_dstL2Id = sciF2a.GetDstId();
                traceParams.m_srcL2Id = sciF2a.GetSrcId();
                uint32_t rbBitmapSize =
                    static_cast<uint32_t>(tbIt.second.m_expected.m_rbBitmap.size());
                traceParams.m_rbStart = tbIt.second.m_expected.m_rbBitmap.at(0);
                traceParams.m_rbEnd = tbIt.second.m_expected.m_rbBitmap.at(rbBitmapSize - 1);
                traceParams.m_rbAssignedNum = rbBitmapSize;
                m_rxPsschTraceUe(traceParams);
                continue;
            }
            else
            {
                NS_LOG_DEBUG(this << " PSSCH SCI stage 2 decoding succeeded, errorRate "
                                  << tbIt.second.m_outputEmForSci2->m_tbler);
            }
            if (sciF2a.GetNdi())
            {
                NS_LOG_DEBUG("RemovePrevDecoded: " << +sciF2a.GetHarqId()
                                                   << " for the packets received from RNTI "
                                                   << tbIt.first << " rv " << +sciF2a.GetRv());
                m_slHarqPhy.RemovePrevDecoded(tbIt.first, sciF2a.GetHarqId());
            }
            tbIt.second.m_isHarqEnabled = sciF2a.GetHarqFbIndicator();
            // Do not dispatch already decoded TBs to UE PHY (may be a blind retx)
            if (m_slHarqPhy.IsPrevDecoded(tbIt.first, sciF2a.GetHarqId()))
            {
                NS_LOG_DEBUG(
                    "Do not dispatch already decoded TB; may be blind retx: " << tbIt.first);
                continue;
            }

            NS_LOG_DEBUG("Trying to decode the PSSCH TB from RNTI : " << tbIt.first);
            // Since we do not rely on RV to track the number of transmissions,
            // before decoding the data, erase the HARQ history of a TB
            // which was not decoded and received from the same rnti and HARQ
            // process
            //  retrieve HARQ info
            const NrErrorModel::NrErrorModelHistory& harqInfoList =
                m_slHarqPhy.GetHarqProcessInfoSl(tbIt.first, sciF2a.GetHarqId());
            if (sciF2a.GetNdi() && !harqInfoList.empty())
            {
                m_slHarqPhy.ResetSlDataHarqProcessStatus(tbIt.first, sciF2a.GetHarqId());
                // fetch again an empty HARQ info
                // sorry to damage the sanity of const but I had no choice
                const_cast<NrErrorModel::NrErrorModelHistory&>(harqInfoList) =
                    m_slHarqPhy.GetHarqProcessInfoSl(tbIt.first, sciF2a.GetHarqId());
            }
            tbIt.second.m_outputEmForData =
                m_slErrorModel->GetTbDecodificationStats(tbIt.second.m_sinrPerceived,
                                                         tbIt.second.m_expected.m_rbBitmap,
                                                         tbIt.second.m_expected.m_tbSize,
                                                         tbIt.second.m_expected.m_mcs,
                                                         harqInfoList);
            if (!rbCollided)
            {
                // check the decodification of data with a random probability
                tbIt.second.m_isCorrupted =
                    GetErrorModelRv()->GetValue() <= tbIt.second.m_outputEmForData->m_tbler;
                for (auto it = tbIt.second.m_sinrPerceived.ConstValuesBegin();
                     it != tbIt.second.m_sinrPerceived.ConstValuesEnd();
                     it++)
                {
                    NS_ABORT_MSG_IF(std::isnan(*it), "Invalid SINR spectrum value (nan)");
                }
                if (tbIt.second.m_isCorrupted)
                {
                    NS_LOG_DEBUG(this << " PSSCH TB decoding failed, errorRate "
                                      << tbIt.second.m_outputEmForData->m_tbler);
                    m_slTbDecodeFailures++;
                    tbIt.second.m_isCorrupted = true;
                }
                else
                {
                    NS_LOG_DEBUG(this << " PSSCH TB decoding successful, errorRate "
                                      << tbIt.second.m_outputEmForData->m_tbler << " data corrupt "
                                      << tbIt.second.m_isCorrupted);
                    tbIt.second.m_isCorrupted = false;
                    m_slHarqPhy.IndicatePrevDecoded(tbIt.first, sciF2a.GetHarqId());
                }

                // Arrange the HARQ history
                if (!tbIt.second.m_isCorrupted)
                {
                    NS_LOG_DEBUG("Reset SL process: " << +sciF2a.GetHarqId()
                                                      << " for the packets received from RNTI "
                                                      << tbIt.first << " rv " << +sciF2a.GetRv());
                    m_slHarqPhy.ResetSlDataHarqProcessStatus(tbIt.first, sciF2a.GetHarqId());
                }
                else
                {
                    NS_LOG_DEBUG("Update SL process: " << +sciF2a.GetHarqId()
                                                       << " for the packet received from RNTI "
                                                       << tbIt.first);
                    m_slHarqPhy.UpdateSlDataHarqProcessStatus(tbIt.first,
                                                              sciF2a.GetHarqId(),
                                                              tbIt.second.m_outputEmForData);
                }

                if (tbIt.second.m_isCorrupted)
                {
                    NS_LOG_INFO("RNTI " << tbIt.first << " processId " << +sciF2a.GetHarqId()
                                        << " size " << tbIt.second.m_expected.m_tbSize << " mcs "
                                        << +tbIt.second.m_expected.m_mcs << " bitmap size "
                                        << tbIt.second.m_expected.m_rbBitmap.size()
                                        << " rv from MAC: " << +sciF2a.GetRv()
                                        << " elements in the history: " << harqInfoList.size()
                                        << " TBLER " << tbIt.second.m_outputEmForData->m_tbler
                                        << " corrupted " << tbIt.second.m_isCorrupted);
                }
            }
        }
        else // No error model enabled
        {
            if (sciF2a.GetNdi())
            {
                NS_LOG_DEBUG("RemovePrevDecoded: " << +sciF2a.GetHarqId()
                                                   << " for the packets received from RNTI "
                                                   << tbIt.first << " rv " << +sciF2a.GetRv());
                m_slHarqPhy.RemovePrevDecoded(tbIt.first, sciF2a.GetHarqId());
            }
            tbIt.second.m_isHarqEnabled = sciF2a.GetHarqFbIndicator();
            // Do not dispatch already decoded TBs to UE PHY (may be a blind retx)
            if (m_slHarqPhy.IsPrevDecoded(tbIt.first, sciF2a.GetHarqId()))
            {
                continue;
            }

            if (m_dropTbOnRbCollisionEnabled)
            {
                NS_LOG_DEBUG(this << " PSSCH DropTbOnRbOnCollision enabled, error model disabled: "
                                     "Checking for RB collision");
                // Check if any of the RBs have been decoded
                for (std::vector<int>::iterator rbIt = tbIt.second.m_expected.m_rbBitmap.begin();
                     rbIt != tbIt.second.m_expected.m_rbBitmap.end();
                     rbIt++)
                {
                    if (collidedRbBitmap.find(*rbIt) != collidedRbBitmap.end())
                    {
                        NS_LOG_DEBUG(*rbIt << " collided, labeled as corrupted!");
                        rbCollided = true;
                        tbIt.second.m_isSci2Corrupted = true;
                        tbIt.second.m_isCorrupted = true;
                        break;
                    }
                }
            }
            /*
            //This if is redundant since the default values for
            //isSci2Corrupted and isCorrupted is false. Leaving
            //it for readability purpose at this stage.
            if (!rbCollided)
              {
                tbIt.second.m_isSci2Corrupted = false;
                tbIt.second.m_isCorrupted = false;
              }
             */
        }

        SlRxDataPacketTraceParams traceParams;
        traceParams.m_timeMs = Simulator::Now().GetSeconds() * 1000.0;
        traceParams.m_cellId = ueRx->GetPhy(GetBwpId())->GetCellId();
        traceParams.m_rnti = ueRx->GetPhy(GetBwpId())->GetRnti();
        traceParams.m_tbSize = tbIt.second.m_expected.m_tbSize;
        traceParams.m_frameNum = tbIt.second.m_expected.m_sfn.GetFrame();
        traceParams.m_subframeNum = tbIt.second.m_expected.m_sfn.GetSubframe();
        traceParams.m_slotNum = tbIt.second.m_expected.m_sfn.GetSlot();
        traceParams.m_txRnti = tbIt.first; // this is the RNTI of the TX UE
        traceParams.m_mcs = tbIt.second.m_expected.m_mcs;
        traceParams.m_rv = sciF2a.GetRv();
        traceParams.m_ndi = sciF2a.GetNdi();
        traceParams.m_sinr = tbIt.second.m_sinrAvg;
        traceParams.m_sinrMin = tbIt.second.m_sinrMin;
        if (m_slDataErrorModelEnabled)
        {
            traceParams.m_tbler = tbIt.second.m_outputEmForData->m_tbler;
            traceParams.m_tblerSci2 = tbIt.second.m_outputEmForSci2->m_tbler;
        }
        else
        {
            traceParams.m_tbler = 0;
            traceParams.m_tblerSci2 = 0;
        }
        traceParams.m_corrupt = tbIt.second.m_isCorrupted;
        traceParams.m_sci2Corrupted = tbIt.second.m_isSci2Corrupted;
        traceParams.m_symStart = tbIt.second.m_expected.m_symStart;
        traceParams.m_numSym = tbIt.second.m_expected.m_numSym;
        traceParams.m_bwpId = GetBwpId();
        uint32_t rbBitmapSize = static_cast<uint32_t>(tbIt.second.m_expected.m_rbBitmap.size());
        traceParams.m_rbStart = tbIt.second.m_expected.m_rbBitmap.at(0);
        traceParams.m_rbEnd = tbIt.second.m_expected.m_rbBitmap.at(rbBitmapSize - 1);
        traceParams.m_rbAssignedNum = rbBitmapSize;
        traceParams.m_dstL2Id = sciF2a.GetDstId();
        traceParams.m_srcL2Id = sciF2a.GetSrcId();
        m_rxPsschTraceUe(traceParams);

        GetSecond GetTBInfo;
        // send HARQ feedback (if not already done for this TB)
        if (tbIt.second.m_isHarqEnabled && !GetTBInfo(tbIt).m_harqFeedbackSent &&
            !m_phySlHarqFeedbackCallback.IsNull())
        {
            GetTBInfo(tbIt).m_harqFeedbackSent = true;
            SlHarqInfo slHarqInfo;
            slHarqInfo.m_txRnti = tbIt.first; // this is the RNTI of the TX UE
            slHarqInfo.m_rnti = ueRx->GetPhy(GetBwpId())->GetRnti();
            slHarqInfo.m_dstL2Id = sciF2a.GetDstId();
            slHarqInfo.m_harqProcessId = sciF2a.GetHarqId();
            slHarqInfo.m_bwpIndex = GetBwpId();
            if (GetTBInfo(tbIt).m_isCorrupted || GetTBInfo(tbIt).m_isSci2Corrupted)
            {
                NS_LOG_DEBUG("Sending NACK HARQ feedback to SlHarqFeedback callback");
                slHarqInfo.m_harqStatus = SlHarqInfo::NACK;
            }
            else
            {
                NS_LOG_DEBUG("Sending ACK HARQ feedback to SlHarqFeedback callback");
                slHarqInfo.m_harqStatus = SlHarqInfo::ACK;
            }
            m_phySlHarqFeedbackCallback(slHarqInfo);
        }

        // Now dispatch the non corrupted TBs to UE PHY
        if (!tbIt.second.m_isCorrupted)
        {
            NS_LOG_DEBUG("SpectrumPhy dispatching a non corrupted TB to UE PHY");
            Ptr<NrSpectrumSignalParametersSlDataFrame> params =
                DynamicCast<NrSpectrumSignalParametersSlDataFrame>(
                    m_slRxSigParamInfo.at(tbIt.second.m_pktIndex).params);
            Ptr<PacketBurst> pb = params->packetBurst;
            Ptr<SpectrumValue> psd = params->psd;
            m_nrPhyRxPsschEndOkCallback(pb, *psd);
        }
    }
    NS_LOG_DEBUG("Clearing m_slTransportBlocks");
    m_slTransportBlocks.clear();
}

void
NrSlSpectrumPhy::RxSlPsfch(std::vector<uint32_t> paramIndexes)
{
    NS_LOG_FUNCTION(this << "Number of PSFCH messages:" << paramIndexes.size());
    for (uint32_t i = 0; i < m_slRxSigParamInfo.size(); i++)
    {
        uint32_t pktIndex = paramIndexes[i];
        Ptr<NrSpectrumSignalParametersSlFeedback> feedbackParams =
            DynamicCast<NrSpectrumSignalParametersSlFeedback>(
                m_slRxSigParamInfo.at(pktIndex).params);
        for (auto& it : feedbackParams->feedbackList)
        {
            NS_LOG_DEBUG("Received RxSlPsfch from node "
                         << feedbackParams->nodeId << " dstL2Id "
                         << it->GetSlHarqFeedback().m_dstL2Id << " harqProcessId "
                         << +it->GetSlHarqFeedback().m_harqProcessId << " bwpIndex "
                         << +it->GetSlHarqFeedback().m_bwpIndex
                         << (it->GetSlHarqFeedback().IsReceivedOk() ? " ACK" : " NACK"));
            m_nrPhyRxSlPsfchCallback(feedbackParams->nodeId, it->GetSlHarqFeedback());
        }
    }
}

Ptr<Packet>
NrSlSpectrumPhy::RetrieveSci2FromPktBurst(uint32_t pktIndex)
{
    NS_LOG_FUNCTION(this << pktIndex);
    Ptr<NrSpectrumSignalParametersSlDataFrame> dataParams =
        DynamicCast<NrSpectrumSignalParametersSlDataFrame>(m_slRxSigParamInfo.at(pktIndex).params);
    Ptr<PacketBurst> pktBurst = dataParams->packetBurst;
    std::list<Ptr<Packet>>::const_iterator it;
    Ptr<Packet> sci2pkt;
    for (it = pktBurst->Begin(); it != pktBurst->End(); it++)
    {
        LteRadioBearerTag tag;
        if (!(*it)->PeekPacketTag(tag))
        {
            // SCI stage 2 is the only packet in the packet burst, which does
            // not have the tag
            sci2pkt = *it;
            break;
        }
    }

    NS_ABORT_MSG_IF(sci2pkt == nullptr, "Did not find SCI stage 2 in PSSCH packet burst");

    return sci2pkt;
}

const NrSpectrumPhy::SinrStats
NrSlSpectrumPhy::GetSinrStats(const SpectrumValue& sinr, const std::vector<int>& rbBitmap)
{
    NS_LOG_FUNCTION(this << sinr);
    SinrStats stats;
    stats.sinrAvg = 0;
    stats.sinrMin = 99999999999;
    for (const auto& rbIndex : rbBitmap)
    {
        stats.sinrAvg += sinr.ValuesAt(rbIndex);
        if (sinr.ValuesAt(rbIndex) < stats.sinrMin)
        {
            stats.sinrMin = sinr.ValuesAt(rbIndex);
        }
    }

    stats.sinrAvg = stats.sinrAvg / rbBitmap.size();

    return stats;
}

void
NrSlSpectrumPhy::SetSlAmc(Ptr<NrAmc> slAmc)
{
    NS_LOG_FUNCTION(this << slAmc);
    m_slAmc = slAmc;
}

void
NrSlSpectrumPhy::SetNrPhyRxPscchEndOkCallback(NrPhyRxPscchEndOkCallback c)
{
    NS_LOG_FUNCTION(this);
    m_nrPhyRxPscchEndOkCallback = c;
}

void
NrSlSpectrumPhy::SetNrPhyRxPsschEndOkCallback(NrPhyRxPsschEndOkCallback c)
{
    NS_LOG_FUNCTION(this);
    m_nrPhyRxPsschEndOkCallback = c;
}

void
NrSlSpectrumPhy::SetNrPhyRxPsschEndErrorCallback(NrPhyRxPsschEndErrorCallback c)
{
    NS_LOG_FUNCTION(this);
    m_nrPhyRxPsschEndErrorCallback = c;
}

void
NrSlSpectrumPhy::SetNrPhyRxSlPsfchCallback(NrPhyRxSlPsfchCallback c)
{
    NS_LOG_FUNCTION(this);
    m_nrPhyRxSlPsfchCallback = c;
}

void
NrSlSpectrumPhy::AddSlExpectedTb(ExpectedTb expectedTb, uint16_t dstL2Id)
{
    NS_LOG_FUNCTION(this);
    auto it = m_slTransportBlocks.find(expectedTb.m_rnti);

    Ptr<NrUeNetDevice> ueRx = DynamicCast<NrUeNetDevice>(GetDevice());

    if (it != m_slTransportBlocks.end())
    {
        // If the RNTI is already registered, ignore this repeat call for now
        // The result will be that one of the two transport blocks will be lost
        // on the receiver side
        NS_LOG_WARN("Variable m_slTransportBlocks is not designed to manage two TBs from same RNTI "
                    "in same slot");
        NS_LOG_DEBUG("RNTI " << ueRx->GetPhy(GetBwpId())->GetRnti()
                             << " failed to add NR SL expected TB from rnti " << expectedTb.m_rnti
                             << " with Dest id " << dstL2Id << " TB size = " << expectedTb.m_tbSize
                             << " mcs = " << static_cast<uint32_t>(expectedTb.m_mcs)
                             << " sfn: " << expectedTb.m_sfn
                             << " symstart = " << static_cast<uint32_t>(expectedTb.m_symStart)
                             << " numSym = " << static_cast<uint32_t>(expectedTb.m_numSym));
        return;
    }
    expectedTb.m_dstL2Id = dstL2Id;
    SlTransportBlockInfo tbInfo({expectedTb});

    bool insertStatus =
        m_slTransportBlocks.emplace(std::make_pair(expectedTb.m_rnti, tbInfo)).second;

    NS_ASSERT_MSG(insertStatus == true, "Unable to emplace the info of an NR SL expected TB");

    NS_LOG_DEBUG("RNTI " << ueRx->GetPhy(GetBwpId())->GetRnti()
                         << " added NR SL expected TB from rnti " << expectedTb.m_rnti
                         << " with Dest id " << dstL2Id << " TB size = " << expectedTb.m_tbSize
                         << " mcs = " << static_cast<uint32_t>(expectedTb.m_mcs)
                         << " sfn: " << expectedTb.m_sfn
                         << " symstart = " << static_cast<uint32_t>(expectedTb.m_symStart)
                         << " numSym = " << static_cast<uint32_t>(expectedTb.m_numSym));
}

void
NrSlSpectrumPhy::ClearExpectedSlTb()
{
    if (m_slTransportBlocks.size())
    {
        NS_LOG_FUNCTION(this);
        m_slTransportBlocks.clear();
    }
}

bool
operator==(const NrSlSpectrumPhy::SlCtrlSigParamInfo& a,
           const NrSlSpectrumPhy::SlCtrlSigParamInfo& b)
{
    return (a.sinrAvg == b.sinrAvg);
}

bool
operator<(const NrSlSpectrumPhy::SlCtrlSigParamInfo& a,
          const NrSlSpectrumPhy::SlCtrlSigParamInfo& b)
{
    // we want by decreasing SINR. The second condition will make
    // sure that the two TBs with equal SINR are inserted in increasing
    // order of the index.
    return (a.sinrAvg > b.sinrAvg) || (a.index < b.index);
}

} // namespace ns3
