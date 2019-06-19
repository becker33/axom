// Copyright (c) 2017-2019, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#ifndef __CELL_DATA_H__
#define __CELL_DATA_H__

#include "axom/core.hpp"  // for axom macros
#include "axom/slam.hpp"

#include "MIRMeshTypes.hpp"

namespace numerics = axom::numerics;
namespace slam = axom::slam;

namespace axom
{
namespace mir
{

  // Struct for collecting data that specifies a mesh or cell's connectivity/topology.
  struct CellTopologyData
  {
      std::vector<PosType> evInds;
      std::vector<PosType> evBegins;
      std::vector<PosType> veInds;
      std::vector<PosType> veBegins;
  };

  // Struct for collecting the data that will populate a mesh's map data structures.
  struct CellMapData
  {
      std::vector<mir::Point2> vertexPositions;                               // Data that goes into MIRMesh's vertexPositions PointMap
      std::vector<std::vector<axom::float64> > vertexVolumeFractions;         // Data that goes into MIRMesh's materialVolumeFractionsVertex ScalarMap
      std::vector<int> elementDominantMaterials;                              // Data that goes into MIRMesh's elementDominantColors IntMap
      std::vector<int> elementParents;                                        // Data that goes into MIRMesh's elementParentIDs IntMap
  };

  /// Represents an arbitrary number of cells that are within the same local coordinate system (i.e. share a set of vertices and elements).
  /// Intended to be used as a helper class to hold intermediate data while processing a mesh, and to be used as input to the MIRMesh class to fully initialize it.
  class CellData
  {
    public:
      CellData();
      ~CellData();

      void mergeCell(CellData cellToMerge);

    public:
      int numVerts;
      int numElems;

      CellTopologyData topology;
      CellMapData mapData;
  };
}
}

#endif