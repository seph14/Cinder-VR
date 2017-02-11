#pragma once

#include "cinder/Cinder.h"
#include "cinder/Thread.h"
#include "cinder/app/App.h"
#include "cinder/Signals.h"

#include "BMI055Integrator.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <memory>

// Get rid of annoying zero length structure warnings from libusb.h in MSVC

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4200)
#endif

#include "libusb.h"

//typedef unsigned char byte;
typedef uint8_t byte;

///
/// \brief The PSVRApi namespace
///
namespace PSVRApi{
    
	///
	/// \brief PSVR_VID
	///
	static int const           PSVR_VID(0x054C);
	///
	/// \brief PSVR_PID
	///
	static int const           PSVR_PID(0x09AF);
	///
	/// \brief PSVR_EP_COMMAND
	///
	static int const           PSVR_EP_CMD_WRITE(0x04);
	///
	/// \brief PSVR_EP_SENSOR
	///
	static int const           PSVR_EP_CMD_READ(PSVR_EP_CMD_WRITE | 0x80);
	///
	/// \brief PSVR_EP_SENSOR
	///
	static int const           PSVR_EP_SENSOR(0x03 | 0x80);
	///
	/// \brief PSVR_INFO_REPORT
	///
	static int const           PSVR_INFO_REPORT(0x80);
	///
	/// \brief PSVR_STATUS_REPORT
	///
	static int const           PSVR_STATUS_REPORT(0xF0);
	///
	/// \brief PSVR_UNSOL_REPORT
	///
	static int const           PSVR_UNSOLICITED_REPORT(0xA0);

    static int const           PSVR_CONFIGURATION(0);
    
	/// ----------------------------------
	/// \brief The PSVR_USB_INTERFACE enum
	///
	typedef enum {
		AUDIO_3D      = 0,
		AUDIO_CONTROL = 1,
		AUDIO_MIC     = 2,
		AUDIO_CHAT    = 3,
		HID_SENSOR    = 4, //sensor for polling all data
        HID_CONTROL   = 5, //controller
		VS_H264       = 6,
		VS_BULK_IN    = 7,
		HID_CONTROL2  = 8
	} PSVR_USB_INTERFACE;

    typedef enum : byte{
		VolumeUp   = 2,
		VolumeDown = 4,
		Mute       = 8
	} PSVR_HEADSET_BUTTONS;

	typedef enum : uint16_t {
		None = 0,
		LedA = (1 << 0),
		LedB = (1 << 1),
		LedC = (1 << 2),
		LedD = (1 << 3),
		LedE = (1 << 4),
		LedF = (1 << 5),
		LedG = (1 << 6),
		LedH = (1 << 7),
		LedI = (1 << 8),
		All = LedA | LedB | LedC | LedD | LedE | LedF | LedG | LedH | LedI
	} PSVR_LEDMASK;
	
    /// -----------------------------
	/// \brief The PSVRFrame struct
	///
    
#pragma pack(1)
	struct PSVRFrame {
		byte                   id;
		byte                   status;
		byte                   start;
		byte                   length;
		byte                   data[60];
	};

	struct PSVRSensorFrame {
        PSVR_HEADSET_BUTTONS  buttons;
        byte                  u_b01;
        byte                  volume;
        byte                  u_b02[5];
		byte                  status;
        byte                  u_b03[7];
        //16
        
        struct {
            uint32_t timestamp;
            struct {
                int16_t yaw;
                int16_t pitch;
                int16_t roll;
            } gyro;
            struct {
                int16_t x;
                int16_t y;
                int16_t z;
            } accel;
        } data[2];
        //48
        
        byte                  calStatus;
        byte                  ready;
        byte                  u_b04[3];
        
        /*
		byte                  timeStampA1;
		byte                  timeStampA2;
		byte                  timeStampA3;
		byte                  timeStampA4;

		byte                  rawGyroYaw_AL;
		byte                  rawGyroYaw_AH;

		byte                  rawGyroPitch_AL;
		byte                  rawGyroPitch_AH;

		byte                  rawGyroRoll_AL;
		byte                  rawGyroRoll_AH;

		byte                  rawMotionX_AL;
		byte                  rawMotionX_AH;

		byte                  rawMotionY_AL;
		byte                  rawMotionY_AH;

		byte                  rawMotionZ_AL;
		byte                  rawMotionZ_AH;        // 32 bytes

		byte                  timeStamp_B1;
		byte                  timeStamp_B2;
		byte                  timeStamp_B3;
		byte                  timeStamp_B4;

		byte                  rawGyroYaw_BL;
		byte                  rawGyroYaw_BH;

		byte                  rawGyroPitch_BL;
		byte                  rawGyroPitch_BH;

		byte                  rawGyroRoll_BL;
		byte                  rawGyroRoll_BH;

		byte                  rawMotionX_BL;
		byte                  rawMotionX_BH;

		byte                  rawMotionY_BL;
		byte                  rawMotionY_BH;

		byte                  rawMotionZ_BL;
		byte                  rawMotionZ_BH;
*/

		byte                  voltageValue;
		byte                  voltageReference;
		int16_t               irSensor;

        byte                  u_b05[5];
		byte                  frameSequence;
        byte                  u_b06;
	};
    
#pragma pack()
	///
	/// \brief The PSVRSensorData struct
	///
	struct PSVRSensorData {
		bool                      volumeUpPressed;
		bool                      volumeDownPressed;
		bool                      mutePressed;

		byte                      volume;

		bool                      isWorn;
		bool                      isDisplayActive;
		bool                      isMicMuted;
		bool                      earphonesConnected;

		int                       timeStamp_A;

		int                       rawGyroYaw_A;
		int                       rawGyroPitch_A;
		int                       rawGyroRoll_A;

		int                       rawMotionX_A;
		int                       rawMotionY_A;
		int                       rawMotionZ_A;

		int                       timeStamp_B;

		int                       rawGyroYaw_B;
		int                       rawGyroPitch_B;
		int                       rawGyroRoll_B;

		int                       rawMotionX_B;
		int                       rawMotionY_B;
		int                       rawMotionZ_B;

		byte                      calStatus;
		byte                      ready;

		byte                      voltageValue;
		byte                      voltageReference;

		short                     irSensor;

		byte                      frameSequence;
	};

	///
	/// \brief The PSVRStatus struct
	///
	struct                     PSVRStatus {
	public:
		bool                   isHeadsetOn;
		bool                   isHeadsetWorn;
		bool                   isCinematic;
		bool                   areHeadphonesUsed;
		bool                   isMuted;
		bool                   isCECUsed;
		int                    volume;
	};

	class PSVRContext {

	public:
        libusb_context *usb = NULL;
        
        PSVRContext( libusb_device* device );
		~PSVRContext();
        
        static std::vector<libusb_device*>   listPSVR();
        static std::shared_ptr<PSVRContext>  initPSVR();
        static std::shared_ptr<PSVRContext>  create( libusb_device* device ) { return std::shared_ptr<PSVRContext>(new PSVRContext(device)); }
        
        bool turnHeadSetOn();
        bool turnHeadSetOff();
        
        bool setLED(PSVR_LEDMASK mask, byte brightess);
        bool setLED(PSVR_LEDMASK mask, byte valueA, byte valueB, byte valueC, byte valueD, byte valueE, byte valueF, byte valueG, byte valueH, byte valueI);

        bool enableVRTracking();
        bool enableVR();
        
        bool enableCinematicMode();
        bool enableCinematicMode(byte distance, byte size, byte brightness, byte micVolume);
        
        /*not implemented*/
        bool recenterHeadset();
        
        bool turnBreakBoxOff();
        bool turnBreakBoxOn();
        
        bool ReadInfo();
        
        bool isHeadsetOn()	     { return stat->isHeadsetOn; }
        bool isHeadsetWorn()     { return stat->isHeadsetWorn; }
        bool isCinematic()       { return stat->isCinematic; }
        bool areHeadphonesUsed() { return stat->areHeadphonesUsed; }
        bool isMuted()           { return stat->isMuted; }
        bool isCECUsed()         { return stat->isCECUsed; }
        int  getVolume()         { return stat->volume; }
        
        std::string            getSerialNumber();
        std::string            getVersion();
        
        ci::signals::Signal<void(bool)>                                            connect;
        ci::signals::Signal<void(std::string, std::string)>                        infoReport;
        ci::signals::Signal<void(void*)>                                           statusReport;
        ci::signals::Signal<void(byte reportId, byte result, std::string message)> unsolicitedReport;
        ci::signals::Signal<void(glm::quat quat, glm::vec3 euler)>                 rotationUpdate;
        
    protected:
		std::string            serialNumber;
		unsigned char          minorVersion;
		unsigned char          majorVersion;
        
        PSVRApi::PSVRStatus *stat;
        
        libusb_device_handle *usbHdl            = NULL;
        struct libusb_config_descriptor *config = NULL;
        uint32_t claimed_interfaces;
        
        bool SendCommand(PSVRFrame *sendCmd);
        
        std::shared_ptr<std::thread> sensorThread, controlThread;
        
        bool running = false;
        void sensorProcess();
        
        void controllerProcess();
        
        void processSensorFrame   (PSVRSensorFrame rawFrame, PSVRSensorData *rawData);
        void processControlFrame  (PSVRFrame frame);
        
        void emitInfoReport       (PSVRFrame frame);
        void emitStatusReport     (PSVRFrame frame);
        void emitUnsolicitedReport(PSVRFrame frame);
    };
    
    typedef std::shared_ptr<PSVRContext> PSVRContextRef;
}