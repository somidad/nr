/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef NR_SL_UE_PHY_H
#define NR_SL_UE_PHY_H

#include "nr-sl-mac-pdu-tag.h"
#include "nr-sl-sci-f1a-header.h"
#include "nr-sl-sci-f2a-header.h"
#include "nr-sl-ue-phy-sap.h"
#include "nr-ue-phy.h"

#include <ns3/nr-sl-ue-cphy-sap.h>

#include <queue>
#include <utility>

namespace ns3
{

extern const Time NR_DEFAULT_PMI_INTERVAL_WB; // Wideband PMI update interval
extern const Time NR_DEFAULT_PMI_INTERVAL_SB; // Subband PMI update interval

class NrChAccessManager;
class BeamManager;
class BeamId;
class NrUePowerControl;
class NrSlCommResourcePool;

/**
 * \ingroup ue-phy
 * \brief The SL UE PHY class
 *
 * This class represents the Sidelink PHY in the User Equipment.
 */
class NrSlUePhy : public NrUePhy
{
    /// allow MemberNrSlUePhySapProvider<NrUePhy> class friend access
    friend class MemberNrSlUeCphySapProvider<NrSlUePhy>;
    friend class MemberNrSlUePhySapProvider<NrUePhy>;

  public:
    /**
     * \brief Get the object TypeId
     * \return the object type id
     */
    static TypeId GetTypeId();

    /**
     * \brief NrSlUePhy constructor.
     */
    NrSlUePhy();

    /**
     * \brief NrSlUePhy destructor.
     */
    ~NrSlUePhy() override;

    /**
     * \brief Receive the HARQ feedback (on the transmission) from
     * NrSpectrumPhy and store it for PSFCH transmission
     *
     * Connected by the helper to a NrSpectrumPhy callback
     *
     * \param m the HARQ feedback
     */
    void EnqueueSlHarqFeedback(const SlHarqInfo& m);

    /**
     * \brief pre-configure sidelink bandwidth
     *
     * This method will used in out of coverage
     * scenarios to set the channel bandwidth.
     * In in-coverage scenario the channel bandwidth
     * is configured by RRC after receiving the MIB.
     *
     * \param slBandwidth The total sidelink channel bandwidth
     */
    void PreConfigSlBandwidth(uint16_t slBandwidth);
    /**
     * \brief Register sidelink bandwidthpart id
     *
     * \param bwpId The bandwidthpart id
     */
    void RegisterSlBwpId(uint16_t bwpId);

    /**
     * \brief Get the NR Sidelink UE Control PHY SAP offered by PHY to RRC
     *
     * \return the NR Sidelink UE Control PHY SAP provider interface offered by
     *         PHY to RRC.
     */
    NrSlUeCphySapProvider* GetNrSlUeCphySapProvider();

    /**
     * \brief Set the NR Sidelink UE Control MAC SAP offered by RRC to PHY
     *
     * \param s the NR Sidelink UE Control MAC SAP user interface offered by
     *          RRC to PHY.
     */
    void SetNrSlUeCphySapUser(NrSlUeCphySapUser* s);

    /**
     * \brief Set the NR Sidelink UE PHY SAP offered by UE MAC to UE PHY
     *
     * \param s the NR Sidelink UE PHY SAP user interface offered to the
     *          UE PHY by UE MAC
     */
    void SetNrSlUePhySapUser(NrSlUePhySapUser* s);
    /**
     * \brief Receive new PSCCH PHY pdu from SpectrumPhy
     * \param p The packet received
     */
    void PhyPscchPduReceived(const Ptr<Packet>& p, const SpectrumValue& psd);
    /**
     * \brief Receive new successfully decoded PSSCH PHY pdu from SpectrumPhy
     * \param pb The packet burst received
     * \param psd The power spectral density received
     */
    void PhyPsschPduReceived(const Ptr<PacketBurst>& pb, const SpectrumValue& psd);
    /**
     * \brief Receive new successfully decoded PSFCH from SpectrumPhy
     * \param sendingNodeId sending nodeId
     * \param harqInfo the HARQ info
     */
    void PhyPsfchReceived(uint32_t sendingNodeId, SlHarqInfo harqInfo);

  protected:
    /**
     * \brief DoDispose method inherited from Object
     */
    void DoDispose() override;

    /**
     * \brief Add NR Sidelink communication transmission pool
     *
     * Adds transmission pool for NR Sidelink communication
     *
     * \param txPool The pointer to the NrSlCommResourcePool
     */
    void DoAddNrSlCommTxPool(Ptr<const NrSlCommResourcePool> txPool);
    /**
     * \brief Add NR Sidelink communication reception pool
     *
     * Adds reception pool for NR Sidelink communication
     *
     * \param rxPool The pointer to the NrSlCommResourcePool
     */
    void DoAddNrSlCommRxPool(Ptr<const NrSlCommResourcePool> rxPool);

  private:
    // SAP methods
    void DoReset() override;

    // overrides
    void StartSlot(const SfnSf& s) override;

    /**
     * \brief Sidelink RX grant information about the expected NR SL transport
     *        block at a certain point in the slot
     *
     * This information will be passed by the NrUePhy to NrSpectrumPhy through a
     * call to AddSlExpectedTb
     */
    struct SlRxGrantInfo
    {
        /**
         * \brief constructor
         * \param rnti Tx RNTI
         * \param dstId Destination id
         * \param tbSize TB Size
         * \param mcs MCS
         * \param rbMap RB map
         * \param symStart Starting symbol index
         * \param numSym Total number of symbols
         * \param sfn SfnSf
         */
        SlRxGrantInfo(uint16_t rnti,
                      uint32_t dstId,
                      uint32_t tbSize,
                      uint8_t mcs,
                      const std::vector<int>& rbMap,
                      uint8_t symStart,
                      uint8_t numSym,
                      const SfnSf& sfn)
            : rnti{rnti},
              dstId{dstId},
              tbSize(tbSize),
              mcs(mcs),
              rbBitmap(rbMap),
              symStart(symStart),
              numSym(numSym),
              sfn(sfn)
        {
        }

        SlRxGrantInfo() = delete;
        SlRxGrantInfo(const SlRxGrantInfo& o) = default;

        uint16_t rnti{0};          //!< Tx RNTI
        uint32_t dstId{0};         //!< Destination id
        uint32_t tbSize{0};        //!< TBSize
        uint8_t mcs{0};            //!< MCS
        std::vector<int> rbBitmap; //!< RB Bitmap
        uint8_t symStart{0};       //!< Sym start
        uint8_t numSym{0};         //!< Num sym
        SfnSf sfn;                 //!< SFN
    };

    /**
     * \brief Start the NR SL slot processing
     * \param s the slot number
     */
    void StartNrSlSlot(const SfnSf& s);
    /**
     * \brief Start the processing of a NR Sidelink variable TTI
     * \param varTtiInfo the slot VarTti allocation info of the variable TTI
     *
     * This time can be a SL CTRL, a SL data, or a SL PSFCH, with
     * an appropriate number of symbols (limited to the number of symbols per
     * slot).
     *
     * At the end of processing, it schedules the method EndNrSlVarTti that will finish
     * the processing of the variable TTI allocation.
     *
     * \see EndNrSlVarTti
     */
    void StartNrSlVarTti(const NrSlVarTtiAllocInfo& varTtiInfo);
    /**
     * \brief End the processing of a NR Sidelink variable TTI
     * \param varTtiInfo the slot VarTti allocation info of the variable TTI
     *
     * The end of the NR SL variable TTI indicates that the allocation has been
     * transmitted. Depending on the variable TTI left with the slot, this method
     * will schedule another NR SL var TTI (StartNrSlVarTti()) or will start
     * new slot.
     *
     * \see StartNrSlVarTti
     * \see StartNrSlSlot
     */
    void EndNrSlVarTti(const NrSlVarTtiAllocInfo& varTtiInfo);
    /**
     * \brief Transmit NR SL CTRL and return the time at which the transmission will end
     * \param varTtiInfo the current slot VarTti allocation info to TX NR SL CTRL
     * \return the time at which the transmission of NR SL CTRL will end
     */
    Time SlCtrl(const NrSlVarTtiAllocInfo& varTtiInfo) __attribute__((warn_unused_result));
    /**
     * \brief Transmit to the spectrum phy the NR SL CTRL packet burst
     *
     * \param pb Packet burst to transmit
     * \param varTtiPeriod period of transmission
     * \param varTtiInfo the slot VarTti allocation info of the variable TTI
     */
    void SendNrSlCtrlChannels(const Ptr<PacketBurst>& pb,
                              const Time& varTtiPeriod,
                              const NrSlVarTtiAllocInfo& varTtiInfo);
    /**
     * \brief Transmit to the spectrum phy the NR SL CTRL packet burst
     *
     * \param pb Packet burst to transmit
     * \param varTtiPeriod period of transmission
     * \param varTtiInfo the slot VarTti allocation info of the variable TTI
     */
    void SendNrSlDataChannels(const Ptr<PacketBurst>& pb,
                              const Time& varTtiPeriod,
                              const NrSlVarTtiAllocInfo& varTtiInfo);
    /**
     * \brief Transmit to the spectrum phy the NR SL FB message list
     *
     * \param feedbackList list of messages to transmit
     * \param varTtiPeriod period of transmission
     * \param varTtiInfo the slot VarTti allocation info of the variable TTI
     */
    void SendNrSlFbChannels(const std::list<Ptr<NrSlHarqFeedbackMessage>>& feedbackList,
                            const Time& varTtiPeriod,
                            const NrSlVarTtiAllocInfo& varTtiInfo);
    /**
     * \brief Transmit NR SL DATA and return the time at which the transmission will end
     * \param varTtiInfo the current slot VarTti allocation info to TX NR SL DATA
     * \return the time at which the transmission of NR SL DATA will end
     */
    Time SlData(const NrSlVarTtiAllocInfo& varTtiInfo) __attribute__((warn_unused_result));
    /**
     * \brief Transmit NR SL feedback and return the time at which the transmission will end
     * \param varTtiInfo the current slot VarTti allocation info to TX NR SL FEEDBACK
     * \return the time at which the transmission of NR SL FEEDBACK will end
     */
    Time SlFeedback(const NrSlVarTtiAllocInfo& varTtiInfo) __attribute__((warn_unused_result));

    /**
     * \brief Get the Sidelink RSRP value in dBm
     *
     * At the moment, SL RSRP is computed using the PSD of the signal in PSCCH
     * for which we have successfully decoded the SCI-1A.
     *
     * \param psd the power spectral density per each RB
     * \return a pair of Sidelink RSRP values in Watt and in dBm
     */
    std::pair<double, double> GetSidelinkRsrp(SpectrumValue psd);

    /**
     * \brief Save the future Sidelink RX grants indicated by SCI 1-A
     * \param sciF1a SCI 1-A header
     * \param tag NrSlMacPduTag
     * \param sbChSize The sub-channel size in RBs
     */
    void SaveFutureSlRxGrants(const NrSlSciF1aHeader& sciF1a,
                              const NrSlMacPduTag& tag,
                              const uint16_t sbChSize);
    /**
     * \brief Send Sidelink expected TB info to NrSpectrumPhy
     * \param s The SfnSf
     *
     * This method will go over the \link m_slRxGrants \endlink list, which stores
     * the info about the possible expected TBs to be received in the current
     * slot without SCI 1-A, and send this info to NrSpectrumPhy.
     */
    void SendSlExpectedTbInfo(const SfnSf& s);
    NrSlUeCphySapProvider* m_nrSlUeCphySapProvider; //!< Control SAP interface to receive calls from
                                                    //!< the UE RRC instance
    NrSlUeCphySapUser* m_nrSlUeCphySapUser{
        nullptr}; //!< Control SAP interface to call the methods of UE RRC instance
    NrSlUePhySapUser* m_nrSlUePhySapUser{
        nullptr}; //!< SAP interface to call the methods of UE MAC instance
    Ptr<const NrSlCommResourcePool> m_slTxPool; //!< Sidelink communication transmission pools
    Ptr<const NrSlCommResourcePool> m_slRxPool; //!< Sidelink communication reception pools
    std::deque<SlRxGrantInfo> m_slRxGrants;     //!< Sidelink RX grants indicated by SCI 1-A
    std::list<std::pair<SfnSf, Ptr<NrSlHarqFeedbackMessage>>>
        m_slHarqFbList; // List of pending SL HARQ FB messages

    /**
     * Structure to keep track of the RSRP measurements of a specific UE
     * within a layer-1 filtering period
     */
    struct UeSlRsrpMeasurementsElement
    {
        double rsrpSum;   ///< Sum of RSRP sample values in linear unit.
        uint16_t rsrpNum; ///< Number of RSRP samples.
    };

    /**
     * Structure to store the RSRP measurements of the current layer-1 filtering period.
     * Indexed by the L2Id of the UE the measurements come from
     */
    std::map<uint32_t, UeSlRsrpMeasurementsElement> m_ueSlRsrpMeasurementsMap;

    /**
     * True if a the UE is measuring and reporting UEs RSRP
     */
    bool m_ueSlRsrpMeasurementsEnabled;
    /**
     * The `ReportUeSlRsrpMeasurements` trace source. Contains trace information
     * regarding sidelink RSRP measured.
     * Exporting the RNTI of the originating UE, the L2 ID of the destination, and the RSRP (in dBm)
     */
    TracedCallback<uint16_t, uint32_t, double> m_reportUeSlRsrpMeasurements;

    /**
     * The RRC instructs the PHY to enable the RSRP measurements of the UEs in proximity
     */
    void DoEnableUeSlRsrpMeasurements();

    /**
     * The RRC instructs the PHY to disable the RSRP measurements of the UEs in proximity
     */
    void DoDisableUeSlRsrpMeasurements();

    /**
     * Perform the layer-1 filtering of RSRP measurements and report the
     * results to the RRC entity.
     */
    void ReportUeSlRsrpMeasurements();

    Time m_rsrpFilterPeriod; //!< L1 Filter Period for RSRP measurements
};

} // namespace ns3

#endif /* NR_SL_UE_PHY_H */
