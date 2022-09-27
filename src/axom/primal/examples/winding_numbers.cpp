// Copyright (c) 2017-2022, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include "axom/config.hpp"

#include "axom/primal.hpp"
#include "axom/core.hpp"
#include "axom/slic.hpp"
#include "axom/fmt.hpp"

// C++ headers
#include <cmath>
#include <iostream>
#include <fstream>

namespace primal = axom::primal;
using Point2D = primal::Point<double, 2>;
using Vector2D = primal::Vector<double, 2>;

using Segment = primal::Segment<double, 2>;
using Triangle = primal::Triangle<double, 2>;
using Bezier = primal::BezierCurve<double, 2>;
using CPolygon = primal::CurvedPolygon<double, 2>;
using Polygon = primal::Polygon<double, 2>;

using vector_field = std::function<Vector2D(Point2D)>;

void winding_number_grid();

CPolygon get_self_intersecting_shape();
CPolygon get_large_shape();
CPolygon get_closed_shape();
CPolygon get_overlapping_shape();

int main(int argc, char** argv)
{
  AXOM_UNUSED_VAR(argc);
  AXOM_UNUSED_VAR(argv);

  axom::slic::SimpleLogger logger;
  winding_number_grid();

  return 0;
}

CPolygon get_split_square_shape()
{
  Point2D nodes1[] = {Point2D {0.0, 0.0}, Point2D {1.0, 0.0}};
  Bezier segment1(nodes1, 1);

  Point2D nodes2[] = {Point2D {1.0, 0.0}, Point2D {1.1, 0.9}};
  Bezier segment2(nodes2, 1);

  Point2D nodes3[] = {Point2D {0.9, 1.1}, Point2D {0.0, 1.0}};
  Bezier segment3(nodes3, 1);

  Point2D nodes4[] = {Point2D {0.0, 1.0}, Point2D {0.0, 0.0}};
  Bezier segment4(nodes4, 1);

  Point2D cross_nodes[] = {Point2D {0.8, 0.8},
                           Point2D {0.25, 0.75},
                           Point2D {0.2, 0.2}};
  Bezier cross_segment(cross_nodes, 2);

  Bezier square_shape[] = {segment1, segment2, segment3, segment4};
  return CPolygon(square_shape, 4);
}

CPolygon get_approx_circle()
{
  Point2D nodes1[] = {Point2D {1.0 * .75, 0.0 * .75},
                      Point2D {1.0 * .75, 1.0 * .75},
                      Point2D {0.0 * .75, 1.0 * .75}};
  Bezier segment1(nodes1, 2);

  Point2D nodes2[] = {Point2D {0.0 * .75, 1.0 * .75},
                      Point2D {-1.0 * .75, 1.0 * .75},
                      Point2D {-1.0 * .75, 0.0 * .75}};
  Bezier segment2(nodes2, 2);

  Point2D nodes3[] = {Point2D {-1.0 * .75, 0.0 * .75},
                      Point2D {-1.0 * .75, -1.0 * .75},
                      Point2D {0.0 * .75, -1.0 * .75}};
  Bezier segment3(nodes3, 2);

  Point2D nodes4[] = {Point2D {0.0 * .75, -1.0 * .75},
                      Point2D {1.0 * .75, -1.0 * .75},
                      Point2D {1.0 * .75, 0.0 * .75}};
  Bezier segment4(nodes4, 2);

  Bezier circle_shape[] = {segment1, segment2, segment3, segment4};
  return CPolygon(circle_shape, 4);
}

CPolygon get_closed_shape()
{
  // self intersecting cubic
  // Simple cubic shape for testing intersecting cubic
  Point2D top_nodes[] = {Point2D {0.0, 0.0},
                         Point2D {0.0, 1.0},
                         Point2D {-1.0, 1.0},
                         Point2D {0.0, 0.0}};
  Bezier top_curve(top_nodes, 3);

  Point2D bot_nodes[] = {Point2D {-1.0, 0.0},
                         Point2D {-1.0, -1.0},
                         Point2D {0.0, -1.0},
                         Point2D {0.0, 0.0}};
  Bezier bot_curve(bot_nodes, 3);
  Bezier cusp_shape[] = {top_curve};
  return CPolygon(cusp_shape, 1);
}

CPolygon get_overlapping_shape()
{
  // self intersecting cubic
  Point2D nodes[] = {Point2D {0.0, 0.0},
                     Point2D {-1.0, 1.0},
                     Point2D {-1.0, -1.0},
                     Point2D {0.0, 0.0}};
  Bezier curve(nodes, 3);

  Point2D nodes4[] = {Point2D {0.0, -0.1}, Point2D {0.0, -0.1}};
  Bezier segment4(nodes4, 1);

  Bezier shape[] = {curve};
  return CPolygon(shape, 1);
}

CPolygon get_very_overlapping_shape()
{
  // self intersecting cubic
  Point2D nodes[] = {Point2D {-0.337, -0.155},
                     Point2D {0.681, 0.532},
                     Point2D {-1.206, 0.312},
                     Point2D {-0.635, -0.181},
                     Point2D {-0.104, -0.382},
                     Point2D {-0.045, 0.247}};
  Bezier curve(nodes, 5);
  Bezier shape[] = {curve};
  return CPolygon(shape, 1);
}

CPolygon get_self_intersecting_shape()
{
  // self intersecting cubic
  Point2D big_nodes[] = {Point2D {0.0, 0.0},
                         Point2D {0.0, 2.0},
                         Point2D {-3.0, -1.0},
                         Point2D {1.5, -1.0},
                         Point2D {1.5, 2.0},
                         Point2D {-3.0, 2.0},
                         Point2D {0.0, -1.0},
                         Point2D {0.0, 1.0}};
  Bezier super_intersecting(big_nodes, 7);

  Point2D closure_closure[] = {Point2D {0.0, 0.0}, Point2D {0.0, 1.0}};
  Bezier closure_curve(closure_closure, 1);

  Point2D small_nodes[] = {Point2D {0.0, 1.0},
                           Point2D {-2.0, 0.5},
                           Point2D {0.0, 0.0}};

  Bezier intersecting_closure(small_nodes, 2);
  Bezier super_intersecting_nodes[] = {super_intersecting, intersecting_closure};
  return CPolygon(super_intersecting_nodes, 2);
}

CPolygon get_figure_shape()
{
  // self intersecting cubic
  Point2D big_nodes[] = {Point2D {0.0, 0.0},
                         Point2D {0.0, 2.0},
                         Point2D {-2.0, 2.0}};
  Bezier super_intersecting(big_nodes, 2);

  Point2D clll[] = {Point2D {-2.0, 2.0},
                    Point2D {-4.5, 4.0},
                    Point2D {-2.0, -2.0}};
  Bezier closure_closure(clll, 2);

  Point2D small_nodes[] = {Point2D {-2.0, -2.0}, Point2D {0.0, 0.0}};

  Bezier intersecting_closure(small_nodes, 1);
  Bezier super_intersecting_nodes[] = {super_intersecting,
                                       intersecting_closure,
                                       closure_closure};
  return CPolygon(super_intersecting_nodes, 3);
}

CPolygon get_large_shape()
{
  // self intersecting cubic
  Point2D nodes1[] = {Point2D {0.0, 0.0},
                      Point2D {2.0, 1.0},
                      Point2D {-1.0, 1.0},
                      Point2D {0.9, 0.0}};
  Bezier self_intersecting_cubic(nodes1, 3);

  // kind-of circlish cubic
  Point2D nodes2[] = {Point2D {0.8, 0.0},
                      Point2D {2.0, 0.0},
                      Point2D {2.0, -1.0},
                      Point2D {0.7, -1.1}};
  Bezier semicircleish_cubic(nodes2, 3);

  // straight line segment
  Point2D nodes3[] = {Point2D {1.0, -1.0}, Point2D {-1.0, -2.0}};
  Bezier segment_linear(nodes3, 1);

  // parabola that intersects with the straight line
  Point2D nodes4[] = {Point2D {-1.0, -2.0},
                      Point2D {1.0, -3.0},
                      Point2D {-1.0, -1.1}};
  Bezier intersecting_parabola(nodes4, 2);

  // semi-circleish parabola that closes the curve
  Point2D nodes5[] = {Point2D {-1.0, -1.0},
                      Point2D {-1.0, 0.0},
                      Point2D {-0.1, 0.0}};
  Bezier semicircleish_parabola(nodes5, 2);

  Bezier pedges[] = {self_intersecting_cubic,
                     semicircleish_cubic,
                     semicircleish_parabola,
                     segment_linear,
                     intersecting_parabola};
  return CPolygon(pedges, 5);
}

void winding_number_grid()
{
  CPolygon cpoly = get_self_intersecting_shape();
 
  // Get big ol grid of query points
  Bezier::BoundingBoxType cpbb(cpoly.boundingBox().scale(2));
  const int num_pts = 399;
  double xpts[num_pts];
  double ypts[num_pts];

  Point2D the_pt = Point2D({0.0, 0.0});
  double ran = 1.0;

  axom::numerics::linspace(the_pt[0] - ran, the_pt[0] + ran, xpts, num_pts);
  axom::numerics::linspace(the_pt[1] - ran, the_pt[1] + ran, ypts, num_pts);

  // Get file storage syntax
  std::ofstream outfile_data(
    "E:/Code/winding_number/griddata.csv");
  if(!outfile.good())
  {
    std::cout << "Could not write to the data file" << std::endl;
    return;
  }

  std::ofstream outfile_curves("E:/Code/winding_number/curve.csv");
  if(!outfile.good())
  {
    std::cout << "Could not write to the curve file" << std::endl;
    return;
  }

  std::cout << cpoly << std::endl;

  // Loop over each query point, store it and the computed winding number there
  for(double& x : xpts)
  {
    for(double& y : ypts)
    {
      Point2D qpoint({x, y});
      // clang-format off
      double winding_num = winding_number(qpoint, cpoly);
      outfile << axom::fmt::format(
        "{0},{1},{2}\n",
        qpoint[0],
        qpoint[1],
        winding_num);
      // clang-format on
    }
    std::cout << ".";
  }
  std::cout << std::endl;
}
