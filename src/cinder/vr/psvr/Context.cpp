//
//  Context.cpp
//  PSVR
//
//  Created by Seph Li on 2017/02/11.
//
//

#include "cinder/vr/psvr/Context.h"
#include "cinder/vr/psvr/DeviceManager.h"
#include "cinder/vr/psvr/Hmd.h"
#include "psvrapi.h"
#include "cinder/app/App.h"
#include "cinder/Log.h"

#if defined( CINDER_VR_ENABLE_PSVR )

namespace cinder { namespace vr { namespace psvr {
    
    Context::Context( const ci::vr::SessionOptions& sessionOptions, ci::vr::psvr::DeviceManager* deviceManager )
    : ci::vr::Context( sessionOptions, deviceManager )
    {
        mDeviceManager = deviceManager;
        mVrSystem = mDeviceManager->getVrSystem();
    }
    
    Context::~Context()
    {
        endSession();
    }
    
    void Context::scanForControllers()
    {
        CI_LOG_I( "Playstation VR doesn't have controllers supports now" );
    }
    
    ContextRef Context::create( const ci::vr::SessionOptions& sessionOptions, ci::vr::psvr::DeviceManager* deviceManager  )
    {
        ContextRef result = ContextRef( new Context( sessionOptions, deviceManager ) );
        return result;
    }
    
    void Context::beginSession()
    {
        // Create HMD
        mHmd = ci::vr::psvr::Hmd::create( this );
        // Set frame rate for VR
        ci::gl::enableVerticalSync( getSessionOptions().getVerticalSync() );
        ci::app::setFrameRate( getSessionOptions().getFrameRate() );
    }
    
    void Context::endSession()
    {
    }
    
    void Context::processEvents()
    {
        
    }
    
}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )
