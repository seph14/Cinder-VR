/*
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

#include "cinder/vr/Hmd.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"

#if defined( CINDER_VR_ENABLE_PSVR )

#include "psvrapi.h"

namespace cinder { namespace vr { namespace psvr  {

class Context;

class Hmd;
using HmdRef = std::shared_ptr<Hmd>;

//! \class Hmd
//!
//!
class Hmd : public vr::Hmd {
public:

	virtual ~Hmd();

	static ci::vr::psvr::HmdRef		create( ci::vr::psvr::Context* context );

	// ---------------------------------------------------------------------------------------------
	// Public methods inherited from ci::vr::Hmd
	// ---------------------------------------------------------------------------------------------

	virtual void						recenterTrackingOrigin() override;

	virtual void						bind() override;
	virtual void						unbind() override;
	virtual void						submitFrame() override;

	virtual float						getFullFov() const;

	virtual ci::Area					getEyeViewport( ci::vr::Eye eye ) const override;

	virtual	void						enableEye( ci::vr::Eye eye, ci::vr::CoordSys eyeMatrixMode = ci::vr::COORD_SYS_WORLD ) override;

	virtual void						calculateOriginMatrix() override;
	virtual void						calculateInputRay() override;

	virtual void						drawDebugInfo() override;

protected:
	// ---------------------------------------------------------------------------------------------
	// Protected methods inherited from ci::vr::Hmd
	// ---------------------------------------------------------------------------------------------

	virtual void						onClipValueChange( float nearClip, float farClip ) override;
	virtual void						onMonoscopicChange() override;

	virtual void						drawMirroredImpl( const ci::Rectf& r ) override;

private:
	Hmd( ci::vr::psvr::Context* context );
	friend ci::vr::psvr::Context;

	ci::vr::psvr::Context				*mContext = nullptr;
	::PSVRApi::PSVRContextRef           mVrSystem;

	ci::gl::GlslProgRef					mDistortionShader;
	ci::gl::GlslProgRef 				mDebugShader;

	ci::ivec3							mSceneVolume = ci::ivec3( 20, 20, 20 );
	float								mNearClip = 0.1f;
	float								mFarClip = 100.0f;

	ci::mat4							mEyeProjectionMatrix[ci::vr::EYE_COUNT];
	ci::mat4							mEyePoseMatrix[ci::vr::EYE_COUNT];

	ci::gl::FboRef						mRenderTargetLeft;
	ci::gl::FboRef						mRenderTargetRight;

	uint32_t							mDistortionIndexCount = 0;
	ci::gl::BatchRef					mDistortionBatch;

	void								setupShaders();
	void								setupMatrices();
	void								setupStereoRenderTargets();
	void								setupDistortion();
	void								setupRenderModels();
	void								setupCompositor();

};

}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )
