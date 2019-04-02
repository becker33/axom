/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Copyright (c) 2017-2019, Lawrence Livermore National Security, LLC.
 *
 * Produced at the Lawrence Livermore National Laboratory
 *
 * LLNL-CODE-741217
 *
 * All rights reserved.
 *
 * This file is part of Axom.
 *
 * For details about use and distribution, please read axom/LICENSE.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include "axom/mint/mesh/FieldTypes.hpp"

#include "axom/core/Types.hpp"
#include "gtest/gtest.h" // for gtest macros

// namespace aliases
namespace mint = axom::mint;

//------------------------------------------------------------------------------
// UNIT TESTS
//------------------------------------------------------------------------------
TEST( mint_mesh_fieldtypes, field_traits )
{
  using int32 = axom::int32;
  using int64 = axom::int64;

  EXPECT_EQ( mint::field_traits< char >::type(), mint::UNDEFINED_FIELD_TYPE );
  EXPECT_EQ( mint::field_traits< float >::type(), mint::FLOAT_FIELD_TYPE );
  EXPECT_EQ( mint::field_traits< double >::type(), mint::DOUBLE_FIELD_TYPE );
  EXPECT_EQ( mint::field_traits< int >::type(), mint::INT32_FIELD_TYPE );
  EXPECT_EQ( mint::field_traits< int32 >::type(), mint::INT32_FIELD_TYPE );
  EXPECT_EQ( mint::field_traits< int64 >::type(), mint::INT64_FIELD_TYPE );
}

//------------------------------------------------------------------------------
#include "axom/slic/core/UnitTestLogger.hpp"
using axom::slic::UnitTestLogger;

int main(int argc, char* argv[])
{
  int result = 0;

  ::testing::InitGoogleTest(&argc, argv);

  UnitTestLogger logger;  // create & initialize test logger,

  // finalized when exiting main scope

  result = RUN_ALL_TESTS();

  return result;
}
