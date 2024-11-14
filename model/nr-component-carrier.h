// Copyright (c) 2015 Danilo Abrignani
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Danilo Abrignani <danilo.abrignani@unibo.it>

#ifndef NR_COMPONENT_CARRIER_H
#define NR_COMPONENT_CARRIER_H

#include <ns3/object.h>

namespace ns3
{

/**
 * \ingroup nr
 *
 * NrComponentCarrier Object, it defines a single Carrier
 * This is the parent class for both NrComponentCarrier
 * and ComponentCarrierUe.
 * This class contains the main physical configuration
 * parameters for a carrier. Does not contain pointers to
 * the MAC/PHY objects of the carrier.
 */
class NrComponentCarrier : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    NrComponentCarrier();

    ~NrComponentCarrier() override;
    void DoDispose() override;
    /**
     * \return the bandwidth in RBs
     */
    uint16_t GetBandwidth() const;

    /**
     * \param bw the bandwidth in RBs
     */
    virtual void SetBandwidth(uint16_t bw);

    /**
     * \return the carrier frequency (EARFCN)
     */
    uint32_t GetEarfcn() const;

    /**
     * \param earfcn the carrier frequency (EARFCN)
     */
    virtual void SetEarfcn(uint32_t earfcn);

    /**
     * \brief Returns the CSG ID of the eNodeB.
     * \return the Closed Subscriber Group identity
     * \sa NrGnbNetDevice::SetCsgId
     */
    uint32_t GetCsgId() const;

    /**
     * \brief Associate the eNodeB device with a particular CSG.
     * \param csgId the intended Closed Subscriber Group identity
     *
     * CSG identity is a number identifying a Closed Subscriber Group which the
     * cell belongs to. eNodeB is associated with a single CSG identity.
     *
     * The same CSG identity can also be associated to several UEs, which is
     * equivalent as enlisting these UEs as the members of this particular CSG.
     *
     * \sa NrGnbNetDevice::SetCsgIndication
     */
    void SetCsgId(uint32_t csgId);

    /**
     * \brief Returns the CSG indication flag of the eNodeB.
     * \return the CSG indication flag
     * \sa NrGnbNetDevice::SetCsgIndication
     */
    bool GetCsgIndication() const;

    /**
     * \brief Enable or disable the CSG indication flag.
     * \param csgIndication if TRUE, only CSG members are allowed to access this
     *                      cell
     *
     * When the CSG indication field is set to TRUE, only UEs which are members of
     * the CSG (i.e. same CSG ID) can gain access to the eNodeB, therefore
     * enforcing closed access mode. Otherwise, the eNodeB operates as a non-CSG
     * cell and implements open access mode.
     *
     * \note This restriction only applies to initial cell selection and
     *       EPC-enabled simulation.
     *
     * \sa NrGnbNetDevice::SetCsgIndication
     */
    void SetCsgIndication(bool csgIndication);

    /**
     * \brief Set as primary carrier
     * \param primaryCarrier true to set as primary carrier
     */
    void SetAsPrimary(bool primaryCarrier);

    /**
     * \brief Checks if the carrier is the primary carrier
     * \returns true if the carrier is primary
     */
    bool IsPrimary() const;

  protected:
    uint32_t m_csgId{0};          ///< CSG ID
    bool m_csgIndication{false};  ///< CSG indication
    bool m_primaryCarrier{false}; ///< whether the carrier is primary
    uint16_t m_bandwidth{0};      ///< bandwidth in RBs */
    uint32_t m_earfcn{0};         ///< carrier frequency */
};

} // namespace ns3

#endif /* NR_COMPONENT_CARRIER_H */
