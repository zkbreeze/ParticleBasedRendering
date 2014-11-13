//
//	main.cpp
//
//
//	Created by Kun Zhao on 2014/08/29.
//
//

#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/UnstructuredVolumeImporter>
#include <kvs/TransferFunction>
#include <kvs/ColorMap>
#include <kvs/OpacityMap>
#include <kvs/glut/TransferFunctionEditor>
#include <kvs/RendererManager>
#include <kvs/CommandLine>
#include "CellByCellUniformSampling2D.h"
#include "ParticleBasedRendererGLSL2D.h"
#include "CellByCellUniformSampling.h"
#include "ParticleBasedRendererGLSL.h"

#include "JetImporter.h"

#include "KVSMLObjectKunPoint.h"
#include "PointImporter.h"
#include "PointExporter.h"

#include "load_ucd.h"
#include "PointObject.h"
#include "PRTObject.h"

#include <kvs/Timer>
#include <kvs/ColorImage>
#include "SnapKey.h"
#include "FPS.h"
#include "TsunamiObject.h"

#include "loaducd.h"

#define TETRA 4
#define PRISM 6


namespace
{
	bool ShadingFlag = true;
	float base_opacity = 0.2;
}

kun::PointObject* CreatePointObject( kvs::VolumeObjectBase* volume, size_t subpixel_level, kvs::TransferFunction tfunc, bool shuffle = 0, bool use_kun_sampling_step = 0 )
{
	kvs::Timer time;
	time.start();
	kun::CellByCellUniformSampling* sampler = new kun::CellByCellUniformSampling();
	sampler->setSubpixelLevel( subpixel_level );
	sampler->setSamplingStep( 0.5 );
	sampler->setTransferFunction( tfunc );
	sampler->setObjectDepth( 0.0 );

	if( shuffle ) sampler->setShuffleParticles();
	if( use_kun_sampling_step ) sampler->setKunSamplingStep();
	kun::PointObject* point = sampler->exec( volume );
	time.stop();
	std::cout << "Particle generation time: " << time.msec() << " msec." << std::endl;
	std::cout << "Particle number: " << point->numberOfVertices() << std::endl;
	return point;
}

kun::PointObject* CreatePointObject2D( kvs::UnstructuredVolumeObject* volume1, kvs::UnstructuredVolumeObject* volume2, size_t subpixel_level, kvs::TransferFunction tfunc, bool shuffle = 0, bool use_kun_sampling_step = 0 )
{
	kvs::Timer time;
	time.start();
	kun::CellByCellUniformSampling2D* sampler = new kun::CellByCellUniformSampling2D();
	sampler->setSubpixelLevel( subpixel_level );
	sampler->setSamplingStep( 0.5 );
	sampler->setTransferFunction( tfunc );
	sampler->setObjectDepth( 0.0 );
	sampler->setSecondVolume( volume2 );
	if( shuffle ) sampler->setShuffleParticles();
	if( use_kun_sampling_step ) sampler->setKunSamplingStep();
	kun::PointObject* point = sampler->exec( volume1);
	time.stop();
	std::cout << "Particle generation time: " << time.msec() << " msec." << std::endl;
	std::cout << "Particle number: " << point->numberOfVertices() << std::endl;
	return point;
}

int main( int argc, char** argv )
{
	kvs::glut::Application app( argc, argv );
	kvs::glut::Screen screen( &app );

	kvs::CommandLine param( argc, argv );
	param.addHelpOption();

	kun::SnapKey key;
	screen.addEvent( &key );
	kun::FPS fps;
	screen.addEvent( &fps );

	// Parameter
	param.addOption( "s", "Subpixel Level", 1, false );
	param.addOption( "o", "base opacity", 1, false );
	param.addOption( "nos", "No Shading", 0, false );
	param.addOption( "trans", "set initial transfer function", 1, false );
	param.addOption( "sg", "shuffle the generated particles", 0, false );
	param.addOption( "rep", "repetition level", 1, false );
	param.addOption( "low_rep", "low repeptition lelve", 1, false ),

	// Data input
	param.addOption( "j", "Jet Data Filename", 1, false );
	param.addOption( "u", "Unstructured Volume Data Filename", 1, false );
	param.addOption( "k", "Structured Volume Data Filename", 1, false );
	param.addOption( "point", "KVSML Point Data Filename", 1, false );
	param.addOption( "prt", "PRT object", 1, false );
	param.addOption( "tetra", "Input tetra Volume Object (binary_ucd)", 1, false );
	param.addOption( "prism", "Input prism Volume Object (binary_ucd)", 1, false );
	param.addOption( "both", "Input prism and tetra Volume Object (binary_ucd)", 1, false );
	param.addOption( "u-prism-ball", "KVSML Data Filename. around ball only", 1, false );
	param.addOption( "prism-ball", "Input prism Volume Object (binary_ucd) and output the cut ball", 1, false );
	param.addOption( "tsunami", "Input tsunami particle data", 1, false );

	// Output
	param.addOption( "writepoint", "Output point object filename", 1, false );
	if ( !param.parse() ) return 1;

	size_t subpixel_level = 1;
	kvs::TransferFunction tfunc_base( 256 );
	bool shuffle_generated_particles = false;

	// Parameter
	if( param.hasOption( "s" ) ) subpixel_level = param.optionValue<size_t>( "s" ) ;
	if( param.hasOption( "o" ) ) base_opacity = param.optionValue<float>( "o" );
	if( param.hasOption( "nos" ) ) ShadingFlag = false;
	if( param.hasOption( "sg" ) ) shuffle_generated_particles = true;

	if( param.hasOption( "trans" ) ) tfunc_base = kvs::TransferFunction( param.optionValue<std::string>( "trans" ) );
	else
	{
		kvs::OpacityMap omap( 256 );
		omap.addPoint( 0, base_opacity );
		omap.addPoint( 255, base_opacity );
		omap.create();
		tfunc_base.setOpacityMap( omap );
	}

	kun::PointObject* point = NULL;

		// Data Input
	if( param.hasOption( "point" ) )
	{
		if( param.hasOption( "low_rep" ) )
			point = new kun::PointImporter( param.optionValue<std::string>( "point" ), (float)param.optionValue<size_t>( "low_rep" ) / param.optionValue<size_t>( "rep" ) );
		else
			point = new kun::PointImporter( param.optionValue<std::string>( "point" ) );
	}
	else if( param.hasOption( "prt" ) )
	{
		kun::PRTObject* prt = new kun::PRTObject( param.optionValue<std::string>( "prt" ) );
		point = prt->toKUNPointObject();
		std::cout << point->numberOfVertices() << std::endl;
	}

	else if( param.hasOption( "u" ) )
	{
		kvs::UnstructuredVolumeObject* volume = new kvs::UnstructuredVolumeImporter( param.optionValue<std::string>( "u" ) );
		point = CreatePointObject( volume, subpixel_level, tfunc_base, shuffle_generated_particles );
	}
	else if( param.hasOption( "k" ) )
	{
		kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeImporter( param.optionValue<std::string>( "k" ) );
		point = CreatePointObject( volume, subpixel_level, tfunc_base, shuffle_generated_particles );
	}
	else if( param.hasOption( "j" ) )
	{
		kvs::StructuredVolumeObject* volume = new kun::JetImporter( param.optionValue<std::string>( "j" )	);
		point = CreatePointObject( volume, subpixel_level, tfunc_base, shuffle_generated_particles );
	}
	else if( param.hasOption( "u-prism-ball" ) )
	{
		kvs::UnstructuredVolumeObject* volume = new kvs::UnstructuredVolumeImporter( param.optionValue<std::string>( "u-prism-ball" ) );
		volume->setMinMaxObjectCoords( kvs::Vec3( -30, -30, -30 ), kvs::Vec3( 30, 30, 30 ) );
		volume->setMinMaxExternalCoords( kvs::Vec3( -30, -30, -30 ), kvs::Vec3( 30, 30, 30 ) );
		point = CreatePointObject( volume, subpixel_level, tfunc_base, shuffle_generated_particles );
	}
	else if( param.hasOption( "prism-ball" ) )
	{
		kvs::UnstructuredVolumeObject* volume = CreateCutPrismObject( param.optionValue<std::string>( "prism-ball" ).c_str() );
		volume->setMinMaxObjectCoords( kvs::Vec3( -30, -30, -30 ), kvs::Vec3( 30, 30, 30 ) );
		volume->setMinMaxExternalCoords( kvs::Vec3( -30, -30, -30 ), kvs::Vec3( 30, 30, 30 ) );
		point = CreatePointObject( volume, subpixel_level, tfunc_base, shuffle_generated_particles );
	}
	else if( param.hasOption( "tetra" ) )
	{
		kvs::UnstructuredVolumeObject* volume1 = new takami::LoadUcd( param.optionValue<std::string>( "tetra" ).c_str() ,TETRA, 3 );
		kvs::UnstructuredVolumeObject* volume2 = new takami::LoadUcd( param.optionValue<std::string>( "tetra" ).c_str() ,TETRA, 1 );
		point = CreatePointObject2D( volume1, volume2, subpixel_level, tfunc_base, shuffle_generated_particles );
	}
	else if( param.hasOption( "prism" ) )
	{
		kvs::UnstructuredVolumeObject* volume = CreateUnstructuredVolumeObject( param.optionValue<std::string>( "prism" ).c_str() ,PRISM );
		point = CreatePointObject( volume, subpixel_level, tfunc_base, shuffle_generated_particles );
	}
	else if( param.hasOption( "both" ) )
	{
		kvs::UnstructuredVolumeObject* volume_p1 = new takami::LoadUcd( param.optionValue<std::string>( "both" ).c_str() ,TETRA, 3 );
		kvs::UnstructuredVolumeObject* volume2_p1 = new takami::LoadUcd( param.optionValue<std::string>( "both" ).c_str() ,TETRA, 1 );
		point = CreatePointObject2D( volume_p1, volume2_p1, subpixel_level, tfunc_base, shuffle_generated_particles );

		kvs::UnstructuredVolumeObject* volume_p2 = new takami::LoadUcd( param.optionValue<std::string>( "both" ).c_str(), PRISM, 3 );
		kvs::UnstructuredVolumeObject* volume2_p2 = new takami::LoadUcd( param.optionValue<std::string>( "both" ).c_str(), PRISM, 1 );
		kun::PointObject* point2 = CreatePointObject2D( volume_p2, volume2_p2, subpixel_level, tfunc_base, shuffle_generated_particles );

		point->add( *point2 );
	}
	else if( param.hasOption( "tsunami" ) )
	{
		kun::TsunamiObject* tsunami = new kun::TsunamiObject( param.optionValue<std::string>( "tsunami" ) );
		point = tsunami->toKUNPointObject( 1 );
	}
	else 
	{
		std::cerr << "No input file !!!!!!!" << std::endl;
		exit(0);
	}

	point->print( std::cout );


	// Data output. If there is output, the program will be ended
	if ( param.hasOption( "writepoint" ) )
	{
		point->shuffle();

		kun::KVSMLObjectKunPoint* kvsml = new kun::PointExporter<kun::KVSMLObjectKunPoint>( point );
		kvsml -> setWritingDataType(kvs::kvsml::ExternalBinary);
		std::string point_filename = param.optionValue<std::string>( "writepoint" );
		kvsml -> write( point_filename.c_str());
		std::cout << "Finish writing " << point_filename << std::endl;

		exit(0);
	}

	// Rendering
	kun::ParticleBasedRenderer2D* renderer = new kun::ParticleBasedRenderer2D();
	if( ShadingFlag == false) renderer->disableShading();

    // set the 2d transfer function
    size_t width = 256;
    size_t height = 256;
    float* tfunc2d = new float[width * height * 4];
    for ( size_t j = 0; j < height; j++ )
        for ( size_t i = 0; i < width; i++ )
        {
            int index = ( i + j * width ) * 4;
            tfunc2d[index] = 1 - (float)i / width; // red
            tfunc2d[index + 1] = (float)j / height; // green
            tfunc2d[index + 2] = 0.1 ;  // blue
            tfunc2d[index + 3] = ( (float) index / 4 ) / ( width * height ); //alpha
//            *(tfunc2d++) = 0.01;
        }

	renderer->set2DTransferFunction( tfunc2d, width, height );
	if( param.hasOption( "low_rep" ) )
		renderer->setRepetitionLevel( param.optionValue<size_t>( "low_rep" ) );
	else if( param.hasOption( "rep" ) )
		renderer->setRepetitionLevel( param.optionValue<size_t>( "rep" ) );
	else
		renderer->setRepetitionLevel( subpixel_level * subpixel_level );

	renderer->setBaseOpacity( ::base_opacity );
	screen.registerObject( point, renderer );
	screen.setBackgroundColor( kvs::RGBColor( 255, 255, 255 ) );
	screen.show();

	return app.run();
}
