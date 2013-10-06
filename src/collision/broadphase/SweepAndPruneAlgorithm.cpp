/********************************************************************************
* ReactPhysics3D physics library, http://code.google.com/p/reactphysics3d/      *
* Copyright (c) 2010-2013 Daniel Chappuis                                       *
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

// Libraries
#include "SweepAndPruneAlgorithm.h"
#include "../CollisionDetection.h"
#include "PairManager.h"
#include <climits>
#include <cstring>

// Namespaces
using namespace reactphysics3d;
using namespace std;

// Initialization of static variables
const bodyindex SweepAndPruneAlgorithm::INVALID_INDEX = std::numeric_limits<bodyindex>::max();
const luint SweepAndPruneAlgorithm::NB_SENTINELS = 2;

// Constructor that takes an AABB as input
AABBInt::AABBInt(const AABB& aabb) {
    min[0] = encodeFloatIntoInteger(aabb.getMin().x);
    min[1] = encodeFloatIntoInteger(aabb.getMin().y);
    min[2] = encodeFloatIntoInteger(aabb.getMin().z);

    max[0] = encodeFloatIntoInteger(aabb.getMax().x);
    max[1] = encodeFloatIntoInteger(aabb.getMax().y);
    max[2] = encodeFloatIntoInteger(aabb.getMax().z);
}

// Constructor that set all the axis with an minimum and maximum value
AABBInt::AABBInt(uint minValue, uint maxValue) {
    min[0] = minValue;
    min[1] = minValue;
    min[2] = minValue;

    max[0] = maxValue;
    max[1] = maxValue;
    max[2] = maxValue;
}

// Constructor
SweepAndPruneAlgorithm::SweepAndPruneAlgorithm(CollisionDetection& collisionDetection)
             :BroadPhaseAlgorithm(collisionDetection) {
    mBoxes = NULL;
    mEndPoints[0] = NULL;
    mEndPoints[1] = NULL;
    mEndPoints[2] = NULL;
    mNbBoxes = 0;
    mNbMaxBoxes = 0;
}

// Destructor
SweepAndPruneAlgorithm::~SweepAndPruneAlgorithm() {
    delete[] mBoxes;
    delete[] mEndPoints[0];
    delete[] mEndPoints[1];
    delete[] mEndPoints[2];
}

// Notify the broad-phase about a new object in the world
/// This method adds the AABB of the object ion to broad-phase
void SweepAndPruneAlgorithm::addObject(CollisionBody* body, const AABB& aabb) {

    bodyindex boxIndex;

    assert(encodeFloatIntoInteger(aabb.getMin().x) > encodeFloatIntoInteger(-FLT_MAX));
    assert(encodeFloatIntoInteger(aabb.getMin().y) > encodeFloatIntoInteger(-FLT_MAX));
    assert(encodeFloatIntoInteger(aabb.getMin().z) > encodeFloatIntoInteger(-FLT_MAX));
    assert(encodeFloatIntoInteger(aabb.getMax().x) < encodeFloatIntoInteger(FLT_MAX));
    assert(encodeFloatIntoInteger(aabb.getMax().y) < encodeFloatIntoInteger(FLT_MAX));
    assert(encodeFloatIntoInteger(aabb.getMax().z) < encodeFloatIntoInteger(FLT_MAX));

    // If the index of the first free box is valid (means that
    // there is a bucket in the middle of the array that doesn't
    // contain a box anymore because it has been removed)
    if (!mFreeBoxIndices.empty()) {
        boxIndex = mFreeBoxIndices.back();
        mFreeBoxIndices.pop_back();
    }
    else {
        // If the array boxes and end-points arrays are full
        if (mNbBoxes == mNbMaxBoxes) {
            // Resize the arrays to make them larger
            resizeArrays();
        }
        
        boxIndex = mNbBoxes;
    }
    
    // Move the maximum limit end-point two elements further
    // at the end-points array in all three axis
    const luint indexLimitEndPoint = 2 * mNbBoxes + NB_SENTINELS - 1;
    for (uint axis=0; axis<3; axis++) {
        EndPoint* maxLimitEndPoint = &mEndPoints[axis][indexLimitEndPoint];
        assert(mEndPoints[axis][0].boxID == INVALID_INDEX && mEndPoints[axis][0].isMin);
        assert(maxLimitEndPoint->boxID == INVALID_INDEX && !maxLimitEndPoint->isMin);
        EndPoint* newMaxLimitEndPoint = &mEndPoints[axis][indexLimitEndPoint + 2];
        newMaxLimitEndPoint->setValues(maxLimitEndPoint->boxID, maxLimitEndPoint->isMin,
                                       maxLimitEndPoint->value);
    }
    
    // Create a new box
    BoxAABB* box = &mBoxes[boxIndex];
    box->body = body;
    const uint maxEndPointValue = encodeFloatIntoInteger(FLT_MAX) - 1;
    const uint minEndPointValue = encodeFloatIntoInteger(FLT_MAX) - 2;
    for (uint axis=0; axis<3; axis++) {
        box->min[axis] = indexLimitEndPoint;
        box->max[axis] = indexLimitEndPoint + 1;
        EndPoint* minimumEndPoint = &mEndPoints[axis][box->min[axis]];
        minimumEndPoint->setValues(boxIndex, true, minEndPointValue);
        EndPoint* maximumEndPoint = &mEndPoints[axis][box->max[axis]];
        maximumEndPoint->setValues(boxIndex, false, maxEndPointValue);
    }
    
    // Add the body pointer to box index mapping
    mMapBodyToBoxIndex.insert(pair<CollisionBody*, bodyindex>(body, boxIndex));
    
    mNbBoxes++;

    // Call the update method to put the end-points of the new AABB at the
    // correct position in the array. This will also create the overlapping
    // pairs in the pair manager if the new AABB is overlapping with others
    // AABBs
    updateObject(body, aabb);
} 

// Notify the broad-phase about an object that has been removed from the world
void SweepAndPruneAlgorithm::removeObject(CollisionBody* body) {

    assert(mNbBoxes > 0);

    // Call the update method with an AABB that is very far away
    // in order to remove all overlapping pairs from the pair manager
    const uint maxEndPointValue = encodeFloatIntoInteger(FLT_MAX) - 1;
    const uint minEndPointValue = encodeFloatIntoInteger(FLT_MAX) - 2;
    const AABBInt aabbInt(minEndPointValue, maxEndPointValue);
    updateObjectIntegerAABB(body, aabbInt);

    // Get the corresponding box
    bodyindex boxIndex = mMapBodyToBoxIndex.find(body)->second;

    // Remove the end-points of the box by moving the maximum end-points two elements back in
    // the end-points array
    const luint indexLimitEndPoint = 2 * mNbBoxes + NB_SENTINELS - 1;
    for (uint axis=0; axis < 3; axis++) {
        EndPoint* maxLimitEndPoint = &mEndPoints[axis][indexLimitEndPoint];
        assert(mEndPoints[axis][0].boxID == INVALID_INDEX && mEndPoints[axis][0].isMin);
        assert(maxLimitEndPoint->boxID == INVALID_INDEX && !maxLimitEndPoint->isMin);
        EndPoint* newMaxLimitEndPoint = &mEndPoints[axis][indexLimitEndPoint - 2];
        assert(mEndPoints[axis][indexLimitEndPoint - 1].boxID == boxIndex);
        assert(!mEndPoints[axis][indexLimitEndPoint - 1].isMin);
        assert(newMaxLimitEndPoint->boxID == boxIndex);
        assert(newMaxLimitEndPoint->isMin);
        newMaxLimitEndPoint->setValues(maxLimitEndPoint->boxID, maxLimitEndPoint->isMin,
                                       maxLimitEndPoint->value);
    }

    // Add the box index into the list of free indices
    mFreeBoxIndices.push_back(boxIndex);

    mMapBodyToBoxIndex.erase(body);
    mNbBoxes--;

    // Check if we need to shrink the allocated memory
    const luint nextPowerOf2 = PairManager::computeNextPowerOfTwo((mNbBoxes-1) / 100 );
    if (nextPowerOf2 * 100 < mNbMaxBoxes) {
        shrinkArrays();
    }
} 
        
// Notify the broad-phase that the AABB of an object has changed.
/// The input is an AABB with integer coordinates
void SweepAndPruneAlgorithm::updateObjectIntegerAABB(CollisionBody* body, const AABBInt& aabbInt) {

    assert(aabbInt.min[0] > encodeFloatIntoInteger(-FLT_MAX));
    assert(aabbInt.min[1] > encodeFloatIntoInteger(-FLT_MAX));
    assert(aabbInt.min[2] > encodeFloatIntoInteger(-FLT_MAX));
    assert(aabbInt.max[0] < encodeFloatIntoInteger(FLT_MAX));
    assert(aabbInt.max[1] < encodeFloatIntoInteger(FLT_MAX));
    assert(aabbInt.max[2] < encodeFloatIntoInteger(FLT_MAX));
    
    // Get the corresponding box
    bodyindex boxIndex = mMapBodyToBoxIndex.find(body)->second;
    BoxAABB* box = &mBoxes[boxIndex];
        
    // Current axis
    for (uint axis=0; axis<3; axis++) {
        
        // Get the two others axis
        const uint otherAxis1 = (1 << axis) & 3;
        const uint otherAxis2 = (1 << otherAxis1) & 3;
        
        // Get the starting end-point of the current axis
        EndPoint* startEndPointsCurrentAxis = mEndPoints[axis];
        
        // -------- Update the minimum end-point ------------//
        
        EndPoint* currentMinEndPoint = &startEndPointsCurrentAxis[box->min[axis]];
        assert(currentMinEndPoint->isMin);
        
        // Get the minimum value of the AABB on the current axis
        uint limit = aabbInt.min[axis];
        
        // If the minimum value of the AABB is smaller
        // than the current minimum endpoint
        if (limit < currentMinEndPoint->value) {
            
            currentMinEndPoint->value = limit;
            
            // The minimum end-point is moving left
            EndPoint savedEndPoint = *currentMinEndPoint;
            luint indexEndPoint = (size_t(currentMinEndPoint) -
                                   size_t(startEndPointsCurrentAxis)) / sizeof(EndPoint);
            const luint savedEndPointIndex = indexEndPoint;
            
            while ((--currentMinEndPoint)->value > limit) {
                BoxAABB* id1 = &mBoxes[currentMinEndPoint->boxID];
                const bool isMin = currentMinEndPoint->isMin;
                
                // If it's a maximum end-point
                if (!isMin) {
                    // The minimum end-point is moving to the left and
                    // passed a maximum end-point. Thus, the boxes start
                    // overlapping on the current axis. Therefore we test
                    // for box intersection
                    if (box != id1) {
                        if (testIntersect2D(*box, *id1, otherAxis1, otherAxis2) &&
                            testIntersect1DSortedAABBs(*id1, aabbInt,
                                                       startEndPointsCurrentAxis, axis)) {
                            
                            // Add an overlapping pair to the pair manager
                            mPairManager.addPair(body, id1->body);
                        }
                    }
                    
                    id1->max[axis] = indexEndPoint--;
                }
                else {
                    id1->min[axis] = indexEndPoint--;
                }
                
                *(currentMinEndPoint+1) = *currentMinEndPoint;
            }
            
            // Update the current minimum endpoint that we are moving
            if (savedEndPointIndex != indexEndPoint) {
                if (savedEndPoint.isMin) {
                    mBoxes[savedEndPoint.boxID].min[axis] = indexEndPoint;
                }
                else {
                    mBoxes[savedEndPoint.boxID].max[axis] = indexEndPoint;
                }

                startEndPointsCurrentAxis[indexEndPoint] = savedEndPoint;
            }
        }
        else if (limit > currentMinEndPoint->value){// The minimum of the box has moved to the right
            
            currentMinEndPoint->value = limit;
            
            // The minimum en-point is moving right
            EndPoint savedEndPoint = *currentMinEndPoint;
            luint indexEndPoint = (size_t(currentMinEndPoint) -
                                   size_t(startEndPointsCurrentAxis)) / sizeof(EndPoint);
            const luint savedEndPointIndex = indexEndPoint;
            
            // For each end-point between the current position of the minimum
            // end-point and the new position of the minimum end-point
            while ((++currentMinEndPoint)->value < limit) {
                BoxAABB* id1 = &mBoxes[currentMinEndPoint->boxID];
                const bool isMin = currentMinEndPoint->isMin;
                
                // If it's a maximum end-point
                if (!isMin) {
                    // The minimum end-point is moving to the right and
                    // passed a maximum end-point. Thus, the boxes stop
                    // overlapping on the current axis.
                    if (box != id1) {
                        if (testIntersect2D(*box, *id1, otherAxis1, otherAxis2)) {
                            
                            // Remove the pair from the pair manager
                            mPairManager.removePair(body->getID(), id1->body->getID());
                        }
                    }
                    
                    id1->max[axis] = indexEndPoint++;
                }
                else {
                    id1->min[axis] = indexEndPoint++;
                }
                
                *(currentMinEndPoint-1) = *currentMinEndPoint;
            }
            
            // Update the current minimum endpoint that we are moving
            if (savedEndPointIndex != indexEndPoint) {
                if (savedEndPoint.isMin) {
                    mBoxes[savedEndPoint.boxID].min[axis] = indexEndPoint;
                }
                else {
                    mBoxes[savedEndPoint.boxID].max[axis] = indexEndPoint;
                }

                startEndPointsCurrentAxis[indexEndPoint] = savedEndPoint;
            }
        }
        
        // ------- Update the maximum end-point ------------ //
        
        EndPoint* currentMaxEndPoint = &startEndPointsCurrentAxis[box->max[axis]];
        assert(!currentMaxEndPoint->isMin);
        
        // Get the maximum value of the AABB on the current axis
        limit = aabbInt.max[axis];
        
        // If the new maximum value of the AABB is larger
        // than the current maximum end-point value. It means
        // that the AABB is moving to the right.
        if (limit > currentMaxEndPoint->value) {
            
            currentMaxEndPoint->value = limit;
            
            EndPoint savedEndPoint = *currentMaxEndPoint;
            luint indexEndPoint = (size_t(currentMaxEndPoint) -
                                   size_t(startEndPointsCurrentAxis)) / sizeof(EndPoint);
            const luint savedEndPointIndex = indexEndPoint;
            
            while ((++currentMaxEndPoint)->value < limit) {
                
                // Get the next end-point
                BoxAABB* id1 = &mBoxes[currentMaxEndPoint->boxID];
                const bool isMin = currentMaxEndPoint->isMin;
                
                // If it's a maximum end-point
                if (isMin) {
                    // The maximum end-point is moving to the right and
                    // passed a minimum end-point. Thus, the boxes start
                    // overlapping on the current axis. Therefore we test
                    // for box intersection
                    if (box != id1) {
                        if (testIntersect2D(*box, *id1, otherAxis1, otherAxis2) &&
                            testIntersect1DSortedAABBs(*id1, aabbInt,
                                                       startEndPointsCurrentAxis,axis)) {
                            
                            // Add an overlapping pair to the pair manager
                            mPairManager.addPair(body, id1->body);
                        }
                    }
                    
                    id1->min[axis] = indexEndPoint++;
                }
                else {
                    id1->max[axis] = indexEndPoint++;
                }
                
                *(currentMaxEndPoint-1) = *currentMaxEndPoint;
            }
            
            // Update the current minimum endpoint that we are moving
            if (savedEndPointIndex != indexEndPoint) {
                if (savedEndPoint.isMin) {
                    mBoxes[savedEndPoint.boxID].min[axis] = indexEndPoint;
                }
                else {
                    mBoxes[savedEndPoint.boxID].max[axis] = indexEndPoint;
                }

                startEndPointsCurrentAxis[indexEndPoint] = savedEndPoint;
            }
        }
        else if (limit < currentMaxEndPoint->value) {   // If the AABB is moving to the left 
            currentMaxEndPoint->value = limit;
            
            EndPoint savedEndPoint = *currentMaxEndPoint;
            luint indexEndPoint = (size_t(currentMaxEndPoint) -
                                   size_t(startEndPointsCurrentAxis)) / sizeof(EndPoint);
            const luint savedEndPointIndex = indexEndPoint;
            
            // For each end-point between the current position of the maximum
            // end-point and the new position of the maximum end-point
            while ((--currentMaxEndPoint)->value > limit) {
                BoxAABB* id1 = &mBoxes[currentMaxEndPoint->boxID];
                const bool isMin = currentMaxEndPoint->isMin;
                
                // If it's a minimum end-point
                if (isMin) {
                    // The maximum end-point is moving to the right and
                    // passed a minimum end-point. Thus, the boxes stop
                    // overlapping on the current axis.
                    if (box != id1) {
                        if (testIntersect2D(*box, *id1, otherAxis1, otherAxis2)) {
                            
                            // Remove the pair from the pair manager
                            mPairManager.removePair(body->getID(), id1->body->getID());
                        }
                    }
                    
                    id1->min[axis] = indexEndPoint--;
                }
                else {
                    id1->max[axis] = indexEndPoint--;
                }
                
                *(currentMaxEndPoint+1) = *currentMaxEndPoint;
            }
            
            // Update the current minimum endpoint that we are moving
            if (savedEndPointIndex != indexEndPoint) {
                if (savedEndPoint.isMin) {
                    mBoxes[savedEndPoint.boxID].min[axis] = indexEndPoint;
                }
                else {
                    mBoxes[savedEndPoint.boxID].max[axis] = indexEndPoint;
                }

                startEndPointsCurrentAxis[indexEndPoint] = savedEndPoint;
            }
        }
    }
}

// Resize the boxes and end-points arrays when it is full
void SweepAndPruneAlgorithm::resizeArrays() {
    
    // New number of boxes in the array
    const luint newNbMaxBoxes = mNbMaxBoxes ? 2 * mNbMaxBoxes : 100;
    const luint nbEndPoints = mNbBoxes * 2 + NB_SENTINELS;
    const luint newNbEndPoints = newNbMaxBoxes * 2 + NB_SENTINELS;
    
    // Allocate memory for the new boxes and end-points arrays
    BoxAABB* newBoxesArray = new BoxAABB[newNbMaxBoxes];
    EndPoint* newEndPointsXArray = new EndPoint[newNbEndPoints];
    EndPoint* newEndPointsYArray = new EndPoint[newNbEndPoints];
    EndPoint* newEndPointsZArray = new EndPoint[newNbEndPoints];
    
    assert(newBoxesArray != NULL);
    assert(newEndPointsXArray != NULL);
    assert(newEndPointsYArray != NULL);
    assert(newEndPointsZArray != NULL);
    
    // If the arrays were not empty before
    if (mNbBoxes > 0) {
        
        // Copy the data from the old arrays into the new one
        memcpy(newBoxesArray, mBoxes, sizeof(BoxAABB) * mNbBoxes);
        const size_t nbBytesNewEndPoints = sizeof(EndPoint) * nbEndPoints;
        memcpy(newEndPointsXArray, mEndPoints[0], nbBytesNewEndPoints);
        memcpy(newEndPointsYArray, mEndPoints[1], nbBytesNewEndPoints);
        memcpy(newEndPointsZArray, mEndPoints[2], nbBytesNewEndPoints);
    }
    else {   // If the arrays were empty

        // Add the limits endpoints (sentinels) into the array
        const uint min = encodeFloatIntoInteger(-FLT_MAX);
        const uint max = encodeFloatIntoInteger(FLT_MAX);
        newEndPointsXArray[0].setValues(INVALID_INDEX, true, min);
        newEndPointsXArray[1].setValues(INVALID_INDEX, false, max);
        newEndPointsYArray[0].setValues(INVALID_INDEX, true, min);
        newEndPointsYArray[1].setValues(INVALID_INDEX, false, max);
        newEndPointsZArray[0].setValues(INVALID_INDEX, true, min);
        newEndPointsZArray[1].setValues(INVALID_INDEX, false, max);
    }

    // Delete the old arrays
    delete[] mBoxes;
    delete[] mEndPoints[0];
    delete[] mEndPoints[1];
    delete[] mEndPoints[2];
    
    // Assign the pointer to the new arrays
    mBoxes = newBoxesArray;
    mEndPoints[0] = newEndPointsXArray;
    mEndPoints[1] = newEndPointsYArray;
    mEndPoints[2] = newEndPointsZArray;
    
    mNbMaxBoxes = newNbMaxBoxes;
}

// Shrink the boxes and end-points arrays when too much memory is allocated
void SweepAndPruneAlgorithm::shrinkArrays() {

    // New number of boxes and end-points in the array
    const luint nextPowerOf2 = PairManager::computeNextPowerOfTwo((mNbBoxes-1) / 100 );
    const luint newNbMaxBoxes = (mNbBoxes > 100) ? nextPowerOf2 * 100 : 100;
    const luint nbEndPoints = mNbBoxes * 2 + NB_SENTINELS;
    const luint newNbEndPoints = newNbMaxBoxes * 2 + NB_SENTINELS;

    assert(newNbMaxBoxes < mNbMaxBoxes);

    // Sort the list of the free boxes indices in ascending order
    mFreeBoxIndices.sort();

    // Reorganize the boxes inside the boxes array so that all the boxes are at the
    // beginning of the array
    std::map<CollisionBody*, bodyindex> newMapBodyToBoxIndex;
    std::map<CollisionBody*,bodyindex>::const_iterator it;
    for (it = mMapBodyToBoxIndex.begin(); it != mMapBodyToBoxIndex.end(); ++it) {

        CollisionBody* body = it->first;
        bodyindex boxIndex = it->second;

        // If the box index is outside the range of the current number of boxes
        if (boxIndex >= mNbBoxes) {

            assert(!mFreeBoxIndices.empty());

            // Get a new box index for that body (from the list of free box indices)
            bodyindex newBoxIndex = mFreeBoxIndices.front();
            mFreeBoxIndices.pop_front();
            assert(newBoxIndex < mNbBoxes);

            // Copy the box to its new location in the boxes array
            BoxAABB* oldBox = &mBoxes[boxIndex];
            BoxAABB* newBox = &mBoxes[newBoxIndex];
            assert(oldBox->body->getID() == body->getID());
            newBox->body = oldBox->body;
            for (uint axis=0; axis<3; axis++) {

                // Copy the minimum and maximum end-points indices
                newBox->min[axis] = oldBox->min[axis];
                newBox->max[axis] = oldBox->max[axis];

                // Update the box index of the end-points
                EndPoint* minimumEndPoint = &mEndPoints[axis][newBox->min[axis]];
                EndPoint* maximumEndPoint = &mEndPoints[axis][newBox->max[axis]];
                assert(minimumEndPoint->boxID == boxIndex);
                assert(maximumEndPoint->boxID == boxIndex);
                minimumEndPoint->boxID = newBoxIndex;
                maximumEndPoint->boxID = newBoxIndex;
            }

            newMapBodyToBoxIndex.insert(pair<CollisionBody*, bodyindex>(body, newBoxIndex));
        }
        else {
            newMapBodyToBoxIndex.insert(pair<CollisionBody*, bodyindex>(body, boxIndex));
        }
    }

    assert(newMapBodyToBoxIndex.size() == mMapBodyToBoxIndex.size());
    mMapBodyToBoxIndex = newMapBodyToBoxIndex;

    // Allocate memory for the new boxes and end-points arrays
    BoxAABB* newBoxesArray = new BoxAABB[newNbMaxBoxes];
    EndPoint* newEndPointsXArray = new EndPoint[newNbEndPoints];
    EndPoint* newEndPointsYArray = new EndPoint[newNbEndPoints];
    EndPoint* newEndPointsZArray = new EndPoint[newNbEndPoints];

    assert(newBoxesArray != NULL);
    assert(newEndPointsXArray != NULL);
    assert(newEndPointsYArray != NULL);
    assert(newEndPointsZArray != NULL);

    // Copy the data from the old arrays into the new one
    memcpy(newBoxesArray, mBoxes, sizeof(BoxAABB) * mNbBoxes);
    const size_t nbBytesNewEndPoints = sizeof(EndPoint) * nbEndPoints;
    memcpy(newEndPointsXArray, mEndPoints[0], nbBytesNewEndPoints);
    memcpy(newEndPointsYArray, mEndPoints[1], nbBytesNewEndPoints);
    memcpy(newEndPointsZArray, mEndPoints[2], nbBytesNewEndPoints);

    // Delete the old arrays
    delete[] mBoxes;
    delete[] mEndPoints[0];
    delete[] mEndPoints[1];
    delete[] mEndPoints[2];

    // Assign the pointer to the new arrays
    mBoxes = newBoxesArray;
    mEndPoints[0] = newEndPointsXArray;
    mEndPoints[1] = newEndPointsYArray;
    mEndPoints[2] = newEndPointsZArray;

    mNbMaxBoxes = newNbMaxBoxes;
}
