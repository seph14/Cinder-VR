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

#include "cinder/vr/psvr/Hmd.h"
#include "cinder/vr/psvr/Context.h"
#include "cinder/vr/psvr/DeviceManager.h"
#include "psvrapi.h"

#if defined( CINDER_VR_ENABLE_PSVR )

#include "cinder/app/App.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/scoped.h"
#include "cinder/Log.h"

#include "vrdevice.h"

#include <iomanip>
#include <utility>

namespace cinder { namespace vr { namespace psvr {

const float kFullFov = 100.0f; // Degrees

//https://www.reddit.com/r/PSVR/comments/58jb5y/ipdeyetoeye_measurement_number_cheat_sheet/
const vec3 leftEyePos  = vec3(-0.0315f, 0.f, 0.f);
const vec3 rightEyePos = vec3(+0.0315f, 0.f, 0.f);
    
std::string toString( const ci::mat4& mat ) {
    const float *m = &(mat[0][0]);
    std::stringstream ss;
    ss  << std::fixed << std::setprecision(5);
    ss  << "[" << std::setw(10) << m[0] << " " << std::setw(10) << m[4] << " " << std::setw(10) << m[8]  <<  " " << std::setw(10) << m[12] << "]\n"
        << "[" << std::setw(10) << m[1] << " " << std::setw(10) << m[5] << " " << std::setw(10) << m[9]  <<  " " << std::setw(10) << m[13] << "]\n"
        << "[" << std::setw(10) << m[2] << " " << std::setw(10) << m[6] << " " << std::setw(10) << m[10] <<  " " << std::setw(10) << m[14] << "]\n"
        << "[" << std::setw(10) << m[3] << " " << std::setw(10) << m[7] << " " << std::setw(10) << m[11] <<  " " << std::setw(10) << m[15] << "]\n";
    //ss << std::resetissflags(std::iss_base::fixed | std::iss_base::floatfield);
    return ss.str();
}
    
std::string toString( const bool val ){
    return val ? "true" : "false";
}

// -------------------------------------------------------------------------------------------------
// Distortion Shader
// -------------------------------------------------------------------------------------------------
//taken from here https://github.com/gusmanb/PSVRFramework/blob/master/VRVideoPlayer/engine.h
//with changes
const std::string kDistortionShaderVertex = 
	"#version 410 core\n"
	"layout(location = 0) in vec4 ciPosition;\n"
	"layout(location = 1) in vec2 ciTexCoord0;\n"
	"uniform mat4 ciModelViewProjection;\n"
	"noperspective out vec2 vUV;\n"
	"void main()\n"
	"{\n"
	"	vUV   	    = ciTexCoord0;\n"
	"	gl_Position = ciModelViewProjection * ciPosition;\n"
	"}\n";

//need to change this shader so it renders separately for each eye
const std::string kDistortionShadeFragment = 
	"#version 410 core\n"
    
    "noperspective in vec2 vUV;\n"
    
    "uniform sampler2D  uTex0;\n"
	"uniform vec2       uDistortion;\n"
	"uniform vec4       uProjection;\n"
	"uniform vec4       uUnprojection;\n"
	"uniform bool       uShowCenter;\n"
    
	"const float uVEyeRelief = 0.06;\n"                    //const value
	"const float uHEyeRelief = 0.07;\n"                    //const value
	"const vec3  scale_in    = vec3(1.0, 1.0, 1.0);\n"     // eye relief in
	"const vec3  scale_out   = vec3(1.0, 1.130, 1.320);\n" // eye relief out
	
    "out vec4 outputColor;\n"
	
    "float poly(float val){\n"
	"	return (uShowCenter && val < 0.00010) ? \n"
	"       10000.0 : 1.0 + (uDistortion.x + uDistortion.y * val) * val;\n"
	"}\n"
    
	"vec2 barrel(vec2 v, vec4 projection, vec4 unprojection){\n"
	"	vec2 w = (v + unprojection.zw) / unprojection.xy;\n"
	"	return projection.xy * (poly(dot(w, w)) * w) - projection.zw;\n"
	"}\n"
    
	"void main() { \n"
	
    "	vec3 vScale = mix(scale_in, scale_out, uVEyeRelief);\n"
	"	vec3 hScale = mix(scale_in, scale_out, uHEyeRelief);\n"
    
	//"	vec4 projectionRight = (projectionLeft + vec4(0.0, 0.0, 1.0, 0.0)) * vec4(1.0, 1.0, -1.0, 1.0);\n"
	//"	vec4 unprojectionRight = (unprojectionLeft + vec4(0.0, 0.0, 1.0, 0.0)) * vec4(1.0, 1.0, -1.0, 1.0);\n"
	
    //"	vec2 a = (vUV.x < 0.5) ? \n"
	//"		barrel(vec2(vUV.x / 0.5, vUV.y), projectionLeft, unprojectionLeft) : \n"
	//"		barrel(vec2((vUV.x - 0.5) / 0.5, vUV.y), projectionRight, unprojectionRight);\n"
	
    "	vec2 a = barrel(vUV, uProjection, uUnprojection);\n"
    
    "	if (a.x < 0.0 || a.x > 1.0 || a.y < 0.0 || a.y > 1.0){\n"
	"		outputColor = vec4(0.0,0.0,0.0,1.0);\n"
	"	}else{\n"
    "       vec2 ra = 2.0 * a - 1.0;\n"
	"		vec2 ar = ((ra * hScale.rr) + 1.0) / 2.0;\n"
	"		vec2 ag = ((ra * hScale.gg) + 1.0) / 2.0;\n"
	"		vec2 ab = ((ra * hScale.bb) + 1.0) / 2.0;\n"
	"		outputColor.r = texture(uTex0, ar).r;\n"
	"		outputColor.g = texture(uTex0, ag).g;\n"
	"		outputColor.b = texture(uTex0, ab).b;\n"
	"		outputColor.a = 1;\n"
	"	}\n"
	"}";

// -------------------------------------------------------------------------------------------------
// Hmd
// -------------------------------------------------------------------------------------------------
Hmd::Hmd( ci::vr::psvr::Context* context ) : ci::vr::Hmd( context ), mContext( context ){
	mNearClip           = context->getSessionOptions().getNearClip();
	mFarClip            = context->getSessionOptions().getFarClip();
	mVrSystem           = context->getVrSystem();
    mOriginInitialized  = true;
	
    mVrSystem->statusReport.connect(std::bind(&Hmd::setStatus, this, std::placeholders::_1) );
    mMirrorMode = Hmd::MirrorMode::MIRROR_MODE_UNDISTORTED_STEREO;
    
    setupShaders();
	setupMatrices();
	setupStereoRenderTargets();
	setupDistortion();
	setupCompositor();
}

Hmd::~Hmd(){
}

ci::vr::psvr::HmdRef Hmd::create( ci::vr::psvr::Context* context ){
	ci::vr::psvr::HmdRef result = ci::vr::psvr::HmdRef( new ci::vr::psvr::Hmd( context ) );
	return result;
}

void Hmd::setupShaders(){
    //debug shader for transparent stuff?
    
	// Distortion shader
	try {
		mDistortionShader = ci::gl::GlslProg::create( kDistortionShaderVertex, kDistortionShadeFragment );
	}catch( const std::exception& e ) {
		std::string errMsg = "Distortion shader failed(" + std::string( e.what() ) + ")";
		throw ci::vr::psvr::Exception( errMsg );
	}
    
#if defined(DEBUG)
    mDistortionShader->uniform("uShowCenter", true);
#else
    mDistortionShader->uniform("uShowCenter", false);
#endif
}

void Hmd::setupMatrices(){
	mEyePoseMatrix[ci::vr::EYE_LEFT]        = getHmdEyePoseMatrix( ci::vr::EYE_LEFT );
    mEyePoseMatrix[ci::vr::EYE_RIGHT]       = getHmdEyePoseMatrix( ci::vr::EYE_RIGHT );
	mEyeProjectionMatrix[ci::vr::EYE_LEFT]  = getHmdEyeProjectionMatrix( ci::vr::EYE_LEFT,  mNearClip, mFarClip );
	mEyeProjectionMatrix[ci::vr::EYE_RIGHT] = getHmdEyeProjectionMatrix( ci::vr::EYE_RIGHT, mNearClip, mFarClip );
    
	// Set eye pose matrices
	mEyeCamera[ci::vr::EYE_LEFT].setViewMatrix ( mEyePoseMatrix[ci::vr::EYE_LEFT]  );
	mEyeCamera[ci::vr::EYE_RIGHT].setViewMatrix( mEyePoseMatrix[ci::vr::EYE_RIGHT] );
	// Set eye projection matrices
	mEyeCamera[ci::vr::EYE_LEFT].setProjectionMatrix ( mEyeProjectionMatrix[ci::vr::EYE_LEFT]  );
	mEyeCamera[ci::vr::EYE_RIGHT].setProjectionMatrix( mEyeProjectionMatrix[ci::vr::EYE_RIGHT] );
    
    if(mDistortionShader){
        VRDevice::vrdevicedata psvrDevice;
        VRDevice::vrlensprops lensProps;
        memcpy(&lensProps, &VRDevice::PSVRLensProps, sizeof(VRDevice::vrlensprops));
        VRDevice::initializeDevice(&psvrDevice, &VRDevice::PSVRScreenProps, &lensProps);
        
        mProjectVec   = psvrDevice.projectionLeft;
        mUnprojectVec = psvrDevice.unprojectionLeft;
        mDistortionShader->uniform("uDistortion", vec2(psvrDevice.lensProperties.distortionCoeffs[0],
                                                       psvrDevice.lensProperties.distortionCoeffs[1]));
    }
    
}

void Hmd::setupStereoRenderTargets(){
	uint32_t renderWidth = 0;
	uint32_t renderHeight = 0;
	mVrSystem->getRecommendedRenderTargetSize( &renderWidth, &renderHeight );
	mRenderTargetSize = ivec2( static_cast<int32_t>( renderWidth ), static_cast<int32_t>( renderHeight ) );
	CI_LOG_I( "mRenderTargetSize=" << mRenderTargetSize );

	// Texture format
	ci::gl::Texture2d::Format texFormat = ci::gl::Texture2d::Format();
	texFormat.setInternalFormat( GL_RGBA8 );
	texFormat.setWrapS( GL_CLAMP_TO_EDGE );
	texFormat.setWrapT( GL_CLAMP_TO_EDGE );
	texFormat.setMinFilter( GL_LINEAR );
	texFormat.setMagFilter( GL_LINEAR );
	// Fbo format
	ci::gl::Fbo::Format fboFormat = ci::gl::Fbo::Format();
	//fboFormat.setSamples( 4 );
	fboFormat.setColorTextureFormat( texFormat );
	fboFormat.enableDepthBuffer();
	// Render targets
	mRenderTargetLeft = ci::gl::Fbo::create( mRenderTargetSize.x, mRenderTargetSize.y, fboFormat );
	mRenderTargetRight = ci::gl::Fbo::create( mRenderTargetSize.x, mRenderTargetSize.y, fboFormat );
}

void Hmd::setupDistortion(){
	struct VertexDesc {
		ci::vec2 position;
		ci::vec2 texCoord;
	};

	std::vector<VertexDesc> vertexData;
	std::vector<uint16_t> indices;

    const int lensGridSegmentCountH = 3;
    const int lensGridSegmentCountV = 2;
    float w = 1.0f/static_cast<float>( lensGridSegmentCountH - 1 );
    float h = 1.0f/static_cast<float>( lensGridSegmentCountV - 1 );

	for(int y = 0; y < lensGridSegmentCountV; y++){
		for(int x = 0; x < lensGridSegmentCountH; x++){
			VertexDesc vert	= VertexDesc();
			vert.position	= ci::vec2( (float)x / w * 2 * mRenderTargetSize.x, (float)y / h * mRenderTargetSize.y );
			vert.texCoord	= ci::vec2( (float)x / w, (float)y / h );
			vertexData.push_back( vert );
		}
	}

	// Left eye indices
    indices.push_back( 0 ); indices.push_back( 1 ); indices.push_back( 4 );
    indices.push_back( 0 ); indices.push_back( 4 ); indices.push_back( 3 );
    
	// Right eye indices
    indices.push_back( 1 ); indices.push_back( 2 ); indices.push_back( 5 );
    indices.push_back( 1 ); indices.push_back( 5 ); indices.push_back( 4 );
    
	// Distortion index count
	mDistortionIndexCount = static_cast<uint32_t>( indices.size() );

	// Vertex data vbo
	ci::geom::BufferLayout layout = ci::geom::BufferLayout();
	layout.append( ci::geom::POSITION,    2, sizeof( VertexDesc ), static_cast<size_t>( offsetof( VertexDesc, position ) ), 0 );
	layout.append( ci::geom::TEX_COORD_0, 2, sizeof( VertexDesc ), static_cast<size_t>( offsetof( VertexDesc, texCoord ) ), 0 );
	ci::gl::VboRef vertexDataVbo = ci::gl::Vbo::create( GL_ARRAY_BUFFER, vertexData );
	std::vector<std::pair<ci::geom::BufferLayout, ci::gl::VboRef>> vertexArrayBuffers = { std::make_pair( layout, vertexDataVbo ) };
	// Indices vbo
	ci::gl::VboRef indicesVbo = ci::gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, indices );
	// Vbo mesh
	ci::gl::VboMeshRef vboMesh = ci::gl::VboMesh::create( static_cast<uint32_t>( vertexData.size() ), GL_TRIANGLES,
                                                          vertexArrayBuffers, static_cast<uint32_t>( indices.size() ),
                                                          GL_UNSIGNED_SHORT, indicesVbo );
	// Create batch
	mDistortionBatch = ci::gl::Batch::create( vboMesh, mDistortionShader );
}

void Hmd::setupCompositor()  { }

void Hmd::onClipValueChange( float nearClip, float farClip ){
	mNearClip = nearClip;
	mFarClip  = farClip;
	
	// Update projection matrices
	mEyeProjectionMatrix[ci::vr::EYE_LEFT]  = getHmdEyeProjectionMatrix( ci::vr::EYE_LEFT,  mNearClip, mFarClip );
	mEyeProjectionMatrix[ci::vr::EYE_RIGHT] = getHmdEyeProjectionMatrix( ci::vr::EYE_RIGHT, mNearClip, mFarClip );
	
	// Set eye projection matrices
	mEyeCamera[ci::vr::EYE_LEFT].setProjectionMatrix ( mEyeProjectionMatrix[ci::vr::EYE_LEFT]  );
	mEyeCamera[ci::vr::EYE_RIGHT].setProjectionMatrix( mEyeProjectionMatrix[ci::vr::EYE_RIGHT] );
}

const ci::mat4 Hmd::getHmdEyeProjectionMatrix(ci::vr::Eye eye, float nearClip, float farClip){
    float aspect  = 960.f / 1080.f;
    return glm::perspectiveFov( glm::radians(kFullFov / aspect), 960.f, 1080.f, mNearClip, mFarClip );
    //return glm::perspective(glm::radians(kFullFov), (960.f / 1080.f), farClip, nearClip);
}
    
const ci::mat4 Hmd::getHmdEyePoseMatrix(ci::vr::Eye eye){
    ci::vec3 position;
    switch(eye){
        case ci::vr::EYE_LEFT:
            position = leftEyePos;
            break;
        case ci::vr::EYE_RIGHT:
            position = rightEyePos;
            break;
        case ci::vr::EYE_HMD:
            position = vec3(0.f);
            break;
        case ci::vr::EYE_UNKNOWN:
        case ci::vr::EYE_COUNT:
            return glm::mat4(1.f);
    }
    
    ci::quat orientation = mContext->getDeviceOrientation();
    ci::mat4 rotMat      = glm::mat4_cast( orientation );
    ci::mat4 posMat      = glm::translate( position );
    ci::mat4 viewMatrix  = posMat*rotMat;
    return ci::inverse( viewMatrix );
}

void Hmd::onMonoscopicChange(){
    
}

void Hmd::recenterTrackingOrigin(){
    mVrSystem->recenterHeadset();
}

void Hmd::bind(){
    
    if( !mLedLit && (mInitTime > 0.f) && (float)ci::app::getElapsedSeconds() > mInitTime ){
        mLedLit = true;
        mVrSystem->setLED(PSVRApi::PSVR_LEDMASK::All, 0x64);
        mVrSystem->recalibrateHeadset();
    }
    
    //HMD matrix
    ci::quat orientation    = mContext->getDeviceOrientation();
    ci::mat4 rotMat         = glm::lookAt(vec3(0.f), vec3(0,0,1), vec3(0,1,0)); //glm::mat4_cast(orientation);
    
    ci::vec3 position       = vec3(0.f);
    ci::mat4 posMat         = glm::translate( position );
    ci::mat4 viewMatrix     = posMat*rotMat;
    viewMatrix = ci::inverse( viewMatrix );
    mHmdCamera.setViewMatrix( viewMatrix );
    
    //left eye
    position       = leftEyePos;
    posMat         = glm::translate( position );
    viewMatrix     = posMat*rotMat;
    viewMatrix = ci::inverse( viewMatrix );
    mEyeCamera[ci::vr::EYE_LEFT].setViewMatrix( viewMatrix );
    
    //right eye
    position       = rightEyePos;
    posMat         = glm::translate( position );
    viewMatrix     = posMat*rotMat;
    viewMatrix = ci::inverse( viewMatrix );
    mEyeCamera[ci::vr::EYE_RIGHT].setViewMatrix( viewMatrix );
}

void Hmd::unbind(){
    mRenderTargetLeft->unbindFramebuffer();
	mRenderTargetRight->unbindFramebuffer();
    updateElapsedFrames();
}

void Hmd::submitFrame(){ }

float Hmd::getFullFov() const{
	return kFullFov;
}

ci::Area Hmd::getEyeViewport( ci::vr::Eye eye ) const {
	auto size = mRenderTargetSize;
	if( ci::vr::EYE_LEFT == eye ) {
		return Area( 0, 0, size.x / 2, size.y );
	}
	return Area( ( size.x + 1 ) / 2, 0, size.x, size.y );
}

void Hmd::enableEye( ci::vr::Eye eye, ci::vr::CoordSys eyeMatrixMode ){
	switch( eye ) {
		case ci::vr::EYE_LEFT: {
			mRenderTargetLeft->bindFramebuffer();
			ci::gl::viewport( mRenderTargetLeft->getSize() );
			ci::gl::clear( mClearColor );
        }
		break;

		case ci::vr::EYE_RIGHT: {
			mRenderTargetRight->bindFramebuffer();
			ci::gl::viewport( mRenderTargetRight->getSize() );
			ci::gl::clear( mClearColor );
		}
		break;

		case ci::vr::EYE_HMD: {
			auto viewport = ci::gl::getViewport();
			auto area     = ci::Area( viewport.first.x, viewport.first.y, viewport.first.x + viewport.second.x, viewport.first.y + viewport.second.y );
			float width   = area.getWidth();
			float height  = area.getHeight();
			float aspect  = width / height;
			ci::mat4 mat  = glm::perspectiveFov( toRadians( getFullFov() / aspect ), width, height, mNearClip, mFarClip );
			mHmdCamera.setProjectionMatrix( mat );
			ci::gl::clear( mClearColor );
		}
		break;
            
        case ci::vr::EYE_COUNT:
        case ci::vr::EYE_UNKNOWN:
            //silence the warnings
            break;
	}

	setMatricesEye( eye, eyeMatrixMode );
    
    ci::gl::multModelMatrix( glm::mat4_cast(mContext->getDeviceOrientation()) );
}

void Hmd::calculateOriginMatrix(){ }

void Hmd::calculateInputRay(){
	// return view direction since we dont have any inputs available
	mInputRay = ci::Ray( vec3(0), mContext->getDeviceDirection() );
}

void Hmd::drawMirroredImpl( const ci::Rectf& r ){
	const uint32_t kTexUnit = 0;

	ci::gl::ScopedDepthTest scopedDepthTest( false );
	ci::gl::ScopedModelMatrix scopedModelMatrix;
	ci::gl::ScopedColor scopedColor( 1, 1, 1 );

	switch( mMirrorMode ) {
		// Default to stereo mirroring
		default:
		case Hmd::MirrorMode::MIRROR_MODE_STEREO: {
			ci::gl::ScopedGlslProg scopedShader( mDistortionShader );

            float width = r.getWidth();
            float height = r.getHeight();
            ci::Rectf fittedRect = ci::Rectf( 0, 0, width / 2.0f, height );

			//float w = r.getWidth() / 2.0f;
			//float h = r.getHeight() / 2.0f;
			// Offset and the scale to fit the rect
			/*ci::mat4 m = ci::mat4();
			m[0][0] =  w;
			m[1][1] = -h;
			m[3][0] =  w + r.x1;
			m[3][1] =  h + r.y1;
			ci::gl::multModelMatrix( m );*/

			// Render left eye
            {
                mDistortionShader->uniform("uProjection",   mProjectVec);
                mDistortionShader->uniform("uUnprojection", mUnprojectVec);
                
				auto resolvedTex = mRenderTargetLeft->getColorTexture();
				resolvedTex->bind( kTexUnit );
				mDistortionShader->uniform( "uTex0", kTexUnit );
				ci::gl::drawSolidRect( fittedRect );
                //mDistortionBatch->draw( 0, mDistortionIndexCount / 2 );
				resolvedTex->unbind( kTexUnit );
			}

			// Render right eye
			/*{
                vec4 projectionRight   = (mProjectVec   + vec4(0.0, 0.0, 1.0, 0.0)) * vec4(1.0, 1.0, -1.0, 1.0);
                vec4 unprojectionRight = (mUnprojectVec + vec4(0.0, 0.0, 1.0, 0.0)) * vec4(1.0, 1.0, -1.0, 1.0);
                mDistortionShader->uniform("uProjection",   projectionRight);
                mDistortionShader->uniform("uUnprojection", unprojectionRight);
                
				auto resolvedTex = mRenderTargetRight->getColorTexture();
				resolvedTex->bind( kTexUnit );
				mDistortionShader->uniform( "uTex0", kTexUnit );
				mDistortionBatch->draw( mDistortionIndexCount / 2, mDistortionIndexCount / 2 );
				resolvedTex->unbind( kTexUnit );
			}*/
		}
		break;

		case Hmd::MirrorMode::MIRROR_MODE_UNDISTORTED_STEREO: {
			float width = r.getWidth();
			float height = r.getHeight();
			ci::Rectf fittedRect = ci::Rectf( 0, 0, width / 2.0f, height );

			// Render left eye
			{
				auto resolvedTex = mRenderTargetLeft->getColorTexture();
				ci::gl::draw( resolvedTex, fittedRect );
			}

			// Render right eye
			{
				fittedRect += vec2( width / 2.0f, 0 );
				auto resolvedTex = mRenderTargetRight->getColorTexture();
				ci::gl::draw( resolvedTex, fittedRect );
			}
		}
		break;

		case Hmd::MirrorMode::MIRROR_MODE_UNDISTORTED_MONO_LEFT: {
			auto tex = mRenderTargetLeft->getColorTexture();
			float width = static_cast<float>( tex->getWidth() );
			float height = static_cast<float>( tex->getHeight() );
			auto texRect = ci::Rectf( 0, 0, width, height );
			auto fittedRect = r.getCenteredFit( texRect, true );
			ci::gl::draw( tex, Area( fittedRect ), r );	
		}
		break;

		case Hmd::MirrorMode::MIRROR_MODE_UNDISTORTED_MONO_RIGHT: {
			auto tex = mRenderTargetRight->getColorTexture();
			float width = static_cast<float>( tex->getWidth() );
			float height = static_cast<float>( tex->getHeight() );
			auto texRect = ci::Rectf( 0, 0, width, height );
			auto fittedRect = r.getCenteredFit( texRect, true );
			ci::gl::draw( tex, Area( fittedRect ), r );
		}
		break;
	}
}

void Hmd::drawDebugInfo(){
}
    
void Hmd::setStatus(void *status){
    PSVRApi::PSVRStatus *stat = (PSVRApi::PSVRStatus*)status;
    
    //hacking to get the LEDs to work, when send the led cmd to fast it throws out unsolicited report
    //start after we are sure the headset is on seems fixed it
    if(mInitTime < 0.f && !stat->isCinematic)
        mInitTime = (float)ci::app::getElapsedSeconds() + 2.f;
    if(stat->isHeadsetWorn){
        mMirrorMode = Hmd::MirrorMode::MIRROR_MODE_STEREO;
    }else{
        mMirrorMode = Hmd::MirrorMode::MIRROR_MODE_UNDISTORTED_STEREO;
    }
    
    CI_LOG_I("\nPSVR Status Update: \n" <<
             "Is Headset On:      " << toString(stat->isHeadsetOn) << "\n" <<
             "Is Headset Worn:    " << toString(stat->isHeadsetWorn) << "\n" <<
             "Is Cinematic Mode:  " << toString(stat->isCinematic) << "\n" <<
             "Are Headphone Used: " << toString(stat->areHeadphonesUsed) << "\n" <<
             "Is Muted:           " << toString(stat->isMuted) << "\n" <<
             "Is CEC Used:        " << toString(stat->isCECUsed) << "\n" <<
             "Volume:             " << stat->volume << "\n");
}

}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )

