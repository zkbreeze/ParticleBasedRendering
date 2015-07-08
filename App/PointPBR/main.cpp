//
//  main.cpp
//
//
//  Created by Kun Zhao on 2015-07-01 13:56:41.
//
//

#include "DensityCalculator.h"
#include "WriteStructuredVolume.h"
#include "TsunamiObject.h"
#include "PointImporter.h"
#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/RayCastingRenderer>
#include <kvs/CommandLine>
#include <kvs/Timer>
#include "ParticleBasedRendererGLSLPoint.h"
#include <kvs/TransferFunction>
#include <kvs/glut/TransferFunctionEditor>
#include <kvs/RendererManager>

namespace
{
	kvs::StructuredVolumeObject* density_volume = NULL;	
	int repetition_level = 36;
}

class TransferFunctionEditor : public kvs::glut::TransferFunctionEditor
{

public:
	TransferFunctionEditor( kvs::ScreenBase* screen ) :
	kvs::glut::TransferFunctionEditor( screen )
	{
	}

	void apply( void )
	{
		kvs::glut::Screen* glut_screen = static_cast<kvs::glut::Screen*>( screen() );
		kvs::RendererBase* r = glut_screen->scene()->rendererManager()->renderer();
		kun::ParticleBasedRendererPoint* renderer = static_cast<kun::ParticleBasedRendererPoint*>( r );
		renderer->setShader( kvs::Shader::Phong( 0.6, 0.4, 0, 1 ) );
		renderer->setDensityVolume( ::density_volume );
		renderer->setRepetitionLevel( ::repetition_level );
		renderer->setTransferFunction( transferFunction() );
		std::cout << "TF adjust time: " << renderer->timer().msec() << std::endl;
		screen()->redraw();
	}
};

int main( int argc, char** argv )
{
	kvs::glut::Application app( argc, argv );
	kvs::glut::Screen screen( &app );

	kvs::CommandLine param( argc, argv );
	param.addHelpOption();
	param.addOption( "m", "Assign max grid number", 1, false );
	param.addOption( "f", "Input KUN point file name", 1, false );
	param.addOption( "t", "Input tsunami file name", 1, false );
	param.addOption( "rep", "Input repetition level", 1, false );

	if( !param.parse() ) return 1;

	kun::PointObject* point = NULL;
	if( param.hasOption( "f" ) ) point = new kun::PointImporter( param.optionValue<std::string>( "f" ) );
	if( param.hasOption( "t" ) ) 
	{
		kun::TsunamiObject* tsunami = new kun::TsunamiObject( param.optionValue<std::string>( "t" ) );
		point = tsunami->toKUNPointObject( 1 );		
	}

	if( param.hasOption( "rep" ) ) ::repetition_level = param.optionValue<int>( "rep" );

	kvs::Timer time;
	time.start();
	kun::DensityCalculator* calculator = new kun::DensityCalculator( point );
	if( param.hasOption( "m" ) ) calculator->setMaxGrid( param.optionValue<int>( "m") );
	density_volume = calculator->outputDensityVolume();
	time.stop();
	std::cout << "Density calculation time: " << time.msec() << "msec" << std::endl;

	kvs::TransferFunction tfunc( 256 );

	kun::ParticleBasedRendererPoint* renderer = new kun::ParticleBasedRendererPoint();
	renderer->setShader( kvs::Shader::Phong( 0.6, 0.4, 0, 1 ) );
	renderer->setDensityVolume( ::density_volume );
	renderer->setRepetitionLevel( ::repetition_level );
	renderer->setTransferFunction( tfunc );

	screen.registerObject( point, renderer );
	screen.setBackgroundColor( kvs::RGBColor( 0, 0, 0 ) );
	screen.show();

	// Set the transfer function editor
	kvs::StructuredVolumeObject* object = new kvs::StructuredVolumeObject();
	object->setGridType( kvs::StructuredVolumeObject::Uniform );
	object->setVeclen( 1 );
	object->setResolution( kvs::Vector3ui( 1, 1, point->numberOfVertices() ) );
	object->setValues( point->values() );
	object->updateMinMaxValues();

	object->print( std::cout );

	TransferFunctionEditor editor( &screen );
	editor.setVolumeObject( object );
	editor.setTransferFunction( tfunc );
	editor.show();

	return app.run();
}