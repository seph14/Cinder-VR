//
//  Context.h
//  PSVR
//
//  Created by Seph Li on 2017/02/10.
//
//

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
        
        void                                setStatus(void *status);
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
