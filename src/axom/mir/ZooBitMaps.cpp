// Copyright (c) 2017-2019, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include "ZooBitMaps.hpp"

namespace axom
{
namespace mir
{

  //      Quad Vertex Indices
  //
  //      0         7         3
  //      @---------@---------@
  //      |                   |
  //      |                   |
  //      |                   |
  //    4 @                   @ 6
  //      |                   |
  //      |                   |  
  //      |                   |
  //      @---------@---------@
  //      1         5         2
  //
  const int quadClipTable[16][19] = 
  {
      {4,0,1,2,3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
      {3,3,7,6,4,7,0,2,6,3,0,1,2,-1,-1,-1,-1,-1,-1},
      {3,6,5,2,4,3,1,5,6,3,0,1,3,-1,-1,-1,-1,-1,-1},
      {4,0,1,5,7,4,7,5,2,3,-1,-1,-1,-1,-1,-1,-1,-1},
      {3,4,1,5,4,0,4,5,2,3,0,2,3,-1,-1,-1,-1,-1,-1},
      {3,4,1,5,4,0,4,5,2,4,0,2,6,7,3,7,6,3,-1},
      {4,0,4,6,3,4,4,1,2,6,-1,-1,-1,-1,-1,-1,-1,-1,-1},
      {3,0,4,7,4,4,1,3,7,3,1,2,3,-1,-1,-1,-1,-1,-1},
      {3,0,4,7,4,4,1,3,7,3,1,2,3,-1,-1,-1,-1,-1,-1},
      {4,0,4,6,3,4,4,1,2,6,-1,-1,-1,-1,-1,-1,-1,-1,-1},
      {3,4,1,5,4,0,4,5,2,4,0,2,6,7,3,7,6,3,-1},
      {3,4,1,5,4,0,4,5,2,3,0,2,3,-1,-1,-1,-1,-1,-1},
      {4,0,1,5,7,4,7,5,2,3,-1,-1,-1,-1,-1,-1,-1,-1},
      {3,6,5,2,4,3,1,5,6,3,0,1,3,-1,-1,-1,-1,-1,-1},
      {3,3,7,6,4,7,0,2,6,3,0,1,2,-1,-1,-1,-1,-1,-1},
      {4,0,1,2,3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}
  };


  //      Triangle Vertex Indices
  //                  0
  //                  @
  //                 / \
  //                /   \
  //             3 @     @ 5
  //              /       \
  //             /         \
  //          1 @-----@-----@ 2
  //                  4
  const int triangleClipTable[8][10] = 
  {
    {3,0,1,2,-1,-1,-1,-1,-1,-1},
    {4,0,1,4,5,3,5,4,2,-1},
    {4,0,3,4,2,3,3,1,4,-1},
    {4,3,1,2,5,3,0,3,5,-1},
    {4,3,1,2,5,3,0,3,5,-1},
    {4,0,3,4,2,3,3,1,4,-1},
    {4,0,1,4,5,3,5,4,2,-1},
    {3,0,1,2,-1,-1,-1,-1,-1,-1}
  };
}
}