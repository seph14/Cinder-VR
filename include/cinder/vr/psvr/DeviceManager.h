//
//  DeviceManager.h
//  PSVR
//
//  Created by Seph Li on 2017/02/10.
//
//

#pragma once

#include "cinder/vr/DeviceManager.h"

#if defined( CINDER_VR_ENABLE_PSVR )

#include "psvrapi.h"

#include <map>

namespace cinder { namespace vr { namespace psvr  {
        
    //! \class Exception
    //!
    //!
    class Exception : public ci::vr::Exception {
    public:
        Exception() {}
        Exception( const std::string& msg ) : ci::vr::Exception( msg ) {}
        virtual ~Exception() {}
    };

    
    class DeviceManager;
    using DeviceManagerRef = std::shared_ptr<DeviceManager>;
    
    //! \class DeviceManager
    //!
    //!
    class DeviceManager : public ci::vr::DeviceManager {
    public:
        DeviceManager( ci::vr::Environment *env );
        virtual ~DeviceManager();
        
        ::PSVRApi::PSVRContextRef           getVrSystem() const { return mVrSystem; }
        
        virtual void						initialize();
        virtual void						destroy();
        virtual uint32_t					numDevices() const;
        virtual ci::vr::ContextRef			createContext( const ci::vr::SessionOptions& sessionOptions, uint32_t deviceIndex );
        
    protected:
        void setInfo(std::string firmware, std::string serial);
        
    private:
        ::PSVRApi::PSVRContextRef           mVrSystem;
        std::string							mDisplayName;
        
    };
    
}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )
