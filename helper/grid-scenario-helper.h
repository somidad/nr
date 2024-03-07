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
 * @brief Helper to place Base Stations and User Terminals in a grid-like scenario.
 *
 * Set the Base Stations'(gNB) locations in a grid-like pattern. User Terminals are then spread over
 * it. SetBsDistance(horizontal_distance, verticle_distance) specifies the spacing between Base
 * Stations in meters. The number of rows and columns where the Base Stations will be placed in a
 * grid-like scenario are set using SetRows(r) and SetColumns(c). SetGridSize(x,y) sets the overall
 * dimensions in meters of grid-like scenario where Base Stations and User Terminals both will be
 * present. The starting position of grid in meters is set using SetBsPositionOffset function.
 */
class GridScenarioHelper : public NodeDistributionScenarioInterface
{
  public:
    /**
     * \brief Default constructor.
     */
    GridScenarioHelper();

    /**
     * \brief Default destructor.
     */
    ~GridScenarioHelper() override;

    /**
     * @brief Set a fixed horizontal and vertical (x axis, y axis) distance (meters) between Base
     * Stations.
     */
    void SetBsDistance(double hDistance, double vDistance);

    /**
     * @brief Set the amount of rows (y axis) for the placement of Base Stations.
     */
    void SetRows(uint32_t r);

    /**
     * @brief Set the amount of columns (x axis) for the placement of Base Stations.
     */
    void SetColumns(uint32_t c);

    /**
     * \brief Set starting position of the grid
     * \param [in] initialPos The starting position vector (x, y, z) that is treated as offset for.
     */
    void SetBsPositionOffset(const Vector& initialPos);

    void SetGridSize(double m, double n); // length m, width n

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
    Vector m_initialPos;                 //!< Initial Position
    double m_length{0};                  //!< Scenario length
    double m_width{0};                   //!< Scenario width
    Ptr<UniformRandomVariable> m_x;      //!< Random variable for X coordinate
    Ptr<UniformRandomVariable> m_y;      //!< Random variable for Y coordinate
};

} // namespace ns3
#endif // GRID_SCENARIO_HELPER_H
