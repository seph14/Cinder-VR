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
#include "cinder/vr/psvr/psvrapi.h"

#if defined( CINDER_VR_ENABLE_PSVR )

#include "cinder/app/App.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/scoped.h"
#include "cinder/Log.h"

#include <iomanip>
#include <utility>

namespace cinder { namespace vr { namespace psvr {

const float kFullFov = 100.0f; // Degrees

std::string toString( const ci::mat4& mat ) 
{
	const float *m = &(mat[0][0]);
	std::stringstream ss;
    ss << std::fixed << std::setprecision(5);
    ss << "[" << std::setw(10) << m[0] << " " << std::setw(10) << m[4] << " " << std::setw(10) << m[8]  <<  " " << std::setw(10) << m[12] << "]\n"
       << "[" << std::setw(10) << m[1] << " " << std::setw(10) << m[5] << " " << std::setw(10) << m[9]  <<  " " << std::setw(10) << m[13] << "]\n"
       << "[" << std::setw(10) << m[2] << " " << std::setw(10) << m[6] << " " << std::setw(10) << m[10] <<  " " << std::setw(10) << m[14] << "]\n"
       << "[" << std::setw(10) << m[3] << " " << std::setw(10) << m[7] << " " << std::setw(10) << m[11] <<  " " << std::setw(10) << m[15] << "]\n";
    //ss << std::resetissflags(std::iss_base::fixed | std::iss_base::floatfield);
    return ss.str();
}

// -------------------------------------------------------------------------------------------------
// Distortion Shader
// -------------------------------------------------------------------------------------------------
//https://github.com/gusmanb/PSVRFramework/blob/master/VRVideoPlayer/engine.h
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
	"uniform sampler2D texture;\n"
	"uniform vec2 distortion;\n"
	"uniform vec4 backgroundColor;\n"
	"uniform vec4 projectionLeft;\n"
	"uniform vec4 unprojectionLeft;\n"
	"uniform int showCenter;\n"
	"uniform float vEyeRelief = 1.0;\n"
	"uniform float hEyeRelief = 1.0;\n"
	"const vec3 scale_in = vec3(1.0, 1.0, 1.0);\n"  // eye relief in
	"const vec3 scale_out = vec3(1.0, 1.130, 1.320);\n"  // eye relief out
	"out vec4 outputColor;\n"
	"in vec2 vUV;\n"
	"float poly(float val){\n"
	"	return (showCenter == 1 && val < 0.00010) ? \n"
	"	10000.0 : 1.0 + (distortion.x + distortion.y * val) * val;\n"
	"}\n"
	"vec2 barrel(vec2 v, vec4 projection, vec4 unprojection){\n"
	"	vec2 w = (v + unprojection.zw) / unprojection.xy;\n"
	"	return projection.xy * (poly(dot(w, w)) * w) - projection.zw;\n"
	"}\n"
	"\n"
	"void main() { \n"
	"	vec3 vScale = mix(scale_in, scale_out, vEyeRelief);\n"
	"	vec3 hScale = mix(scale_in, scale_out, hEyeRelief);\n"
	"	vec4 projectionRight = (projectionLeft + vec4(0.0, 0.0, 1.0, 0.0)) * vec4(1.0, 1.0, -1.0, 1.0);\n"
	"	vec4 unprojectionRight = (unprojectionLeft + vec4(0.0, 0.0, 1.0, 0.0)) * vec4(1.0, 1.0, -1.0, 1.0);\n"
	"	vec2 a = (vUV.x < 0.5) ? \n"
	"		barrel(vec2(vUV.x / 0.5, vUV.y), projectionLeft, unprojectionLeft) : \n"
	"		barrel(vec2((vUV.x - 0.5) / 0.5, vUV.y), projectionRight, unprojectionRight);\n"
	"	if (a.x < 0.0 || a.x > 1.0 || a.y < 0.0 || a.y > 1.0)\n"
	"		outputColor = backgroundColor;\n"
	"	else{\n"
	"		vec2 ar = (((a * 2.0 - 1.0) * vec2(hScale.r, vScale.r)) + 1.0) / 2.0;\n"
	"		vec2 ag = (((a * 2.0 - 1.0) * vec2(hScale.g, vScale.g)) + 1.0) / 2.0;\n"
	"		vec2 ab = (((a * 2.0 - 1.0) * vec2(hScale.b, vScale.b)) + 1.0) / 2.0;\n"
	"		outputColor.r = texture2D(texture, vec2(ar.x * 0.5 + (finalUv.x < 0.5 ? 0.0 : 0.5), ar.y)).r;\n"
	"		outputColor.g = texture2D(texture, vec2(ag.x * 0.5 + (finalUv.x < 0.5 ? 0.0 : 0.5), ag.y)).g;\n"
	"		outputColor.b = texture2D(texture, vec2(ab.x * 0.5 + (finalUv.x < 0.5 ? 0.0 : 0.5), ab.y)).b;\n"
	"		outputColor.a = 1;\n"
	"	}\n"
	"}";

// -------------------------------------------------------------------------------------------------
// Hmd
// -------------------------------------------------------------------------------------------------
Hmd::Hmd( ci::vr::psvr::Context* context )
	: ci::vr::Hmd( context ), mContext( context )
{
	mNearClip = context->getSessionOptions().getNearClip();
	mFarClip = context->getSessionOptions().getFarClip();

	mVrSystem = context->getVrSystem();

	mRenderModels.resize( ::vr::k_unMaxTrackedDeviceCount );	

	setupShaders();
	setupMatrices();
	setupStereoRenderTargets();
	setupDistortion();
	setupRenderModels();
	setupCompositor();

	auto shader = ci::gl::getStockShader( ci::gl::ShaderDef().texture( GL_TEXTURE_2D ) );
	mControllerIconBatch[ci::vr::Controller::TYPE_LEFT] = ci::gl::Batch::create( ci::geom::Plane().axes( ci::vec3( 1, 0, 0 ), ci::vec3( 0, 0, -1 ) ), shader );
	mControllerIconBatch[ci::vr::Controller::TYPE_RIGHT] = ci::gl::Batch::create( ci::geom::Plane().axes( ci::vec3( 1, 0, 0 ), ci::vec3( 0, 0, -1 ) ), shader );
}

Hmd::~Hmd()
{
}

ci::vr::psvr::HmdRef Hmd::create( ci::vr::psvr::Context* context )
{
	ci::vr::psvr::HmdRef result = ci::vr::psvr::HmdRef( new ci::vr::psvr::Hmd( context ) );
	return result;
}

void Hmd::setupShaders()
{
	// Distortion shader
	try {
		mDistortionShader = ci::gl::GlslProg::create( kDistortionShaderVertex, kDistortionShadeFragment );
	}catch( const std::exception& e ) {
		std::string errMsg = "Distortion shader failed(" + std::string( e.what() ) + ")";
		throw ci::vr::psvr::Exception( errMsg );
	}
}

void Hmd::setupMatrices(){
	mEyePoseMatrix[ci::vr::EYE_LEFT]        = getHmdEyePoseMatrix( mVrSystem, ::vr::Eye_Left );
	mEyePoseMatrix[ci::vr::EYE_RIGHT]       = getHmdEyePoseMatrix( mVrSystem, ::vr::Eye_Right );
	mEyeProjectionMatrix[ci::vr::EYE_LEFT]  = getHmdEyeProjectionMatrix( mVrSystem, ::vr::Eye_Left, mNearClip, mFarClip );
	mEyeProjectionMatrix[ci::vr::EYE_RIGHT] = getHmdEyeProjectionMatrix( mVrSystem, ::vr::Eye_Right, mNearClip, mFarClip );
	// Set eye pose matrices
	mEyeCamera[ci::vr::EYE_LEFT].setViewMatrix( mEyePoseMatrix[ci::vr::EYE_LEFT] );
	mEyeCamera[ci::vr::EYE_RIGHT].setViewMatrix( mEyePoseMatrix[ci::vr::EYE_RIGHT] );
	// Set eye projection matrices
	mEyeCamera[ci::vr::EYE_LEFT].setProjectionMatrix( mEyeProjectionMatrix[ci::vr::EYE_LEFT] );
	mEyeCamera[ci::vr::EYE_RIGHT].setProjectionMatrix( mEyeProjectionMatrix[ci::vr::EYE_RIGHT] );
}

void Hmd::setupStereoRenderTargets(){
	uint32_t renderWidth = 0;
	uint32_t renderHeight = 0;
	mVrSystem->GetRecommendedRenderTargetSize( &renderWidth, &renderHeight );
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
	fboFormat.setSamples( 4 );
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

	float xOffset = -1.0f;
	for(int y = 0; y < 2; y++){
		for(int x = 0; x < 2; x++){
			VertexDesc vert		= VertexDesc();
			vert.position		= ci::vec2( xOffset + u, -1.0f + ( 2.0f * y * h ) );
			vert.texCoordRed	= ci::vec2( dc.rfRed[0],   1.0f - dc.rfRed[1] );
			vert.texCoordGreen	= ci::vec2( dc.rfGreen[0], 1.0f - dc.rfGreen[1] );
			vert.texCoordBlue	= ci::vec2( dc.rfBlue[0],  1.0f - dc.rfBlue[1] );
			vertexData.push_back( vert );
		}
	}

	const int lensGridSegmentCountH = 4;
	const int lensGridSegmentCountV = 4;
	float w = 1.0f/static_cast<float>( lensGridSegmentCountH - 1 );
	float h = 1.0f/static_cast<float>( lensGridSegmentCountV - 1 );

	// Left eye distortion vertex data
	float xOffset = -1.0f;
	for( int y = 0; y < lensGridSegmentCountV; y++ ) {
		for( int x = 0; x < lensGridSegmentCountH; ++x ) {
			float u = x * w; 
			float v = 1.0f - ( y * h );
			::vr::DistortionCoordinates_t dc = mVrSystem->ComputeDistortion( ::vr::Eye_Left, u, v );
			
			VertexDesc vert		= VertexDesc();
			vert.position		= ci::vec2( xOffset + u, -1.0f + ( 2.0f * y * h ) );
			vert.texCoordRed	= ci::vec2( dc.rfRed[0],   1.0f - dc.rfRed[1] );
			vert.texCoordGreen	= ci::vec2( dc.rfGreen[0], 1.0f - dc.rfGreen[1] );
			vert.texCoordBlue	= ci::vec2( dc.rfBlue[0],  1.0f - dc.rfBlue[1] );
			vertexData.push_back( vert );
		}
	}

	// Right eye distortion vertex data
	xOffset = 0;
	for( int y = 0; y < lensGridSegmentCountV; y++ ) {
		for( int x = 0; x < lensGridSegmentCountH; ++x ) {
			float u = x * w; 
			float v = 1 - ( y * h );
			::vr::DistortionCoordinates_t dc = mVrSystem->ComputeDistortion( ::vr::Eye_Right, u, v );

			VertexDesc vert		= VertexDesc();
			vert.position		= ci::vec2( xOffset + u, -1.0f + ( 2.0f * y * h ) );
			vert.texCoordRed	= ci::vec2( dc.rfRed[0],   1.0f - dc.rfRed[1] );
			vert.texCoordGreen	= ci::vec2( dc.rfGreen[0], 1.0f - dc.rfGreen[1] );
			vert.texCoordBlue	= ci::vec2( dc.rfBlue[0],  1.0f - dc.rfBlue[1] );
			vertexData.push_back( vert );
		}
	}

	// Left eye indices
	uint16_t offset = 0;
	for( uint16_t y = 0; y < ( lensGridSegmentCountV - 1 ); ++y )	{
		for( uint16_t x = 0; x < ( lensGridSegmentCountH - 1 ); ++x )		{
			uint16_t a = ( lensGridSegmentCountH * y ) + x + offset;
			uint16_t b = ( lensGridSegmentCountH * y ) + x + 1 + offset;
			uint16_t c = ( ( y+ 1 ) * lensGridSegmentCountH ) + x + 1 + offset;
			uint16_t d = ( ( y +1 ) * lensGridSegmentCountH ) + x  + offset;

			indices.push_back( a );
			indices.push_back( b );
			indices.push_back( c );

			indices.push_back( a );
			indices.push_back( c );
			indices.push_back( d );
		}
	}

	// Right eye indices
	offset = (lensGridSegmentCountH)*(lensGridSegmentCountV);
	for( uint16_t y = 0; y < ( lensGridSegmentCountV - 1 ); ++y ) {
		for( uint16_t x = 0; x < ( lensGridSegmentCountH - 1 ); ++x ) {
			uint16_t a = ( lensGridSegmentCountH * y ) + x + offset;
			uint16_t b = ( lensGridSegmentCountH * y ) + x + 1 + offset;
			uint16_t c = ( ( y+ 1 ) * lensGridSegmentCountH ) + x + 1 + offset;
			uint16_t d = ( ( y +1 ) * lensGridSegmentCountH ) + x  + offset;

			indices.push_back( a );
			indices.push_back( b );
			indices.push_back( c );

			indices.push_back( a );
			indices.push_back( c );
			indices.push_back( d );
		}
	}

	// Distortion index count
	mDistortionIndexCount = static_cast<uint32_t>( indices.size() );

	// Vertex data vbo
	ci::geom::BufferLayout layout = ci::geom::BufferLayout();
	layout.append( ci::geom::POSITION,    2, sizeof( VertexDesc ), static_cast<size_t>( offsetof( VertexDesc, position ) ),      0 );
	layout.append( ci::geom::TEX_COORD_0, 2, sizeof( VertexDesc ), static_cast<size_t>( offsetof( VertexDesc, texCoordRed ) ),   0 );
	layout.append( ci::geom::TEX_COORD_1, 2, sizeof( VertexDesc ), static_cast<size_t>( offsetof( VertexDesc, texCoordGreen ) ), 0 );
	layout.append( ci::geom::TEX_COORD_2, 2, sizeof( VertexDesc ), static_cast<size_t>( offsetof( VertexDesc, texCoordBlue ) ),  0 );	
	ci::gl::VboRef vertexDataVbo = ci::gl::Vbo::create( GL_ARRAY_BUFFER, vertexData );
	std::vector<std::pair<ci::geom::BufferLayout, ci::gl::VboRef>> vertexArrayBuffers = { std::make_pair( layout, vertexDataVbo ) };
	
	// Indices vbo
	ci::gl::VboRef indicesVbo = ci::gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, indices );

	// Vbo mesh
	ci::gl::VboMeshRef vboMesh = ci::gl::VboMesh::create( static_cast<uint32_t>( vertexData.size() ), GL_TRIANGLES, vertexArrayBuffers, static_cast<uint32_t>( indices.size() ), GL_UNSIGNED_SHORT, indicesVbo );

	// Create batch
	mDistortionBatch = ci::gl::Batch::create( vboMesh, mDistortionShader );
}

void Hmd::setupRenderModels()
{
	// Allocate entries for all tracked devices
	mRenderModels.resize( ::vr::k_unMaxTrackedDeviceCount );

	for( ::vr::TrackedDeviceIndex_t trackedDeviceIndex = ::vr::k_unTrackedDeviceIndex_Hmd + 1; trackedDeviceIndex < ::vr::k_unMaxTrackedDeviceCount; ++trackedDeviceIndex ) {
		activateRenderModel( trackedDeviceIndex );
	}
}

void Hmd::setupCompositor()
{
	if( ! ::vr::VRCompositor() ) {
		throw ci::vr::psvr::Exception( "Couldn't initialize compositor" );
	}

	switch( getSessionOptions().getTrackingOrigin() ) {
		case ci::vr::TRACKING_ORIGIN_SEATED: {
			::vr::VRCompositor()->SetTrackingSpace( ::vr::TrackingUniverseSeated );
		}
		break;

		case ci::vr::TRACKING_ORIGIN_STANDING: {
			::vr::VRCompositor()->SetTrackingSpace( ::vr::TrackingUniverseStanding );
		}
		break;
	}
}

void Hmd::updatePoseData(){
	mContext->updatePoseData();
	const auto& pose = mContext->getPose( ::vr::k_unTrackedDeviceIndex_Hmd );

	if( pose.bPoseIsValid ) {
		const auto& hmdMat = mContext->getTrackingToDeviceMatrix( ::vr::k_unTrackedDeviceIndex_Hmd );
		mEyeCamera[ci::vr::EYE_LEFT].setHmdMatrix( hmdMat );
		mEyeCamera[ci::vr::EYE_RIGHT].setHmdMatrix( hmdMat );
		mHmdCamera.setHmdMatrix( hmdMat );

		mDeviceToTrackingMatrix = mContext->getDeviceToTrackingMatrix( ::vr::k_unTrackedDeviceIndex_Hmd );
		mTrackingToDeviceMatrix = mContext->getTrackingToDeviceMatrix( ::vr::k_unTrackedDeviceIndex_Hmd );
	}

	if( ( ! mOriginInitialized ) && pose.bPoseIsValid ) {
		calculateOriginMatrix();
		// Flag as initialized		
		mOriginInitialized = true;
	}

	if( mOriginInitialized ) {
		calculateInputRay();
	}
}

void Hmd::onClipValueChange( float nearClip, float farClip ){
	mNearClip = nearClip;
	mFarClip = farClip;
	
	// Update projection matrices
	mEyeProjectionMatrix[ci::vr::EYE_LEFT]  = getHmdEyeProjectionMatrix( mVrSystem, ::vr::Eye_Left, mNearClip, mFarClip );
	mEyeProjectionMatrix[ci::vr::EYE_RIGHT] = getHmdEyeProjectionMatrix( mVrSystem, ::vr::Eye_Right, mNearClip, mFarClip );
	
	// Set eye projection matrices
	mEyeCamera[ci::vr::EYE_LEFT].setProjectionMatrix( mEyeProjectionMatrix[ci::vr::EYE_LEFT] );
	mEyeCamera[ci::vr::EYE_RIGHT].setProjectionMatrix( mEyeProjectionMatrix[ci::vr::EYE_RIGHT] );
}

void Hmd::onMonoscopicChange(){

}

void Hmd::recenterTrackingOrigin()
{
}

void Hmd::bind(){}

void Hmd::unbind()
{
	mRenderTargetLeft->unbindFramebuffer();
	mRenderTargetRight->unbindFramebuffer();

/*
	submitFrame();
	updatePoseData(); 

	const auto& pose = mContext->getPose( ::vr::k_unTrackedDeviceIndex_Hmd );
	if( pose.bPoseIsValid ) {
		updateElapsedFrames();
	}
*/
}

void Hmd::submitFrame(){
	// Left eye
	{
		GLuint resolvedTexId = mRenderTargetLeft->getColorTexture()->getId();
		::vr::Texture_t eyeTex = { reinterpret_cast<void*>( resolvedTexId ), ::vr::API_OpenGL, ::vr::ColorSpace_Gamma };
		::vr::VRCompositor()->Submit( ::vr::Eye_Left, &eyeTex );
	}

	// Right eye
	{
		GLuint resolvedTexId = mRenderTargetRight->getColorTexture()->getId();
		::vr::Texture_t eyeTex = { reinterpret_cast<void*>( resolvedTexId ), ::vr::API_OpenGL, ::vr::ColorSpace_Gamma };
		::vr::VRCompositor()->Submit( ::vr::Eye_Right, &eyeTex );
	}

	{
		// Note from psvr sample:
		//
		// HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
		// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
		// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
		// 1/29/2014 mikesart
		//
		glFinish();
	}


	// Update pose data
	{
		updatePoseData(); 

		const auto& pose = mContext->getPose( ::vr::k_unTrackedDeviceIndex_Hmd );
		if( pose.bPoseIsValid ) {
			updateElapsedFrames();
		}
	}
}

float Hmd::getFullFov() const
{
	return kFullFov;
}

ci::Area Hmd::getEyeViewport( ci::vr::Eye eye ) const
{
	auto size = mRenderTargetSize;
	if( ci::vr::EYE_LEFT == eye ) {
		return Area( 0, 0, size.x / 2, size.y );
	}
	return Area( ( size.x + 1 ) / 2, 0, size.x, size.y );
}

void Hmd::enableEye( ci::vr::Eye eye, ci::vr::CoordSys eyeMatrixMode )
{
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
			auto area = ci::Area( viewport.first.x, viewport.first.y, viewport.first.x + viewport.second.x, viewport.first.y + viewport.second.y );
			float width = area.getWidth();
			float height = area.getHeight();
			float aspect = width / height;
			ci::mat4 mat = glm::perspectiveFov( toRadians( getFullFov() / aspect ), width, height, mNearClip, mFarClip );
			mHmdCamera.setProjectionMatrix( mat );
			ci::gl::clear( mClearColor );
		}
		break;
	}

	setMatricesEye( eye, eyeMatrixMode );
}

void Hmd::calculateOriginMatrix(){
	// Rotation matrix
	const ci::mat4& hmdDeviceToTrackingMat = mContext->getDeviceToTrackingMatrix( ::vr::k_unTrackedDeviceIndex_Hmd );
	ci::vec3 p0 = ci::vec3( hmdDeviceToTrackingMat * ci::vec4( 0, 0, 0, 1 ) );
	ci::vec3 p1 = ci::vec3( hmdDeviceToTrackingMat * ci::vec4( 0, 0, -1, 1 ) );
	ci::vec3 dir = p1 - p0;
	ci::vec3 v0 = ci::vec3( 0, 0, -1 );
	ci::vec3 v1 = ci::normalize( ci::vec3( dir.x, 0, dir.z ) );
	ci::quat q = ci::quat( v0, v1 );
	ci::mat4 rotationMatrix = glm::mat4_cast( q );
	// Position matrix
	//float dist = getSessionOptions().getOriginOffset();
	//ci::vec3 offset = p0 + v1*dist;
	//ci::mat4 positionMatrix = ci::translate( offset );
	const ci::vec3& offset = getSessionOptions().getOriginOffset();
	ci::vec3 w = v1;
	ci::vec3 v = ci::vec3( 0, 1, 0 );
	ci::vec3 u = ci::cross( w, v );
	ci::mat4 positionMatrix = ci::translate( p0 + ( offset.x * u ) + ( offset.y * v ) + ( -offset.z * w ) );

	switch( getSessionOptions().getOriginMode() ) {
		case ci::vr::ORIGIN_MODE_OFFSETTED: {
			// Rotation matrix
			rotationMatrix = ci::mat4();
			// Position matrix
			mOriginPosition = offset;
			positionMatrix = ci::translate( mOriginPosition );
		}
		break;

		case ci::vr::ORIGIN_MODE_HMD_OFFSETTED: {
			// Rotation matrix
			rotationMatrix = ci::mat4();
			// Position matrix
			mOriginPosition = ci::vec3( p0.z ) + offset;
			positionMatrix = ci::translate( mOriginPosition );
		}
		break;

		case ci::vr::ORIGIN_MODE_HMD_ORIENTED: {
			// Uses the current rotationMatrix and positionMatrix values.
		}
		break;

		default: {
			rotationMatrix = ci::mat4();
			positionMatrix = ci::mat4();
		}
		break;
	}

	// Compose origin matrix
	mOriginMatrix = positionMatrix*rotationMatrix;
	mInverseOriginMatrix = glm::affineInverse( mOriginMatrix );
}

void Hmd::calculateInputRay()
{
	// Ray components
	ci::mat4 deviceToTrackingMatrix = mContext->getDeviceToTrackingMatrix( ::vr::k_unTrackedDeviceIndex_Hmd );
	ci::mat4 coordSysMatrix = mInverseLookMatrix * mInverseOriginMatrix * deviceToTrackingMatrix;
	ci::vec3 p0 = ci::vec3( coordSysMatrix * ci::vec4( 0, 0, 0, 1 ) );
	ci::vec3 dir = ci::vec3( coordSysMatrix * ci::vec4( 0, 0, -1, 0 ) );
	// Input ray
	mInputRay = ci::Ray( p0, dir );
}

void Hmd::drawMirroredImpl( const ci::Rectf& r )
{
	const uint32_t kTexUnit = 0;

	ci::gl::ScopedDepthTest scopedDepthTest( false );
	ci::gl::ScopedModelMatrix scopedModelMatrix;
	ci::gl::ScopedColor scopedColor( 1, 1, 1 );

	switch( mMirrorMode ) {
		// Default to stereo mirroring
		default:
		case Hmd::MirrorMode::MIRROR_MODE_STEREO: {
			ci::gl::ScopedGlslProg scopedShader( mDistortionShader );

			float w = r.getWidth() / 2.0f;
			float h = r.getHeight() / 2.0f;

			// Offset and the scale to fit the rect
			ci::mat4 m = ci::mat4();
			m[0][0] =  w;
			m[1][1] = -h;
			m[3][0] =  w + r.x1;
			m[3][1] =  h + r.y1;
			ci::gl::multModelMatrix( m );

			// Render left eye
			{
				auto resolvedTex = mRenderTargetLeft->getColorTexture();
				resolvedTex->bind( kTexUnit );
				mDistortionShader->uniform( "uTex0", kTexUnit );
				mDistortionBatch->draw( 0, mDistortionIndexCount / 2 );
				resolvedTex->unbind( kTexUnit );
			}

			// Render right eye
			{
				auto resolvedTex = mRenderTargetRight->getColorTexture();
				resolvedTex->bind( kTexUnit );
				mDistortionShader->uniform( "uTex0", kTexUnit );
				mDistortionBatch->draw( mDistortionIndexCount / 2, mDistortionIndexCount / 2 );
				resolvedTex->unbind( kTexUnit );
			}
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

void Hmd::drawDebugInfo()
{
}

void Hmd::activateRenderModel( ::vr::TrackedDeviceIndex_t trackedDeviceIndex )
{
	// Bail if there's an existing model for trackedDeviceIndex.
	if( mRenderModels[trackedDeviceIndex] ) {
		return;
	}

	ci::vr::psvr::DeviceManager* deviceManager = mContext->getDeviceManager();
	std::string renderModelName = ci::vr::psvr::getTrackedDeviceString( mVrSystem, trackedDeviceIndex, ::vr::Prop_RenderModelName_String );
	if( ! renderModelName.empty() ) {
		RenderModelDataRef renderModelData = deviceManager->getRenderModelData( renderModelName );
		if( renderModelData ) {
			RenderModelRef renderModel = RenderModel::create( renderModelData, mRenderModelShader );
			mRenderModels[trackedDeviceIndex] = renderModel;
		}

		CI_LOG_I( "...added: " << renderModelName );
	}
}

}}} // namespace cinder::vr::psvr

#endif // defined( CINDER_VR_ENABLE_PSVR )