/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef GRID_SCENARIO_HELPER_H
#define GRID_SCENARIO_HELPER_H

#include "node-distribution-scenario-interface.h"

#include <ns3/random-variable-stream.h>
#include <ns3/vector.h>

namespace ns3
{

/**
 * @brief The GridScenarioHelper class
 *
 * The grid is structured as follows:
 *
 * initialPos => gNB1----m_horizontalBsDistance----gNB2------------------------------gNB3
 *                |                                  |                                  |
 *                |                        m_verticalBsDistance                         |
 *                |                                  |                                  |
 *               gNB4------------------------------gNB5------------------------------gNB6
 *
   In the above topology, there are 2 rows and 3 columns.
 */
class GridScenarioHelper : public NodeDistributionScenarioInterface
{
  public:
    /**
     * \brief GridScenarioHelper
     * \brief A helper to place gNBs and User Terminals on a grid.
     */
    GridScenarioHelper();

    /**
     * \brief ~GridScenarioHelper
     */
    ~GridScenarioHelper() override;
    /**
     * @brief SetRows
     * \param r number of rows of the grid
     */
    void SetRows(uint32_t r);

    /**
     * @brief SetColumns
     * \param c number of columns of the grid
     */
    void SetColumns(uint32_t c);

    /**
     * \brief Set starting position of the grid
     * \param [in] initialPos The starting position vector (x, y, z), where z is ignored.
     */
    void SetBsPositionOffset(const Vector& initialPos);

    /**
     * \brief Set the boundaries for the grid
     * \param maxDistanceX The maximum distance (meters) UTs can be away from the gNB on the X-axis.
     * \param maxDistanceY the maximum distance (meters) UTs can be away from the gNB on the Y-axis.
     */
    void SetGnbCoverage(double maxDistanceX, double maxDistanceY);

    /**
     * \brief Set the boundaries for the grid
     * \param hDistance The horizontal distance (meters) between each two gNBs.
     * \param vDistance The vertical distance (meters) between each two gNBs.
     */
    void SetBsDistance(double hDistance, double vDistance);

    // inherited
    void CreateScenario() override;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  private:
    double m_verticalBsDistance{-1.0};   //!< Distance between gnb
    double m_horizontalBsDistance{-1.0}; //!< Distance between gnb
    uint32_t m_rows{0};                  //!< Grid rows
    uint32_t m_columns{0};               //!< Grid columns
    Vector m_initialPos; //!< Initial Position from where the grid starts placing gNBs
    double m_maxDistanceX{
        0}; //!< The maximum distance (meters) UTs can be away from the gNB on the X-axis.
    double m_maxDistanceY{
        0}; //!< The maximum distance (meters) UTs can be away from the gNB on the Y-axis.
    Ptr<UniformRandomVariable> m_x; //!< Random variable for X coordinate
    Ptr<UniformRandomVariable> m_y; //!< Random variable for Y coordinate
};

} // namespace ns3
#endif // GRID_SCENARIO_HELPER_H
