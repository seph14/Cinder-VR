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
