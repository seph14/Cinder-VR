//
//  DeviceManager.cpp
//  PSVR
//
//  Created by Seph Li on 2017/02/11.
//
//


#include "cinder/vr/psvr/DeviceManager.h"
#include "cinder/vr/psvr/Context.h"
#include "cinder/Log.h"

#if defined( CINDER_VR_ENABLE_PSVR )

namespace cinder { namespace vr { namespace psvr {
    
    const std::string kDeviceVendorName = "Sony Playstation VR";
    
    // -------------------------------------------------------------------------------------------------
    // DeviceManager
    // -------------------------------------------------------------------------------------------------
    DeviceManager::DeviceManager( ci::vr::Environment *env )
    : ci::vr::DeviceManager( ci::vr::API_PSVR, kDeviceVendorName, env ) {
        //we could optionally list all psvr devices here and throw the error out now
        /*if( PSVRApi::PSVRContext::listPSVR().size() < 1 ) {
            throw ci::vr::psvr::Exception( "PSVR is not present" );
        }*/
    }
    
    DeviceManager::~DeviceManager(){}
    
    void DeviceManager::initialize(){
        CI_LOG_I( "Initializing devices for Playstation VR" );
        
        mVrSystem = PSVRApi::PSVRContext::initPSVR();
        if( mVrSystem == NULL ){
            throw psvr::Exception( "PSVR libusb initialization failed, please check log" );
        }
        
        mVrSystem->infoReport.connect(std::bind(&DeviceManager::setInfo, this,
                                                std::placeholders::_1, std::placeholders::_1));
        mVrSystem->turnHeadSetOn();
        //mVrSystem->enableVR();
    }
    
    void DeviceManager::destroy(){
        CI_LOG_I( "Destroying and turnoff devices for Playstation VR" );
        mVrSystem->enableCinematicMode();
        mVrSystem->turnHeadSetOff();
        mVrSystem.reset();
    }
    
    uint32_t DeviceManager::numDevices() const {
        const uint32_t kMaxDevices = 1;
        return kMaxDevices;
    }
    
    void DeviceManager::setInfo(std::string firmware, std::string serial) {
        mDisplayName =  "PlaystationVR - Firmware: " + firmware + ", Serial: " + serial;
    }
    
    ci::vr::ContextRef DeviceManager::createContext( const ci::vr::SessionOptions& sessionOptions, uint32_t deviceIndex ) {
        if( deviceIndex >= numDevices() ) {
            throw ci::vr::psvr::Exception( "Device index out of range, deviceIndex=" + std::to_string( deviceIndex ) + ", maxIndex=" + std::to_string( numDevices() ) );
        }
        
        ci::vr::ContextRef result = ci::vr::psvr::Context::create( sessionOptions, this );
        return result;
    }
    
}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )
