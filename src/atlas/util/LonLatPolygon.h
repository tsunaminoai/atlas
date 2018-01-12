/*
 * (C) Copyright 1996-2018 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#pragma once

#include <vector>
#include "atlas/util/Polygon.h"

namespace atlas {
namespace util {

//------------------------------------------------------------------------------------------------------

class LonLatPolygon : public PolygonCoordinates {
public:

    // -- Constructors

    LonLatPolygon(const Polygon&, const atlas::Field& lonlat, bool includesNorthPole, bool includesSouthPole, bool removeAlignedPoints = true);

    LonLatPolygon(const std::vector<PointLonLat>& points, bool includesNorthPole, bool includesSouthPole);

    // -- Overridden methods

    /*
     * Point-in-polygon test based on winding number
     * @note reference <a href="http://geomalgorithms.com/a03-_inclusion.html">Inclusion of a Point in a Polygon</a>
     * @param[in] P given point
     * @return if point is in polygon
     */
    bool contains(const PointLonLat& P) const;

};

//------------------------------------------------------------------------------------------------------

}  // util
}  // atlas

