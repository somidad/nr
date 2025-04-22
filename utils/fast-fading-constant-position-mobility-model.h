// Copyright (c) 2025 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef FAST_FADING_CONSTANT_POSITION_MOBILITY_MODEL_H
#define FAST_FADING_CONSTANT_POSITION_MOBILITY_MODEL_H

#include "ns3/constant-position-mobility-model.h"

namespace ns3
{

/**
 * @ingroup nr-utils
 * @brief Fast fading constant position mobility model is used to allow
 * the generation of smooth fast fading 3GPP channel updates due to the velocity
 * that can be set to this model, even if the nodes are actually not moving.
 * Such generation of these small fast fading channel updates can be interesting
 * to have when evaluating some features using the channel state information.
 *
 * @see See for example cttc-3gpp-indoor-calibration.cc example.
 */
class FastFadingConstantPositionMobilityModel : public ConstantPositionMobilityModel
{
  public:
    static TypeId GetTypeId();

    Vector m_fakeVelocity{0.0,
                          0.0,
                          0.0}; // fake velocity that can be set through the attribute FakeVelocity

  private:
    Vector DoGetVelocity() const override;
};

NS_OBJECT_ENSURE_REGISTERED(FastFadingConstantPositionMobilityModel);

TypeId
FastFadingConstantPositionMobilityModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::FastFadingConstantPositionMobilityModel")
            .SetParent<ConstantPositionMobilityModel>()
            .SetGroupName("Mobility")
            .AddConstructor<FastFadingConstantPositionMobilityModel>()
            .AddAttribute(
                "FakeVelocity",
                "The current velocity of the mobility model.",
                VectorValue(Vector(0.0, 0.0, 0.0)), // ignored initial value.
                MakeVectorAccessor(&FastFadingConstantPositionMobilityModel::m_fakeVelocity),
                MakeVectorChecker());
    return tid;
}

Vector
FastFadingConstantPositionMobilityModel::DoGetVelocity() const
{
    return m_fakeVelocity;
}

} // namespace ns3
#endif /* FAST_FADING_CONSTANT_POSITION_MOBILITY_MODEL_H */
