// Copyright (c) 2017-2021, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/*!
 * \file shaping_driver.cpp
 * \brief Driver for shaping material volume fractions onto a simulation mesh
 */

// Axom includes
#include "axom/core.hpp"
#include "axom/slic.hpp"
#include "axom/slam.hpp"
#include "axom/sidre.hpp"
#include "axom/primal.hpp"
#include "axom/spin.hpp"
#include "axom/klee.hpp"
#include "axom/mint.hpp"
#include "axom/quest.hpp"

#include "fmt/fmt.hpp"
#include "fmt/locale.h"
#include "CLI11/CLI11.hpp"

#include "mfem.hpp"

// C/C++ includes
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <fstream>
#include <iomanip>  // for setprecision()

#ifdef AXOM_USE_MPI
  #include "mpi.h"
#endif

// NOTE: Axom must be configured with AXOM_ENABLE_MFEM_SIDRE_DATACOLLECTION compiler define for the klee containment driver

namespace mint = axom::mint;
namespace quest = axom::quest;
namespace slic = axom::slic;
namespace sidre = axom::sidre;
namespace klee = axom::klee;

using QFunctionCollection = mfem::NamedFieldsMap<mfem::QuadratureFunction>;
using DenseTensorCollection = mfem::NamedFieldsMap<mfem::DenseTensor>;

using VolFracSampling = quest::shaping::VolFracSampling;

//------------------------------------------------------------------------------

/** Struct to parse and store the input parameters */
struct Input
{
public:
  std::string meshFile;

  std::string shapeFile;
  klee::ShapeSet shapeSet;

  int quadratureOrder {5};
  int outputOrder {2};
  int samplesPerKnotSpan {25};
  double weldThresh {1e-9};

  VolFracSampling vfSampling {VolFracSampling::SAMPLE_AT_QPTS};

  std::vector<double> queryBoxMins;
  std::vector<double> queryBoxMaxs;

private:
  bool m_verboseOutput {false};

public:
  Input() = default;

  bool isVerbose() const { return m_verboseOutput; }

  std::string getDCMeshName() const
  {
    using axom::utilities::string::removeSuffix;

    // Remove the parent directories and file suffix
    std::string name = axom::Path(meshFile).baseName();
    name = removeSuffix(name, ".root");

    return name;
  }

  void parse(int argc, char** argv, CLI::App& app)
  {
    app.add_option("-m,--mesh-file", meshFile)
      ->description(
        "Path to computational mesh (generated by MFEMSidreDataCollection)")
      ->check(CLI::ExistingFile)
      ->required();

    app.add_option("-i,--shape-file", shapeFile)
      ->description("Path to input shape file")
      ->check(CLI::ExistingFile)
      ->required();

    app.add_flag("-v,--verbose", m_verboseOutput)
      ->description("Enable/disable verbose output")
      ->capture_default_str();

    app.add_option("-o,--order", outputOrder)
      ->description("order of the output grid function")
      ->capture_default_str()
      ->check(CLI::NonNegativeNumber);

    app.add_option("-q,--quadrature-order", quadratureOrder)
      ->description(
        "Quadrature order for sampling the inout field. \n"
        "Determines number of samples per element in determining "
        "volume fraction field")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);

    std::map<std::string, VolFracSampling> vfsamplingMap {
      {"qpts", VolFracSampling::SAMPLE_AT_QPTS},
      {"dofs", VolFracSampling::SAMPLE_AT_DOFS}};
    app.add_option("-s,--sampling-type", vfSampling)
      ->description(
        "Sampling strategy. \n"
        "Sampling either at quadrature points or collocated with "
        "degrees of freedom")
      ->capture_default_str()
      ->transform(CLI::CheckedTransformer(vfsamplingMap, CLI::ignore_case));

    app.add_option("-n,--segments-per-knot-span", samplesPerKnotSpan)
      ->description(
        "(2D only) Number of linear segments to generate per NURBS knot span")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);

    app.add_option("-t,--weld-threshold", weldThresh)
      ->description("Threshold for welding")
      ->check(CLI::NonNegativeNumber)
      ->capture_default_str();

    app.get_formatter()->column_width(35);

    // could throw an exception
    app.parse(argc, argv);

    slic::setLoggingMsgLevel(m_verboseOutput ? slic::message::Debug
                                             : slic::message::Info);
  }
};

/**
 * \brief Print some info about the mesh
 *
 * \note In MPI-based configurations, this is a collective call, but only prints on rank 0
 */
void printMeshInfo(mfem::Mesh* mesh, const std::string& prefixMessage = "")
{
  namespace primal = axom::primal;

  int myRank = 0;
#ifdef AXOM_USE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
#endif

  int numElements = mesh->GetNE();

  mfem::Vector mins, maxs;
#ifdef MFEM_USE_MPI
  auto* pmesh = dynamic_cast<mfem::ParMesh*>(mesh);
  if(pmesh != nullptr)
  {
    pmesh->GetBoundingBox(mins, maxs);
    numElements = pmesh->ReduceInt(numElements);
    myRank = pmesh->GetMyRank();
  }
  else
#endif
  {
    mesh->GetBoundingBox(mins, maxs);
  }

  if(myRank == 0)
  {
    switch(mesh->Dimension())
    {
    case 2:
      SLIC_INFO(fmt::format(
        "{} mesh has {} elements and (approximate) bounding box {}",
        prefixMessage,
        numElements,
        primal::BoundingBox<double, 2>(primal::Point<double, 2>(mins.GetData()),
                                       primal::Point<double, 2>(maxs.GetData()))));
      break;
    case 3:
      SLIC_INFO(fmt::format(
        "{} mesh has {} elements and (approximate) bounding box {}",
        prefixMessage,
        numElements,
        primal::BoundingBox<double, 3>(primal::Point<double, 3>(mins.GetData()),
                                       primal::Point<double, 3>(maxs.GetData()))));
      break;
    }
  }

  slic::flushStreams();
}

/// \brief Utility function to initialize the logger
void initializeLogger()
{
  // Initialize Logger
  slic::initialize();
  slic::setLoggingMsgLevel(slic::message::Info);

  slic::LogStream* logStream {nullptr};

#ifdef AXOM_USE_MPI
  int num_ranks = 1;
  MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
  if(num_ranks > 1)
  {
    std::string fmt = "[<RANK>][<LEVEL>]: <MESSAGE>\n";
  #ifdef AXOM_USE_LUMBERJACK
    const int RLIMIT = 8;
    logStream =
      new slic::LumberjackStream(&std::cout, MPI_COMM_WORLD, RLIMIT, fmt);
  #else
    logStream = new slic::SynchronizedStream(&std::cout, MPI_COMM_WORLD, fmt);
  #endif
  }
  else
#endif  // AXOM_USE_MPI
  {
    std::string fmt = "[<LEVEL>]: <MESSAGE>\n";
    logStream = new slic::GenericOutputStream(&std::cout, fmt);
  }

  slic::addStreamToAllMsgLevels(logStream);
}

/// \brief Utility function to finalize the logger
void finalizeLogger()
{
  if(slic::isInitialized())
  {
    slic::flushStreams();
    slic::finalize();
  }
}

//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
#ifdef AXOM_USE_MPI
  MPI_Init(&argc, &argv);
  int my_rank, num_ranks;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
#else
  int my_rank = 0;
  int num_ranks = 1;
#endif

  initializeLogger();

  //---------------------------------------------------------------------------
  // Set up and parse command line arguments
  //---------------------------------------------------------------------------
  Input params;
  CLI::App app {"Driver for Klee shaping query"};

  try
  {
    params.parse(argc, argv, app);
  }
  catch(const CLI::ParseError& e)
  {
    int retval = -1;
    if(my_rank == 0)
    {
      retval = app.exit(e);
    }
    finalizeLogger();

#ifdef AXOM_USE_MPI
    MPI_Bcast(&retval, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Finalize();
#endif
    exit(retval);
  }

  //---------------------------------------------------------------------------
  // Load the klee shape file and extract some information
  //---------------------------------------------------------------------------
  params.shapeSet = klee::readShapeSet(params.shapeFile);
  const klee::Dimensions shapeDim = params.shapeSet.getDimensions();

  // Apply error checking
#ifndef AXOM_USE_C2C
  SLIC_ERROR_IF(shapeDim == klee::Dimension::Two,
                "Shaping with contour files requires an Axom configured with "
                "the C2C library");
#endif

  //---------------------------------------------------------------------------
  // Load the computational mesh
  //---------------------------------------------------------------------------
  const bool dc_owns_data = true;
  sidre::MFEMSidreDataCollection originalMeshDC(params.getDCMeshName(),
                                                nullptr,
                                                dc_owns_data);
  originalMeshDC.SetComm(MPI_COMM_WORLD);
  {
    std::string protocol = "sidre_hdf5";
    originalMeshDC.Load(params.meshFile, protocol);
  }

  //---------------------------------------------------------------------------
  // Set up DataCollection for shaping
  //---------------------------------------------------------------------------
  sidre::MFEMSidreDataCollection shapingDC("shaping", nullptr, dc_owns_data);
  {
    shapingDC.SetMeshNodesName("positions");

    auto* pmesh = dynamic_cast<mfem::ParMesh*>(originalMeshDC.GetMesh());
    mfem::Mesh* shapingMesh = (pmesh != nullptr)
      ? new mfem::ParMesh(*pmesh)
      : new mfem::Mesh(*originalMeshDC.GetMesh());
    shapingDC.SetMesh(shapingMesh);
  }
  printMeshInfo(shapingDC.GetMesh(), "After loading");

  //---------------------------------------------------------------------------
  // Initialize the shaping query object
  //---------------------------------------------------------------------------
  quest::MFEMShaping shaper(params.shapeSet, &shapingDC);

  const int sampleOrder = params.quadratureOrder;
  const int outputOrder = params.outputOrder;

  shaper.setSamplesPerKnotSpan(params.samplesPerKnotSpan);
  shaper.setVertexWeldThreshold(params.weldThresh);

  //---------------------------------------------------------------------------
  // Process each of the shapes
  //---------------------------------------------------------------------------
  SLIC_INFO(fmt::format("{:=^80}", "Sampling InOut fields for shapes"));
  for(const auto& s : params.shapeSet.getShapes())
  {
    // Load the shape from file
    shaper.loadShape(s);
    slic::flushStreams();

    // Apply the specified geometric transforms
    shaper.applyTransforms(s);
    slic::flushStreams();

    // Generate a spatial index over the shape
    shaper.prepareShapeQuery(shapeDim, s);
    slic::flushStreams();

    // Query the mesh against this shape
    shaper.runShapeQuery(params.vfSampling, sampleOrder, outputOrder);
    slic::flushStreams();

    // Apply the replacement rules for this shape against the existing materials
    shaper.applyReplacementRules(s);
    slic::flushStreams();

    // Finalize data structures associated with this shape and spatial index
    shaper.finalizeShapeQuery();
    slic::flushStreams();
  }

  //---------------------------------------------------------------------------
  // After shaping in all shapes, generate/adjust the material volume fractions
  //---------------------------------------------------------------------------
  SLIC_INFO(
    fmt::format("{:=^80}", "Generating volume fraction fields for materials"));

  shaper.adjustVolumeFractions(params.vfSampling, outputOrder);

  //---------------------------------------------------------------------------
  // Save meshes and fields
  //---------------------------------------------------------------------------
#ifdef MFEM_USE_MPI
  shaper.getDC()->Save();
#endif

  //---------------------------------------------------------------------------
  // Cleanup and exit
  //---------------------------------------------------------------------------
  finalizeLogger();
#ifdef AXOM_USE_MPI
  MPI_Finalize();
#endif

  return 0;
}
