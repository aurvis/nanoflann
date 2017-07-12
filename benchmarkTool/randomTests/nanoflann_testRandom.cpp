/***********************************************************************
 * Software License Agreement (BSD License)
 *
 * Copyright 2011-2016 Jose Luis Blanco (joseluisblancoc@gmail.com).
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *************************************************************************/

#include <nanoflann.hpp>
#include <ctime>
#include <cstdlib>
#include <iostream>

using namespace std;
using namespace nanoflann;

// This is an exampleof a custom data set class
template <typename T>
struct PointCloud
{
    struct Point
    {
        T  x,y,z;
    };

    std::vector<Point>  pts;

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return pts.size(); }

    // Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
    inline T kdtree_distance(const T *p1, const size_t idx_p2,size_t /*size*/) const
    {
        const T d0=p1[0]-pts[idx_p2].x;
        const T d1=p1[1]-pts[idx_p2].y;
        const T d2=p1[2]-pts[idx_p2].z;
        return d0*d0+d1*d1+d2*d2;
    }

    // Returns the dim'th component of the idx'th point in the class:
    // Since this is inlined and the "dim" argument is typically an immediate value, the
    //  "if/else's" are actually solved at compile time.
    inline T kdtree_get_pt(const size_t idx, int dim) const
    {
        if (dim==0) return pts[idx].x;
        else if (dim==1) return pts[idx].y;
        else return pts[idx].z;
    }

    // Optional bounding-box computation: return false to default to a standard bbox computation loop.
    //   Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
    //   Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

};

template <typename T>
PointCloud<T> generateRandomPointCloud(const size_t N, const T max_range = 10)
{
    PointCloud<T> point;
    point.pts.resize(N);
    for (size_t i=0;i<N;i++)
    {
        point.pts[i].x = max_range * (rand() % 1000) / T(1000);
        point.pts[i].y = max_range * (rand() % 1000) / T(1000);
        point.pts[i].z = max_range * (rand() % 1000) / T(1000);
    }
    return point;
}


template <typename num_t>
void kdtree_demo(const size_t N, double &buildTimer, double &queryTimer)
{
    PointCloud<num_t> cloudS = generateRandomPointCloud<num_t>(N);
    PointCloud<num_t> cloudT = generateRandomPointCloud<num_t>(N);

    clock_t begin = clock();
    // construct a kd-tree index:
    typedef KDTreeSingleIndexAdaptor<
        L2_Simple_Adaptor<num_t, PointCloud<num_t> > ,
        PointCloud<num_t>,
        3 /* dim */
        > my_kd_tree_t;
    my_kd_tree_t   index(3 /*dim*/, cloudS, KDTreeSingleIndexAdaptorParams(10 /* max leaf */) );
    index.buildIndex();
    clock_t end = clock();
    buildTimer += double(end - begin) / CLOCKS_PER_SEC;
    
    {
        double elapsed_secs=0;
        for(size_t i=0;i<N;i++)
        {
            num_t query_pt[3] = {cloudT.pts[i].x, cloudT.pts[i].y, cloudT.pts[i].z};
            // do a knn search
            const size_t num_results = 1;
            size_t ret_index;
            num_t out_dist_sqr;
            clock_t begin = clock();
            nanoflann::KNNResultSet<num_t> resultSet(num_results);
            resultSet.init(&ret_index, &out_dist_sqr);
            index.findNeighbors(resultSet, query_pt, nanoflann::SearchParams(10));
            clock_t end = clock();
            elapsed_secs += double(end - begin);
        }
        elapsed_secs = elapsed_secs/CLOCKS_PER_SEC;        
        queryTimer += elapsed_secs/N;
    }
}

int main()
{
    // Randomize Seed
    srand(time(NULL));
    size_t plotCount = 10;
    // Number of points
    size_t Ns[] = {1e3, 5e3, 1e4, 5e4, 1e5, 2e5, 5e5, 7e5, 1e6, 2e6};
    // And repetitions for each point cloud size:
    size_t nReps[] = {1, 1, 1, 1,  1,  1,  1,  1,  1, 1};
    // buildTime : time required to build the kd-tree index
    // queryTime : time required to find nearest neighbor for a single point in the kd-tree
    vector<double> buildTime, queryTime;

    for (size_t i=0;i<plotCount;i++)
    {
        double buildTimer = 0, queryTimer = 0;
        for (size_t repets=0;repets<nReps[i];repets++)
            kdtree_demo<float>(Ns[i], buildTimer, queryTimer);
        buildTimer /= nReps[i];
        queryTimer /= nReps[i];
        buildTime.push_back(buildTimer);
        queryTime.push_back(queryTimer);
    }
    for(size_t i=0;i<buildTime.size();i++)
        std::cout<<buildTime[i]<<" ";
    std::cout<<"\n";

    for(size_t i=0;i<queryTime.size();i++)
        std::cout<<queryTime[i]<<" ";
    std::cout<<"\n";
    return 0;
}
