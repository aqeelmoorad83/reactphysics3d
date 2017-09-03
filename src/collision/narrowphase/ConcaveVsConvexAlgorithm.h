/********************************************************************************
* ReactPhysics3D physics library, http://www.reactphysics3d.com                 *
* Copyright (c) 2010-2016 Daniel Chappuis                                       *
*********************************************************************************
*                                                                               *
* This software is provided 'as-is', without any express or implied warranty.   *
* In no event will the authors be held liable for any damages arising from the  *
* use of this software.                                                         *
*                                                                               *
* Permission is granted to anyone to use this software for any purpose,         *
* including commercial applications, and to alter it and redistribute it        *
* freely, subject to the following restrictions:                                *
*                                                                               *
* 1. The origin of this software must not be misrepresented; you must not claim *
*    that you wrote the original software. If you use this software in a        *
*    product, an acknowledgment in the product documentation would be           *
*    appreciated but is not required.                                           *
*                                                                               *
* 2. Altered source versions must be plainly marked as such, and must not be    *
*    misrepresented as being the original software.                             *
*                                                                               *
* 3. This notice may not be removed or altered from any source distribution.    *
*                                                                               *
********************************************************************************/
/*
#ifndef REACTPHYSICS3D_CONCAVE_VS_CONVEX_ALGORITHM_H
#define	REACTPHYSICS3D_CONCAVE_VS_CONVEX_ALGORITHM_H

// Libraries
#include "NarrowPhaseAlgorithm.h"
#include "collision/shapes/ConvexShape.h"
#include "collision/shapes/ConcaveShape.h"
#include "memory/SingleFrameAllocator.h"
#include <unordered_map>

/// Namespace ReactPhysics3D
namespace reactphysics3d {

// Class ConvexVsTriangleCallback
class MiddlePhaseTriangleCallback : public TriangleCallback {

    protected:

        /// Broadphase overlapping pair
        OverlappingPair* mOverlappingPair;

        /// Pointer to the concave proxy shape
        ProxyShape* mConcaveProxyShape;

        /// Pointer to the convex proxy shape
        ProxyShape* mConvexProxyShape;

        /// Pointer to the concave collision shape
        const ConcaveShape* mConcaveShape;

        /// Reference to the single-frame memory allocator
        Allocator& mAllocator;

    public:

        /// Pointer to the first element of the linked-list of narrow-phase info
        NarrowPhaseInfo* narrowPhaseInfoList;

        /// Constructor
        MiddlePhaseTriangleCallback(OverlappingPair* overlappingPair,
                                    ProxyShape* concaveProxyShape,
                                    ProxyShape* convexProxyShape, const ConcaveShape* concaveShape,
                                    Allocator& allocator)
            :mOverlappingPair(overlappingPair), mConcaveProxyShape(concaveProxyShape),
             mConvexProxyShape(convexProxyShape), mConcaveShape(concaveShape),
             mAllocator(allocator), narrowPhaseInfoList(nullptr) {

        }

        /// Test collision between a triangle and the convex mesh shape
        virtual void testTriangle(uint meshSubpart, uint triangleIndex, const Vector3* trianglePoints,
                                  const Vector3* verticesNormals) override;
};

// Class SmoothMeshContactInfo
struct SmoothMeshContactInfo {

    public:

        ContactManifoldInfo* contactManifoldInfo;
        ContactPointInfo* contactInfo;
        bool isFirstShapeTriangle;
        Vector3 triangleVertices[3];
        bool isUVWZero[3];

        /// Constructor
        SmoothMeshContactInfo(ContactManifoldInfo* manifoldInfo, ContactPointInfo* contactPointInfo,
                              bool firstShapeTriangle,
                              const Vector3& trianglePoint1, const Vector3& trianglePoint2,
                              const Vector3& trianglePoint3, bool isUZero, bool isVZero, bool isWZero)
            : contactManifoldInfo(manifoldInfo), contactInfo(contactPointInfo) {

            isFirstShapeTriangle = firstShapeTriangle;

            triangleVertices[0] = trianglePoint1;
            triangleVertices[1] = trianglePoint2;
            triangleVertices[2] = trianglePoint3;

            isUVWZero[0] = isUZero;
            isUVWZero[1] = isVZero;
            isUVWZero[2] = isWZero;
        }

};

struct ContactsDepthCompare {
    bool operator()(const SmoothMeshContactInfo& contact1, const SmoothMeshContactInfo& contact2)
    {
        return contact1.contactInfo->penetrationDepth < contact2.contactInfo->penetrationDepth;
    }
};

/// Method used to compare two smooth mesh contact info to sort them
//inline static bool contactsDepthCompare(const SmoothMeshContactInfo& contact1,
//                                        const SmoothMeshContactInfo& contact2) {
//    return contact1.contactInfo.penetrationDepth < contact2.contactInfo.penetrationDepth;
//}

// TODO : Delete this
// Class SmoothCollisionNarrowPhaseCallback
class SmoothCollisionNarrowPhaseCallback {

    private:

        std::vector<SmoothMeshContactInfo>& mContactPoints;


    public:

        // Constructor
        SmoothCollisionNarrowPhaseCallback(std::vector<SmoothMeshContactInfo>& contactPoints)
          : mContactPoints(contactPoints) {

        }

};

// TODO : Delete this
// Class ConcaveVsConvexAlgorithm
class ConcaveVsConvexAlgorithm {

    protected :

        // -------------------- Attributes -------------------- //        

        // -------------------- Methods -------------------- //

        /// Process the concave triangle mesh collision using the smooth mesh collision algorithm
        void processSmoothMeshCollision(OverlappingPair* overlappingPair,
                                        std::vector<SmoothMeshContactInfo> contactPoints,
                                        NarrowPhaseCallback* narrowPhaseCallback);

        /// Add a triangle vertex into the set of processed triangles
        void addProcessedVertex(std::unordered_multimap<int, Vector3>& processTriangleVertices,
                                const Vector3& vertex);

        /// Return true if the vertex is in the set of already processed vertices
        bool hasVertexBeenProcessed(const std::unordered_multimap<int, Vector3>& processTriangleVertices,
                                    const Vector3& vertex) const;

    public :

        // -------------------- Methods -------------------- //

        /// Constructor
        ConcaveVsConvexAlgorithm() = default;

        /// Destructor
        ~ConcaveVsConvexAlgorithm() = default;

        /// Private copy-constructor
        ConcaveVsConvexAlgorithm(const ConcaveVsConvexAlgorithm& algorithm) = delete;

        /// Private assignment operator
        ConcaveVsConvexAlgorithm& operator=(const ConcaveVsConvexAlgorithm& algorithm) = delete;

        /// Compute a contact info if the two bounding volume collide
        void testCollision(const NarrowPhaseInfo* narrowPhaseInfo,
                                   NarrowPhaseCallback* narrowPhaseCallback);
};

// Add a triangle vertex into the set of processed triangles
inline void ConcaveVsConvexAlgorithm::addProcessedVertex(std::unordered_multimap<int, Vector3>& processTriangleVertices, const Vector3& vertex) {
    processTriangleVertices.insert(std::make_pair(int(vertex.x * vertex.y * vertex.z), vertex));
}

}

#endif

*/