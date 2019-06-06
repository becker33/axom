// Copyright (c) 2017-2019, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#ifndef AXOM_PRIMAL_BVH_BUILDER_IMPL_H_
#define AXOM_PRIMAL_BVH_BUILDER_IMPL_H_

#include "axom/primal/spatial_acceleration/ExecutionSpace.hpp"
#include "axom/primal/spatial_acceleration/linear_bvh/BVHData.hpp"
#include "axom/primal/spatial_acceleration/linear_bvh/RadixTree.hpp"
#include "axom/primal/spatial_acceleration/linear_bvh/vec.hpp"
#include "axom/primal/spatial_acceleration/linear_bvh/aabb.hpp"

#include "axom/core/utilities/Utilities.hpp" // for isNearlyEqual()
#include "axom/slic/interface/slic.hpp"      // for slic

// RAJA includes
#include "RAJA/RAJA.hpp"

namespace axom
{
namespace primal
{
namespace bvh
{

//expands 10-bit unsigned int into 30 bits
static inline AXOM_HOST_DEVICE
axom::int32 expand_bits32(axom::int32 x32)
{
  x32 = (x32 | (x32 << 16)) & 0x030000FF;
  x32 = (x32 | (x32 << 8)) & 0x0300F00F;
  x32 = (x32 | (x32 << 4)) & 0x030C30C3;
  x32 = (x32 | (x32 << 2)) & 0x09249249;
  return x32;
}

//------------------------------------------------------------------------------
static inline AXOM_HOST_DEVICE
axom::int64 expand_bits64(axom::int32 x)
{
  axom::int64 x64 = x & 0x1FFFFF;
  x64 = (x64 | x64 << 32) & 0x1F00000000FFFF;
  x64 = (x64 | x64 << 16) & 0x1F0000FF0000FF;
  x64 = (x64 | x64 << 8) & 0x100F00F00F00F00F;
  x64 = (x64 | x64 << 4) & 0x10c30c30c30c30c3;
  x64 = (x64 | x64 << 2) & 0x1249249249249249;

  return x64;
}

//------------------------------------------------------------------------------
//Returns 30 bit morton code for coordinates for
// x, y, and z are expecting to be between [0,1]
static inline AXOM_HOST_DEVICE
axom::int32 morton32_encode( axom::float32 x,
                             axom::float32 y,
                             axom::float32 z=0.0 )
{
  //take the first 10 bits. Note, 2^10 = 1024
  x = fmin(fmax(x * 1024.0f, 0.0f), 1023.0f);
  y = fmin(fmax(y * 1024.0f, 0.0f), 1023.0f);
  z = fmin(fmax(z * 1024.0f, 0.0f), 1023.0f);

  //expand 10 bits to 30
  axom::int32 xx = expand_bits32((axom::int32)x);
  axom::int32 yy = expand_bits32((axom::int32)y);
  axom::int32 zz = expand_bits32((axom::int32)z);
  //interleave coordinates
  return (zz << 2 | yy << 1 | xx);
}

//------------------------------------------------------------------------------
//Returns 30 bit morton code for coordinates for
//coordinates in the unit cude
static inline AXOM_HOST_DEVICE
axom::int64 morton64_encode( axom::float32 x,
                             axom::float32 y,
                             axom::float32 z=0.0 )
{
  //take the first 21 bits. Note, 2^21= 2097152.0f
  x = fmin(fmax(x * 2097152.0f, 0.0f), 2097151.0f);
  y = fmin(fmax(y * 2097152.0f, 0.0f), 2097151.0f);
  z = fmin(fmax(z * 2097152.0f, 0.0f), 2097151.0f);

  //expand the 10 bits to 30
  axom::int64 xx = expand_bits64((axom::int32)x);
  axom::int64 yy = expand_bits64((axom::int32)y);
  axom::int64 zz = expand_bits64((axom::int32)z);

  //interleave coordinates
  return (zz << 2 | yy << 1 | xx);
}

template < typename ExecSpace, typename FloatType >
void transform_boxes( const FloatType *boxes,
                      AABB<FloatType,3> *aabbs,
                      int32 size)
{
  using exec_policy = typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(
      RAJA::RangeSegment(0, size), AXOM_LAMBDA (int32 i)
  {
    AABB< FloatType, 3> aabb;
    Vec< FloatType,3 > min_point, max_point;

    const int32 offset = i * 6;
    min_point[0] = boxes[offset + 0];
    min_point[1] = boxes[offset + 1];
    min_point[2] = boxes[offset + 2];

    max_point[0] = boxes[offset + 3];
    max_point[1] = boxes[offset + 4];
    max_point[2] = boxes[offset + 5];

    aabb.include(min_point);
    aabb.include(max_point);
    aabbs[i] = aabb;
  } );

}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename FloatType >
void transform_boxes( const FloatType *boxes,
                      AABB<FloatType,2> *aabbs,
                      int32 size)
{
  using exec_policy = typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(
      RAJA::RangeSegment(0, size), AXOM_LAMBDA (int32 i)
  {
    AABB< FloatType, 2> aabb;
    Vec< FloatType,2 > min_point, max_point;

    const int32 offset = i * 4;
    min_point[0] = boxes[offset + 0];
    min_point[1] = boxes[offset + 1];

    max_point[0] = boxes[offset + 2];
    max_point[1] = boxes[offset + 3];

    aabb.include(min_point);
    aabb.include(max_point);
    aabbs[ i ] = aabb;
  } );

}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename FloatType >
AABB<FloatType,3> reduce(AABB<FloatType,3> *aabbs, int32 size)
{

  using reduce_policy =
      typename primal::execution_space< ExecSpace >::raja_reduce;
  RAJA::ReduceMin< reduce_policy, FloatType> xmin(infinity32());
  RAJA::ReduceMin< reduce_policy, FloatType> ymin(infinity32());
  RAJA::ReduceMin< reduce_policy, FloatType> zmin(infinity32());

  RAJA::ReduceMax< reduce_policy, FloatType> xmax(neg_infinity32());
  RAJA::ReduceMax< reduce_policy, FloatType> ymax(neg_infinity32());
  RAJA::ReduceMax< reduce_policy, FloatType> zmax(neg_infinity32());

  using exec_policy = typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(RAJA::RangeSegment(0,size), AXOM_LAMBDA(int32 i)
  {

    const AABB<FloatType, 3> &aabb = aabbs[i];

    xmin.min(aabb.m_x.min());
    ymin.min(aabb.m_y.min());
    zmin.min(aabb.m_z.min());

    xmax.max(aabb.m_x.max());
    ymax.max(aabb.m_y.max());
    zmax.max(aabb.m_z.max());

  } );

  AABB<FloatType,3> res;
  Vec< FloatType, 3 > mins =
      make_vec< FloatType >( xmin.get(), ymin.get(), zmin.get() );
  Vec< FloatType, 3 > maxs =
      make_vec< FloatType >( xmax.get(), ymax.get(), zmax.get() );

  res.include(mins);
  res.include(maxs);
  return res;
}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename FloatType >
AABB<FloatType,2> reduce(AABB<FloatType,2> *aabbs, int32 size)
{

  using reduce_policy =
      typename primal::execution_space< ExecSpace >::raja_reduce;
  RAJA::ReduceMin< reduce_policy, FloatType> xmin(infinity32());
  RAJA::ReduceMin< reduce_policy, FloatType> ymin(infinity32());

  RAJA::ReduceMax< reduce_policy, FloatType> xmax(neg_infinity32());
  RAJA::ReduceMax< reduce_policy, FloatType> ymax(neg_infinity32());


  using exec_policy =
      typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(
      RAJA::RangeSegment(0, size), AXOM_LAMBDA (int32 i)
  {

    const AABB<FloatType, 2> &aabb = aabbs[ i ];
    xmin.min(aabb.m_x.min());
    ymin.min(aabb.m_y.min());


    xmax.max(aabb.m_x.max());
    ymax.max(aabb.m_y.max());

  } );

  AABB<FloatType,2> res;
  Vec< FloatType, 2 > mins = make_vec< FloatType >(xmin.get(), ymin.get() );
  Vec< FloatType, 2 > maxs = make_vec< FloatType >(xmax.get(), ymax.get() );

  res.include(mins);
  res.include(maxs);
  return res;
}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename FloatType >
void get_mcodes( AABB<FloatType,2> *aabbs,
                    int32 size,
                    const AABB< FloatType,2 > &bounds,
                    uint32* mcodes )
{
  Vec<FloatType,2> extent, inv_extent, min_coord;
  extent[0] = bounds.m_x.max() - bounds.m_x.min();
  extent[1] = bounds.m_y.max() - bounds.m_y.min();

  min_coord[0] = bounds.m_x.min();
  min_coord[1] = bounds.m_y.min();

  for ( int i = 0; i < 2; ++i )
  {
    inv_extent[ i ] =
        utilities::isNearlyEqual< FloatType >( extent[i], .0f ) ?
            0.f : 1.f / extent[i];
  }

  using exec_policy =
      typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(RAJA::RangeSegment(0,size), AXOM_LAMBDA(int32 i)
  {
    const AABB<FloatType,2> &aabb = aabbs[i];

    // get the center and normalize it
    FloatType dx = aabb.m_x.center() - min_coord[ 0 ];
    FloatType dy = aabb.m_y.center() - min_coord[ 1 ];
    float32 centroid_x = static_cast< float32 >( dx * inv_extent[0] );
    float32 centroid_y = static_cast< float32 >( dy * inv_extent[1] );
    mcodes[ i ] = morton32_encode(centroid_x, centroid_y );
  } );

}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename FloatType >
void get_mcodes( AABB<FloatType,3> *aabbs,
                    int32 size,
                    const AABB< FloatType,3 > &bounds,
                    uint32* mcodes )
{
  Vec< FloatType, 3 > extent, inv_extent, min_coord;
  extent[0] = bounds.m_x.max() - bounds.m_x.min();
  extent[1] = bounds.m_y.max() - bounds.m_y.min();
  extent[2] = bounds.m_z.max() - bounds.m_z.min();

  min_coord[0] = bounds.m_x.min();
  min_coord[1] = bounds.m_y.min();
  min_coord[2] = bounds.m_z.min();

  for ( int i = 0; i < 3; ++i )
  {
    inv_extent[ i ] =
      utilities::isNearlyEqual< FloatType >( extent[i], .0f ) ?
          0.f : 1.f / extent[i];
  }

  using exec_policy =
      typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(RAJA::RangeSegment(0,size), AXOM_LAMBDA(int32 i)
  {
    const AABB<FloatType,3> &aabb = aabbs[i];

    // get the center and normalize it
    FloatType dx = aabb.m_x.center() - min_coord[0];
    FloatType dy = aabb.m_y.center() - min_coord[1];
    FloatType dz = aabb.m_z.center() - min_coord[2];
    float32 centroid_x = static_cast< float32 >( dx * inv_extent[0] );
    float32 centroid_y = static_cast< float32 >( dy * inv_extent[1] );
    float32 centroid_z = static_cast< float32 >( dz * inv_extent[2] );
    mcodes[ i ] = morton32_encode(centroid_x, centroid_y, centroid_z);
  } );

}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename IntType >
void array_counting( IntType* iterator,
                     const IntType& size,
                     const IntType& start,
                     const IntType& step)
{
  using exec_policy = typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(
      RAJA::RangeSegment(0, size), AXOM_LAMBDA(int32 i)
  {
    iterator[ i ] = start + i * step;
  } );
}

//------------------------------------------------------------------------------
//
// reorder and array based on a new set of indices.
// array   [a,b,c]
// indices [1,0,2]
// result  [b,a,c]
//
template< typename ExecSpace, typename T>
void reorder(int32 *indices, T *&array, int32 size)
{
  T* temp = axom::allocate< T >( size );

  using exec_policy = typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(
      RAJA::RangeSegment(0, size), AXOM_LAMBDA (int32 i)
  {
    int32 in_idx = indices[ i ];
    temp[ i ]    = array[ in_idx ];
  } );


  axom::deallocate(array);
  array = temp;
}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename MCType >
void sort_mcodes( MCType*& mcodes, int32 size, int32* iter )
{
  array_counting< ExecSpace >(iter, size, 0, 1);

  // TODO: create custom sort for GPU / CPU
  // WARNING: this will segfault with CUDA and not unified memory
  std::sort(iter,
            iter + size,
            [=](int32 i1, int32 i2)
            {
              return mcodes[i1] < mcodes[i2];
            } );


  reorder< ExecSpace >(iter, mcodes, size);
}

//------------------------------------------------------------------------------
template < typename IntType, typename MCType >
AXOM_HOST_DEVICE IntType delta( const IntType &a,
                                const IntType &b,
                                const IntType &inner_size,
                                const MCType *mcodes )
{
  bool tie = false;
  bool out_of_range = (b < 0 || b > inner_size);
  //still make the call but with a valid adderss
  const int32 bb = (out_of_range) ? 0 : b;
  const uint32 acode = mcodes[a];
  const uint32 bcode = mcodes[bb];
  //use xor to find where they differ
  uint32 exor = acode ^ bcode;
  tie = (exor == 0);
  //break the tie, a and b must always differ
  exor = tie ? uint32(a) ^ uint32(bb) : exor;
  int32 count = clz(exor);
  if (tie)
    count += 32;
  count = (out_of_range) ? - 1 : count;
  return count;
}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename FloatType, int NDIMS >
void build_tree(  RadixTree< FloatType, NDIMS > &data )
{
  // http://research.nvidia.com/sites/default/files/publications/karras2012hpg_paper.pdf

  // Pointers and vars are redeclared because I have a faint memory
  // of a huge amount of pain and suffering due so cuda
  // labda captures of pointers indide a struct. Bad memories
  // of random segfaults ........ be warned
  const int32 inner_size = data.m_inner_size;
  int32 *lchildren_ptr = data.m_left_children;
  int32 *rchildren_ptr = data.m_right_children;
  int32 *parent_ptr = data.m_parents;
  const uint32 *mcodes_ptr = data.m_mcodes;

  using exec_policy = typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(
      RAJA::RangeSegment(0, inner_size), AXOM_LAMBDA (int32 i)
  {
    //determine range direction
    int32 d = 0 > (delta(i, i + 1, inner_size, mcodes_ptr) - delta(i, i - 1, inner_size, mcodes_ptr)) ? -1 : 1;

    //find upper bound for the length of the range
    int32 min_delta = delta(i, i - d, inner_size, mcodes_ptr);
    int32 lmax = 2;
    while (delta(i, i + lmax * d, inner_size, mcodes_ptr) > min_delta)
          lmax *= 2;

    //binary search to find the lower bound
    int32 l = 0;
    for (int32 t = lmax / 2; t >= 1; t /= 2)
    {
      if (delta(i, i + (l + t) * d, inner_size, mcodes_ptr) > min_delta)
      {
        l += t;
      }
    }

    int32 j = i + l * d;
    int32 delta_node = delta(i, j, inner_size, mcodes_ptr);
    int32 s = 0;
    FloatType div_factor = 2.f;
    //find the split postition using a binary search
    for (int32 t = (int32)ceil(float32(l) / div_factor);;
         div_factor *= 2, t = (int32)ceil(float32(l) / div_factor))
    {
      if (delta(i, i + (s + t) * d, inner_size, mcodes_ptr) > delta_node)
      {
        s += t;
      }
      if (t == 1) break;
    }

    int32 split = i + s * d + min(d, 0);
    // assign parent/child pointers
    if (min(i, j) == split)
    {
      //leaf
      parent_ptr[split + inner_size] = i;
      lchildren_ptr[i] = split + inner_size;
    }
    else
    {
      //inner node
      parent_ptr[split] = i;
      lchildren_ptr[i] = split;
    }


    if (max(i, j) == split + 1)
    {
      //leaf
      parent_ptr[split + inner_size + 1] =  i;
      rchildren_ptr[i] =  split + inner_size + 1;
    }
    else
    {
      parent_ptr[split + 1] = i;
      rchildren_ptr[i] = split + 1;
    }

    if(i == 0)
    {
      // flag the root
      parent_ptr[0] = -1;
    }

  } );

}

//------------------------------------------------------------------------------
template< typename ExecSpace, typename T>
static void array_memset(T* array, const int32 size, const T val)
{
  using exec_policy = typename primal::execution_space< ExecSpace >::raja_exec;
  RAJA::forall< exec_policy >(
      RAJA::RangeSegment(0, size), AXOM_LAMBDA (int32 i)
  {
    array[ i ] = val;
  } );
}

//------------------------------------------------------------------------------
template < typename ExecSpace, typename FloatType, int NDIMS >
void propagate_aabbs( RadixTree< FloatType, NDIMS >& data)
{
  const int inner_size = data.m_inner_size;
  const int leaf_size = data.m_inner_size + 1;
  SLIC_ASSERT( leaf_size == data.m_size );

  // Pointers and vars are redeclared because I have a faint memory
  // of a huge amount of pain and suffering due so cuda
  // labda captures of pointers indide a struct. Bad memories
  // of random segfaults ........ be warned
  const int32 *lchildren_ptr = data.m_left_children;
  const int32 *rchildren_ptr = data.m_right_children;
  const int32 *parent_ptr = data.m_parents;
  const AABB< FloatType,NDIMS >  *leaf_aabb_ptr = data.m_leaf_aabbs;

  AABB<FloatType,NDIMS>  *inner_aabb_ptr = data.m_inner_aabbs;

  int32* counters_ptr = axom::allocate<int32>(inner_size);

  array_memset< ExecSpace >(counters_ptr, inner_size, 0);

  using exec_policy   =
      typename primal::execution_space< ExecSpace >::raja_exec;
  using atomic_policy =
      typename primal::execution_space< ExecSpace >::raja_atomic;

  RAJA::forall< exec_policy >(
      RAJA::RangeSegment(0,leaf_size), AXOM_LAMBDA(int32 i)
  {
    int32 current_node = parent_ptr[inner_size + i];

    while( current_node != -1)
    {
      int32 old= RAJA::atomic::atomicAdd< atomic_policy >(&(counters_ptr[current_node]),1);

      if(old == 0)
      {
        // first thread to get here kills itself
        return;
      }

      int32 lchild = lchildren_ptr[current_node];
      int32 rchild = rchildren_ptr[current_node];

      // gather the aabbs
      AABB< FloatType,NDIMS > aabb;
      if(lchild >= inner_size)
      {
        aabb.include(leaf_aabb_ptr[lchild - inner_size]);
      }
      else
      {
        aabb.include(inner_aabb_ptr[lchild]);
      }

      if(rchild >= inner_size)
      {
        aabb.include(leaf_aabb_ptr[rchild - inner_size]);
      }
      else
      {
        aabb.include(inner_aabb_ptr[rchild]);
      }

      inner_aabb_ptr[current_node] = aabb;

      current_node = parent_ptr[current_node];
    }

  } );

  axom::deallocate(counters_ptr);
}


//------------------------------------------------------------------------------
template < typename ExecSpace, typename FloatType, int NDIMS >
void build_radix_tree( const FloatType* boxes,
                       int size,
                       AABB< FloatType, NDIMS >& bounds,
                       RadixTree< FloatType, NDIMS >& radix_tree )
{
  radix_tree.allocate( size );

  // copy so we don't reorder the input
  transform_boxes< ExecSpace >(boxes, radix_tree.m_leaf_aabbs, size);

  // evaluate global bounds
  bounds = reduce< ExecSpace >(radix_tree.m_leaf_aabbs, size);

  // sort aabbs based on morton code
  // original positions of the sorted morton codes.
  // allows us to gather / sort other arrays.
  get_mcodes< ExecSpace >( radix_tree.m_leaf_aabbs, size, bounds,
                           radix_tree.m_mcodes );
  sort_mcodes< ExecSpace >( radix_tree.m_mcodes, size,
                            radix_tree.m_leafs );
  reorder< ExecSpace >( radix_tree.m_leafs, radix_tree.m_leaf_aabbs, size );

  build_tree< ExecSpace >( radix_tree );

  propagate_aabbs< ExecSpace >( radix_tree );
}

} /* nanmespace axom */
} /* namespace primal */
} /* namespace bvh */

#endif
