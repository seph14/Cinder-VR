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

	virtual float						getFullFov() const override;

	virtual ci::Area					getEyeViewport( ci::vr::Eye eye ) const override;

	virtual	void						enableEye( ci::vr::Eye eye, ci::vr::CoordSys eyeMatrixMode = ci::vr::COORD_SYS_WORLD ) override;

	virtual void						calculateOriginMatrix() override;
	virtual void						calculateInputRay() override;

    virtual void						drawControllers( ci::vr::Eye eyeType ) override { };
	virtual void						drawDebugInfo() override;

protected:
	// ---------------------------------------------------------------------------------------------
	// Protected methods inherited from ci::vr::Hmd
	// ---------------------------------------------------------------------------------------------

	virtual void						onClipValueChange( float nearClip, float farClip ) override;
	virtual void						onMonoscopicChange() override;

	virtual void						drawMirroredImpl( const ci::Rectf& r ) override;

    void                                setStatus(void *status);
    const ci::mat4                      getHmdEyePoseMatrix(ci::vr::Eye eye);
    const ci::mat4                      getHmdEyeProjectionMatrix(ci::vr::Eye eye, float nearClip, float farClip);
    
private:
	Hmd( ci::vr::psvr::Context* context );
	friend ci::vr::psvr::Context;

	ci::vr::psvr::Context				*mContext = nullptr;
	::PSVRApi::PSVRContextRef           mVrSystem;

	ci::gl::GlslProgRef					mDistortionShader;
	ci::gl::GlslProgRef 				mDebugShader;

	float								mNearClip    = 0.1f;
	float								mFarClip     = 100.0f;
    bool                                mLedLit      = false;
    float                               mInitTime    = -1.f;
    
    ci::vec4                            mProjectVec, mUnprojectVec;
    
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
	void								setupCompositor();
};

}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )
