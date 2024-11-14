// Copyright (c) 2015 Danilo Abrignani
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Danilo Abrignani <danilo.abrignani@unibo.it>

#include "nr-component-carrier.h"

#include <ns3/abort.h>
#include <ns3/boolean.h>
#include <ns3/log.h>
#include <ns3/uinteger.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrComponentCarrier");

NS_OBJECT_ENSURE_REGISTERED(NrComponentCarrier);

TypeId
NrComponentCarrier::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrComponentCarrier")
            .SetParent<Object>()
            .AddAttribute("Bandwidth",
                          "Transmission Bandwidth Configuration in number of Resource Blocks",
                          UintegerValue(25),
                          MakeUintegerAccessor(&NrComponentCarrier::SetBandwidth,
                                               &NrComponentCarrier::GetBandwidth),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("Earfcn",
                          "Downlink E-UTRA Absolute Radio Frequency Channel Number (EARFCN) "
                          "as per 3GPP 36.101 Section 5.7.3.",
                          UintegerValue(100),
                          MakeUintegerAccessor(&NrComponentCarrier::SetEarfcn,
                                               &NrComponentCarrier::GetEarfcn),
                          MakeUintegerChecker<uint32_t>(0, 262143))
            .AddAttribute(
                "CsgId",
                "The Closed Subscriber Group (CSG) identity that this eNodeB belongs to",
                UintegerValue(0),
                MakeUintegerAccessor(&NrComponentCarrier::SetCsgId, &NrComponentCarrier::GetCsgId),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "CsgIndication",
                "If true, only UEs which are members of the CSG (i.e. same CSG ID) "
                "can gain access to the eNodeB, therefore enforcing closed access mode. "
                "Otherwise, the eNodeB operates as a non-CSG cell and implements open access mode.",
                BooleanValue(false),
                MakeBooleanAccessor(&NrComponentCarrier::SetCsgIndication,
                                    &NrComponentCarrier::GetCsgIndication),
                MakeBooleanChecker())
            .AddAttribute(
                "PrimaryCarrier",
                "If true, this Carrier Component will be the Primary Carrier Component (PCC) "
                "Only one PCC per eNodeB is (currently) allowed",
                BooleanValue(false),
                MakeBooleanAccessor(&NrComponentCarrier::SetAsPrimary,
                                    &NrComponentCarrier::IsPrimary),
                MakeBooleanChecker());
    return tid;
}

NrComponentCarrier::NrComponentCarrier()
    : Object()
{
    NS_LOG_FUNCTION(this);
}

NrComponentCarrier::~NrComponentCarrier()
{
    NS_LOG_FUNCTION(this);
}

void
NrComponentCarrier::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Object::DoDispose();
}

uint16_t
NrComponentCarrier::GetBandwidth() const
{
    return m_bandwidth;
}

void
NrComponentCarrier::SetBandwidth(uint16_t bw)
{
    m_bandwidth = bw;
}

uint32_t
NrComponentCarrier::GetEarfcn() const
{
    return m_earfcn;
}

void
NrComponentCarrier::SetEarfcn(uint32_t earfcn)
{
    m_earfcn = earfcn;
}

uint32_t
NrComponentCarrier::GetCsgId() const
{
    return m_csgId;
}

void
NrComponentCarrier::SetCsgId(uint32_t csgId)
{
    NS_LOG_FUNCTION(this << csgId);
    m_csgId = csgId;
}

bool
NrComponentCarrier::GetCsgIndication() const
{
    return m_csgIndication;
}

void
NrComponentCarrier::SetCsgIndication(bool csgIndication)
{
    NS_LOG_FUNCTION(this << csgIndication);
    m_csgIndication = csgIndication;
}

bool
NrComponentCarrier::IsPrimary() const
{
    return m_primaryCarrier;
}

void
NrComponentCarrier::SetAsPrimary(bool primaryCarrier)
{
    NS_LOG_FUNCTION(this << primaryCarrier);
    m_primaryCarrier = primaryCarrier;
}

} // namespace ns3
