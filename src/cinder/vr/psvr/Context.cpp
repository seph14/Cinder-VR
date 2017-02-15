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


Copyright (c) 2016, The Cinder Project, All rights reserved.

This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and
the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/vr/psvr/Context.h"
#include "cinder/vr/psvr/DeviceManager.h"
#include "cinder/vr/psvr/Hmd.h"
#include "psvrapi.h"
#include "cinder/app/App.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"

#if defined( CINDER_VR_ENABLE_PSVR )

namespace cinder { namespace vr { namespace psvr {
    
    Context::Context( const ci::vr::SessionOptions& sessionOptions, ci::vr::psvr::DeviceManager* deviceManager )
    : ci::vr::Context( sessionOptions, deviceManager ) {
        mDeviceManager = deviceManager;
        mVrSystem = mDeviceManager->getVrSystem();
        mVrSystem->connect.connect          (std::bind(&Context::onConnect,      this, std::placeholders::_1  ));
        mVrSystem->rotationUpdate.connect   (std::bind(&Context::rotationUpdate, this, std::placeholders::_1, std::placeholders::_2));
        mVrSystem->unsolicitedReport.connect(std::bind(&Context::setUnsolicited, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }
    
    Context::~Context(){
        endSession();
    }
    
    void Context::scanForControllers(){
        CI_LOG_W( "Playstation VR doesn't have controllers supports now" );
    }
    
    ContextRef Context::create( const ci::vr::SessionOptions& sessionOptions, ci::vr::psvr::DeviceManager* deviceManager ){
        ContextRef result = ContextRef( new Context( sessionOptions, deviceManager ) );
        return result;
    }
    
    void Context::beginSession(){
        // Create HMD
        mHmd = ci::vr::psvr::Hmd::create( this );
        // Set frame rate for VR
        ci::gl::enableVerticalSync( getSessionOptions().getVerticalSync() );
        ci::app::setFrameRate( getSessionOptions().getFrameRate() );
    }
    
    void Context::endSession(){ }
    void Context::processEvents(){ }
    
    void Context::onConnect(bool isConnected){
        if(!isConnected){
            CI_LOG_E("PSVR is disconnected");
        }else {
            CI_LOG_I("PSVR is connected");
        }
    }
    
    void Context::setUnsolicited(byte reportId, byte result, std::string message){
        std::string content = "Unsolicited: report:" + toString((int)reportId) + ", result:" + toString((int)result) + ", message:" + message;
        CI_LOG_W(content);
    }
    
    void Context::rotationUpdate(ci::quat quat, ci::vec3 dir){
        mQuat = quat;
        mDir  = glm::normalize(dir);
    }

    
}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )
