/*	===============
 *	Vertice/Port.cc
 *	===============
 */

#include "Vertice/Port.hh"

// Standard C
#include <stddef.h>

// Standard C/C++
#include <cmath>

// Standard C++
#include <functional>
#include <string>

// Nitrogen
#include "Nitrogen/Sound.h"

// Vectoria
#include "Vectoria/Clipping3D.hh"
#include "Vectoria/LinearAlgebra3D.hh"
#include "Vectoria/Polygon3D.hh"
#include "Vectoria/ViewFrustum.hh"

// Vertice
#include "Vertice/Objects.hh"
#include "Vertice/Model.hh"


namespace Vertice
{
	
	namespace N = Nitrogen;
	
	/*
	RGB::Color Tile::Color( const Point2D& pt ) const
	{
		RGB::Color color = Color();
		const double scale = 20;
		
		int x = std::floor( pt.x * scale );
		int y = std::floor( pt.y * scale );
		
		if ( std::abs( x % 2 )  ==  std::abs( y % 2 ) )
		{
			color *= 0.9;
		}
		
		return color;
	}
	*/
	
	void Port::SendCameraCommand( std::size_t contextIndex, short cmd )
	{
		double incrMove = 0.5;
		double incrRotate = 5.0 * 3.14159 / 180;
		
		if ( contextIndex == 0 )
		{
			N::SysBeep();
			return;
		}
		
		Moveable& target = itsScene.GetContext( contextIndex );
		
		switch ( cmd )
		{
			case cmdMoveLeft:
				target.ContextTranslate(-incrMove, 0, 0);
				break;
			case cmdMoveRight:
				target.ContextTranslate(incrMove, 0, 0);
				break;
			case cmdMoveForward:
				target.ContextTranslate(0, incrMove, 0);
				break;
			case cmdMoveBackward:
				target.ContextTranslate(0, -incrMove, 0);
				break;
			case cmdMoveUp:
				target.ContextTranslate(0, 0, incrMove);
				break;
			case cmdMoveDown:
				target.ContextTranslate(0, 0, -incrMove);
				break;
			case cmdYawLeft:
				target.ContextYaw(incrRotate);
				break;
			case cmdYawRight:
				target.ContextYaw(-incrRotate);
				break;
			case cmdPitchUp:
				target.ContextPitch(incrRotate);
				break;
			case cmdPitchDown:
				target.ContextPitch(-incrRotate);
				break;
			case cmdRollLeft:
				target.ContextRoll(-incrRotate);
				break;
			case cmdRollRight:
				target.ContextRoll(incrRotate);
				break;
			case cmdExpand:
				target.ContextScale(1.05);
				break;
			case cmdContract:
				target.ContextScale(1.0 / 1.05);
				break;
			case cmdLevelPitch:
				//xform = target.GetTransform();
				break;
			case cmdLevelRoll:
				//xform = target.GetTransform();
				break;
			case cmdGroundHeight:
				{
					V::Point3D::Type loc = Transformation( V::Point3D::Make( 0, 0, 0 ), target.itsTransform );
					
					V::Vector3D::Type vec = V::Vector3D::Make( 0, 0, -loc[ Z ] /* + target.groundHeight*/ );
					
					target.LocalTranslate( vec );
				}
				break;
			case cmdProjectionMode:
				//mCamera.fishEyeMode = !mCamera.fishEyeMode;
				break;
			default:
				break;
		}
	}
	
	
	class GetMeshModels
	{
		private:
			const Scene&  itsScene;
			Frame&        itsResultFrame;
			
			std::vector< V::XMatrix > itsTransformStack;
		
		public:
			GetMeshModels( const Scene&  scene,
			               Frame&        outFrame ) : itsScene      ( scene    ),
			                                          itsResultFrame( outFrame )
			{
				itsTransformStack.push_back( V::XMatrix( V::IdentityMatrix() ) );
			}
			
			void operator()( std::size_t index = 0 );
	};
	
	
	void GetMeshModels::operator()( std::size_t index )
	{
		const Context& context = itsScene.GetContext( index );
		
		itsTransformStack.push_back( Compose( itsTransformStack.back(), context.itsTransform ) );
		
		MeshModel model = context;
		
		model.Transform( V::Transformer< V::Point3D::Type >( itsTransformStack.back() ) );
		
		itsResultFrame.AddModel( model );
		
		const std::vector< std::size_t >& subs = context.Subcontexts();
		
		std::for_each( subs.begin(), subs.end(), *this );
		
		itsTransformStack.pop_back();
	}
	
	
	static double FocalLength( V::Radians alpha )
	{
		double e = 1 / std::tan( alpha / 2.0 );
		return e;
	}
	
	static double sAspectRatio = 1;
	
	static V::Radians sHorizontalFieldOfViewAngle = V::Degrees( 60 );
	
	static double sFocalLength = FocalLength( sHorizontalFieldOfViewAngle );
	
	
	static V::Point3D::Type PerspectiveDivision( const V::Point3D::Type& pt )
	{
		return pt / -pt[ Z ] * sFocalLength;
	}
	
	
	void Port::MakeFrame( Frame& outFrame ) const
	{
		if ( itsScene.Cameras().empty() )  return;
		
		Camera& camera = itsScene.Cameras().front();
		
		const V::XMatrix& world2eye = camera.WorldToEyeTransform( itsScene );
		const V::XMatrix& eye2port  = camera.EyeToPortTransform();
		
		V::XMatrix world2port = Compose( world2eye, eye2port );
		
		V::XMatrix port2worldInverseTranspose = Transpose( world2port );
		
		V::Plane3D::Type nearPlane = port2worldInverseTranspose * V::NearPlane(   0.01 );
		V::Plane3D::Type farPlane  = port2worldInverseTranspose * V::FarPlane ( 100    );
		
		V::Plane3D::Type leftPlane  = port2worldInverseTranspose * V::LeftPlane (  sFocalLength );
		V::Plane3D::Type rightPlane = port2worldInverseTranspose * V::RightPlane(  sFocalLength );
		
		V::Plane3D::Type bottomPlane = port2worldInverseTranspose * V::BottomPlane( sFocalLength, sAspectRatio );
		V::Plane3D::Type topPlane    = port2worldInverseTranspose * V::TopPlane   ( sFocalLength, sAspectRatio );
		
		Frame newFrame;
		
		GetMeshModels( itsScene, newFrame )();
		
		typedef std::vector< MeshModel >::iterator ModelIter;
		
		std::vector< MeshModel >& models = newFrame.Models();
		
		V::Transformer< V::Point3D::Type > transformer( world2port );
		
		V::Point3D::Type eye = V::Point3D::Make( 0, 0, 0 );
		
		eye = camera.EyeToWorldTransform( itsScene ) * eye;
		
		for ( ModelIter it = models.begin();  it != models.end();  ++it )
		{
			it->CullBackfaces( eye );
			
			it->ClipAgainstPlane( nearPlane   );
			it->ClipAgainstPlane( farPlane    );
			it->ClipAgainstPlane( leftPlane   );
			it->ClipAgainstPlane( rightPlane  );
			it->ClipAgainstPlane( bottomPlane );
			it->ClipAgainstPlane( topPlane    );
			
			if ( it->Polygons().empty() )
			{
				it->Swap( models.back() );
				
				models.pop_back();
				
				--it;
			}
			else
			{
				it->CullDeadPoints();
			}
		}
		
		for ( ModelIter it = models.begin();  it != models.end();  ++it )
		{
			it->Transform( transformer );
		}
		
		newFrame.SortByDepth();
		
		outFrame.Swap( newFrame );
	}
	
	
	void Frame::SortByDepth()
	{
		std::sort( itsModels.begin(), itsModels.end() );
	}
	
	
	static V::Point2D::Type Point3DTo2D( const V::Point3D::Type& pt )
	{
		return V::Point2D::Make( pt[ X ], pt[ Y ] );
	}
	
	
	MeshModel* Frame::HitTest( const V::Point3D::Type& pt1 )
	{
		V::Point3D::Type pt0 = V::Point3D::Make( 0, 0, 0 );
		
		// The ray is inverted to face the same way as the face normal.
		V::Vector3D::Type ray = UnitLength( pt0 - pt1 );
		
		double minZ = 0;
		MeshModel* closestModel = NULL;
		const MeshPolygon* closestPoly = NULL;
		
		for ( std::vector< MeshModel >::iterator it = itsModels.begin();  it != itsModels.end();  ++it )
		{
			MeshModel& model = *it;
			const std::vector< MeshPolygon >& polygons = model.Polygons();
			
			typedef std::vector< MeshPolygon >::const_iterator PolygonIter;
			
			for ( PolygonIter it = polygons.begin();  it != polygons.end();  ++it )
			{
				const MeshPolygon& polygon = *it;
				
				const std::vector< unsigned >& offsets = polygon.Vertices();
				
				std::vector< V::Point3D::Type > points( offsets.size() );
				
				std::transform( offsets.begin(),
				                offsets.end(),
				                points.begin(),
				                model.Mesh() );
				
				V::Plane3D::Type plane = V::PlaneVector( points );
				
				std::transform( points.begin(),
				                points.end(),
				                points.begin(),
				                std::ptr_fun( PerspectiveDivision ) );
				
				V::Polygon2D poly2d;
				
				std::vector< V::Point2D::Type >& points2d( poly2d.Points() );
				
				points2d.resize( points.size() );
				
				std::transform( points.begin(),
				                points.end(),
				                points2d.begin(),
				                std::ptr_fun( Point3DTo2D ) );
				
				V::Point3D::Type sectPt = LinePlaneIntersection( ray, pt0, plane );
				
				double dist = Magnitude( sectPt - pt0 );
				
				V::Point2D::Type current_pixel_2d = Point3DTo2D( pt1 );
				
				if (    dist > 0
				     && (dist < minZ  ||  minZ == 0)
				     && poly2d.ContainsPoint( current_pixel_2d ) )
				{
					minZ = dist;
					
					closestPoly  = &*it;
					closestModel = &model;
				}
			}
		}
		
		return closestModel;
	}
	
}

