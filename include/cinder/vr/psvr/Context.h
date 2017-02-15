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
#pragma once

#include "cinder/vr/Context.h"

#if defined( CINDER_VR_ENABLE_PSVR )

#include "psvr.h"
#include "psvrapi.h"

namespace cinder { namespace vr { namespace psvr  {
    
    class DeviceManager;
    
    class Context;
    using ContextRef = std::shared_ptr<Context>;
    
    //! \class Context
    //!
    //!
    class Context : public ci::vr::Context {
    public:
        virtual ~Context();
        static ContextRef					create( const ci::vr::SessionOptions& sessionOptions, ci::vr::psvr::DeviceManager* deviceManager );
        
        ci::vr::psvr::DeviceManager         *getDeviceManager() const { return mDeviceManager; }
        ::PSVRApi::PSVRContextRef           getVrSystem() const { return mVrSystem; }
        
        virtual void						scanForControllers() override;

        const ci::quat                      getDeviceOrientation() const {return mQuat;}
        const ci::vec3                      getDeviceDirection()   const {return mDir;}

    protected:
        Context( const ci::vr::SessionOptions& sessionOptions, ci::vr::psvr::DeviceManager* deviceManager );
        friend class ci::vr::Environment;
        
        virtual void						beginSession() override;
        virtual void						endSession() override;
        
        virtual void						processEvents() override;
        
        void                                onConnect(bool isConnected);
        void                                setUnsolicited(byte reportId, byte result, std::string message);
        void                                rotationUpdate(ci::quat quat, ci::vec3 dir);

    private:
        ci::vr::psvr::DeviceManager		    *mDeviceManager = nullptr;
        ::PSVRApi::PSVRContextRef           mVrSystem;
        
        ci::quat                            mQuat;
        ci::vec3                            mDir;
    };
    
}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )
