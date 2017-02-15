/*
Copyright (c) 2016-2017, Seph Li - All rights reserved.
This code is intended for use with the Cinder C++ library: http://libcinder.org
This file is part of Cinder-PSVR.
Cinder-PSVR is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
Cinder-PSVR is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with Cinder-PSVR.  If not, see <http://www.gnu.org/licenses/>.


This file is from PSVRFramework - https://github.com/gusmanb/PSVRFramework/blob/master/VRVideoPlayer/vrdevice.h

 * PSVRFramework - PlayStation VR PC framework
 * Copyright (C) 2016 Agustín Giménez Bernad <geniwab@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cinder/vr/psvr/vrdevice.h"
#include <math.h>

namespace VRDevice{
    float distort(const float coefficients[], float radius){
        float result = 1;
        float rFactor = 1;
        float rSquared = radius * radius;
        
        rFactor *= rSquared; //unnecessary, can be shorted, left to increase readability
        result += coefficients[0] * rFactor;
        rFactor *= rSquared;
        result += coefficients[1] * rFactor;
        
        return radius * result;
    }
    
    float distortInverse(const float coefficients[], float radius){
        float r0 = radius / 0.9f;
        float r1 = radius * 0.9f;
        float dr0 = radius - distort(coefficients, r0);
        
        //0.1mm
        while (glm::abs(r1 - r0) > 0.0001){
            float dr1 = radius - distort(coefficients, r1);
            float r2 = r1 - dr1 * ((r1 - r0) / (dr1 - dr0));
            r0 = r1;
            r1 = r2;
            dr0 = dr1;
        }
        
        return r1;
    }
    
    glm::vec4 projectionMatrixToVector(glm::mat4 matrix, float xScale, float yScale, float xTrans, float yTrans){
        glm::vec4 vec = glm::vec4(
                                  matrix[0][0] * xScale,
                                  matrix[1][1] * yScale,
                                  matrix[2][0] - 1 - xTrans,
                                  matrix[2][1] - 1 - yTrans);
        vec /= 2.0f;
        return vec;
    }
    
    vrparams getUndistortedParams(const vrscreenprops* screenProps, const vrlensprops* lensProps, const vrphysicalprops* physicalProps){
        vrparams params;
        
        float eyeToScreenDistance = lensProps->screenLensDistance;
        float halfLensDistance = lensProps->interLensDistance / 2 / eyeToScreenDistance;
        float screenWidth = physicalProps->widthMeters / eyeToScreenDistance;
        float screenHeight = physicalProps->heightMeters / eyeToScreenDistance;
        
        float eyePosX = screenWidth / 2 - halfLensDistance;
        float eyePosY = (lensProps->baselineDistance - physicalProps->bevelMeters) / eyeToScreenDistance;
        
        float maxFov = lensProps->fov;
        float viewerMax = distortInverse(lensProps->distortionCoeffs, tan(glm::radians(maxFov)));
        params.outerDist = glm::min(eyePosX, viewerMax);
        params.innerDist = glm::min(halfLensDistance, viewerMax);
        params.bottomDist = glm::min(eyePosY, viewerMax);
        params.topDist = glm::min(screenHeight - eyePosY, viewerMax);
        params.eyePosX = eyePosX;
        params.eyePosY = eyePosY;
        
        return params;
    }
    
    vrviewport getUndistortedViewportLeftEye(const vrscreenprops* screenProps, const vrlensprops* lensProps, const vrphysicalprops* physicalProps){
        vrviewport vp;
        vrparams p = getUndistortedParams(screenProps, lensProps, physicalProps);
        
        float eyeToScreenDistance = lensProps->screenLensDistance;
        float screenWidth = physicalProps->widthMeters / eyeToScreenDistance;
        float screenHeight = physicalProps->heightMeters / eyeToScreenDistance;
        float xPxPerTanAngle = screenProps->xRes / screenWidth;
        float yPxPerTanAngle = screenProps->yRes / screenHeight;
        
        float  x = round((p.eyePosX - p.outerDist) * xPxPerTanAngle);
        float  y = round((p.eyePosY - p.bottomDist) * yPxPerTanAngle);
        
        vp.x = x;
        vp.y = y;
        vp.width = round((p.eyePosX + p.innerDist) * xPxPerTanAngle) - x;
        vp.height = round((p.eyePosY + p.topDist) * yPxPerTanAngle) - y;
        return vp;
    }
    
    vrfov getUndistortedFieldOfViewLeftEye(const vrscreenprops* screenProps, const vrlensprops* lensProps, const vrphysicalprops* physicalProps){
        vrfov fov;
        vrparams p = getUndistortedParams(screenProps, lensProps, physicalProps);
        fov.leftDegrees = glm::degrees(atan(p.outerDist));
        fov.rightDegrees = glm::degrees(atan(p.innerDist));
        fov.downDegrees = glm::degrees(atan(p.bottomDist));
        fov.upDegrees = glm::degrees(atan(p.topDist));
        
        return fov;
    }
    
    vrfov getDistortedFieldOfViewLeftEye(const vrscreenprops* screenProps, const vrlensprops* lensProps, const vrphysicalprops* physicalProps){
        vrfov fov;
        
        float eyeToScreenDistance = lensProps->screenLensDistance;
        
        float outerDist = (physicalProps->widthMeters - lensProps->interLensDistance) / 2;
        float innerDist = lensProps->interLensDistance / 2;
        float bottomDist = lensProps->baselineDistance - physicalProps->bevelMeters;
        float topDist = physicalProps->heightMeters - bottomDist;
        
        float outerAngle = glm::degrees(atan(distort(lensProps->distortionCoeffs, outerDist / eyeToScreenDistance)));
        float innerAngle = glm::degrees(atan(distort(lensProps->distortionCoeffs, innerDist / eyeToScreenDistance)));
        float bottomAngle = glm::degrees(atan(distort(lensProps->distortionCoeffs, bottomDist / eyeToScreenDistance)));
        float topAngle = glm::degrees(atan(distort(lensProps->distortionCoeffs, topDist / eyeToScreenDistance)));
        
        fov.leftDegrees = glm::min(outerAngle, lensProps->fov);
        fov.rightDegrees = glm::min(innerAngle, lensProps->fov);
        fov.downDegrees = glm::min(bottomAngle, lensProps->fov);
        fov.upDegrees = glm::min(topAngle, lensProps->fov);
        
        return fov;
    }
    
    vrfov getFieldOfViewLeftEye(const vrscreenprops* screenProps, const vrlensprops* lensProps, const vrphysicalprops* physicalProps, bool undistorted){
        return undistorted ? getUndistortedFieldOfViewLeftEye(screenProps, lensProps, physicalProps) :
        getDistortedFieldOfViewLeftEye(screenProps, lensProps, physicalProps);
    }
    
    glm::mat4 getProjectionMatrixLeftEye(const vrscreenprops* screenProps, const vrlensprops* lensProps, const vrphysicalprops* physicalProps, bool undistorted){
        vrfov fov = getFieldOfViewLeftEye(screenProps, lensProps, physicalProps, undistorted);
        
        float top = tan(glm::radians(fov.upDegrees)) * NEAR_DISTANCE;
        float bottom = tan(glm::radians(fov.downDegrees)) * NEAR_DISTANCE;
        float left = tan(glm::radians(fov.leftDegrees)) * NEAR_DISTANCE;
        float right = tan(glm::radians(fov.rightDegrees)) * NEAR_DISTANCE;
        
        return glm::frustum(-left, right, -bottom, top, NEAR_DISTANCE, FAR_DISTANCE);
        
    }
    
    void initializeDevice(vrdevicedata* deviceData, const vrscreenprops* screenProps, const vrlensprops* lensProps){
        memcpy(&deviceData->screenProperties, screenProps, sizeof(vrscreenprops));
        memcpy(&deviceData->lensProperties, lensProps, sizeof(vrlensprops));
        
        vrphysicalprops* pprops = &deviceData->physicalProperties;
        pprops->widthMeters = (METERS_PER_INCH / screenProps->xDPI) * screenProps->xRes;
        pprops->heightMeters = (METERS_PER_INCH / screenProps->yDPI) * screenProps->yRes;
        pprops->bevelMeters = 0.004f; //Cardboard residual data, left to test it
        
        deviceData->distortedLeftFOV = getFieldOfViewLeftEye(screenProps, lensProps, pprops, false);
        deviceData->undistortedLeftFOV = getFieldOfViewLeftEye(screenProps, lensProps, pprops, true);
        
        deviceData->undistortedLeftViewport = getUndistortedViewportLeftEye(screenProps, lensProps, pprops);
        
        deviceData->distortedProjection = getProjectionMatrixLeftEye(screenProps, lensProps, pprops, false);
        deviceData->undistortedProjection = getProjectionMatrixLeftEye(screenProps, lensProps, pprops, true);
        
        deviceData->projectionLeft = projectionMatrixToVector(deviceData->distortedProjection, 1, 1, 0, 0);
        
        float xScale = deviceData->undistortedLeftViewport.width / (screenProps->xRes / 2);
        float yScale = deviceData->undistortedLeftViewport.height / screenProps->yRes;
        float xTrans = 2 * (deviceData->undistortedLeftViewport.x + deviceData->undistortedLeftViewport.width / 2) / (screenProps->xRes / 2) - 1;
        float yTrans = 2 * (deviceData->undistortedLeftViewport.y + deviceData->undistortedLeftViewport.height / 2) / screenProps->yRes - 1;
        
        deviceData->unprojectionLeft = projectionMatrixToVector(deviceData->undistortedProjection, xScale, yScale, xTrans, yTrans);
    }
}
