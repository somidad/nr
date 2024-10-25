/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SCENARIO_HELPER_H
#define SCENARIO_HELPER_H

#include <ns3/propagation-loss-model.h>
#include <ns3/ptr.h>
#include <ns3/spectrum-channel.h>
#include <ns3/spectrum-propagation-loss-model.h>

#include <memory>
#include <vector>

namespace ns3
{

struct ScenarioHelper
{
    enum Scenario
    {
        RMa,                   //!< RMa
        RMa_LoS,               //!< RMa where all the nodes will be in Line-of-Sight
        RMa_nLoS,              //!< RMA where all the nodes will not be in Line-of-Sight
        UMa_LoS,               //!< UMa where all the nodes will be in Line-of-Sight
        UMa_nLoS,              //!< UMa where all the nodes will not be in Line-of-Sight
        UMa,                   //!< UMa
        UMi_StreetCanyon,      //!< UMi_StreetCanyon
        UMi_StreetCanyon_LoS,  //!< UMi_StreetCanyon where all the nodes will be in Line-of-Sight
        UMi_StreetCanyon_nLoS, //!< UMi_StreetCanyon where all the nodes will not be in
                               //!< Line-of-Sight
        InH_OfficeOpen,        //!< InH_OfficeOpen
        InH_OfficeOpen_LoS,    //!< indoor office where all the nodes will be in Line-of-Sight
        InH_OfficeOpen_nLoS,   //!< indoor office where all the nodes will not be in Line-of-Sight
        InH_OfficeMixed,       //!< InH_OfficeMixed
        InH_OfficeMixed_LoS,   //!< indoor office where all the nodes will be in Line-of-Sight
        InH_OfficeMixed_nLoS,  //!< indoor office where all the nodes will not be in Line-of-Sight
        UMa_Buildings,         //!< UMa with buildings
        UMi_Buildings,         //!< UMi_StreetCanyon with buildings
        V2V_Highway,           //!< V2V_Highway
        V2V_Urban,             //!< V2V_Urban
        Custom                 //!< User-defined custom scenario
    } m_scenario{RMa};

    /**
     * \brief Retrieve a string version of the scenario
     * \return the string-fied version of the scenario
     */
    std::string GetScenario() const;

    Ptr<SpectrumChannel>
        m_channel; //!< Channel for the Bwp. Leave it nullptr to let the helper fill it
    Ptr<PropagationLossModel>
        m_propagation; //!< Propagation model. Leave it nullptr to let the helper fill it
    Ptr<PhasedArraySpectrumPropagationLossModel>
        m_3gppChannel; //!< Nr Channel. Leave it nullptr to let the helper fill it
};

} // namespace ns3

#endif /* SCENARIO_HELPER_H */
