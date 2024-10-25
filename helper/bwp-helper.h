/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef BWP_HELPER_H
#define BWP_HELPER_H

#include <ns3/ptr.h>
#include <ns3/spectrum-channel.h>

#include <memory>
#include <vector>

namespace ns3
{

/**
 * \ingroup helper
 * \brief Spectrum part
 *
 * This is the minimum unit of usable spectrum by a PHY class. For creating
 * any GNB or UE, you will be asked to provide a list of BandwidthPartInfo
 * to the methods NrHelper::InstallGnbDevice() and NrHelper::InstallUeDevice().
 * The reason is that the helper will, for every GNB and UE in the scenario,
 * create a PHY class that will be attached to the channels included in this struct.
 *
 * For every bandwidth part (in this context, referred to a spectrum part) you
 * have to indicate the central frequency and the higher/lower frequency, as
 * well as the entire bandwidth plus the modeling.
 *
 * The pointers to the channels, if left empty, will be initialized by
 * NrHelper::InitializeOperationBand().
 */
struct BandwidthPart
{
    uint8_t m_bwpId{0};             //!< BWP id
    double m_centralFrequency{0.0}; //!< BWP central frequency
    double m_channelBandwidth{0.0}; //!< BWP bandwidth

    BandwidthPart() = default;
    /**
     * Parameterized constructor, for directly defining BandwidthPartInfo outside of CcBwpCreator
     *
     * Use this constructor if you want to customize the propagation models outside of the
     * ThreeGppPropagationModel framework (e.g., s simple Friis loss model with no shadowing
     * or other fading models).
     * After calling this constructor, the user is responsible for setting the m_channel pointer
     * after adding any desired loss and delay models to it; the m_propagation and m_3GppChannel
     * pointers may be left null.
     *
     * \param bwpId Bandwidth Part ID
     * \param centralFrequency Central frequency in Hz
     * \param channelBandwidth Channel bandwidth in Hz
     * \param scenario The 3GPP scenario (if applicable).  If unset, will default to 'Custom'.
     *
     * If a 3GPP scenario is desired, the CcBwpCreator should probably be used to initialize
     * the 3GPP propagation modesl properly; this constructor is more aimed at providing an
     * option to configure a non-3GPP model.
     *
     * An example usage of this constructor to set up a Friis propagation loss model is:
     * \code
     *   std::vector<std::reference_wrapper<std::unique_ptr<BandwidthPartInfo> > > bwps;
     *   std::unique_ptr<BandwidthPartInfo> bwpi (new BandwidthPartInfo (bwpId, centralFrequency,
     * bandwidth)); auto spectrumChannel = CreateObject<MultiModelSpectrumChannel> (); auto
     * propagationLoss = CreateObject<FriisPropagationLossModel> ();
     *   propagationLoss->SetAttributeFailSafe ("Frequency", DoubleValue (centralFrequencyBand1));
     *   spectrumChannel->AddPropagationLossModel (propagationLoss);
     *   bwpi->m_channel = spectrumChannel;
     *   bwps.push_back(bwpi);
     * \endcode
     */
    BandwidthPart(uint8_t bwpId,
                  double centralFrequency,
                  double channelBandwidth);

    Ptr<SpectrumChannel>
        m_channel; //!< Channel for the Bwp. Leave it nullptr to let the helper fill it
};

/**
 * \ingroup utils
 * \brief unique_ptr of BandwidthPartInfo
 */
typedef std::unique_ptr<BandwidthPart> BandwidthPartPtr;
/**
 * \ingroup utils
 * \brief unique_ptr of a const BandwidthPartInfo
 */
typedef std::unique_ptr<const BandwidthPart> BandwidthPartConstPtr;
/**
 * \ingroup utils
 * \brief vector of unique_ptr of BandwidthPartInfo
 */
typedef std::vector<std::reference_wrapper<BandwidthPartPtr>> BandwidthPartPtrVector;

std::ostream& operator<<(std::ostream& os, const BandwidthPart& item);

} // namespace ns3

#endif /* BWP_HELPER_H */
