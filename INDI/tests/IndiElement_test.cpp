/** \file IndiElement_test.cpp
  * \brief Catch2 tests for pcf::IndiElement (INDI/libcommon/IndiElement.cpp/.hpp).
  *
  * Exercises every constructor, accessor, mutator, conversion, validity check,
  * and enum<->string converter of the IndiElement value class.
  */
#include "../../tests/catch2/catch.hpp"

#include <sstream>

#include "../libcommon/IndiElement.hpp"

using pcf::IndiElement;

namespace IndiElement_test
{

SCENARIO( "IndiElement construction, assignment, and comparison", "[IndiElement]" )
{
   GIVEN( "the various constructors" )
   {
      WHEN( "constructing each way" )
      {
         IndiElement e0;
         REQUIRE( e0.getName() == "" );

         IndiElement e1( "el1" );
         REQUIRE( e1.getName() == "el1" );

         IndiElement e2( "el2", std::string( "val2" ) );
         REQUIRE( e2.getValue() == "val2" );

         IndiElement e3( "el3", "val3" );
         REQUIRE( e3.get() == "val3" );

         IndiElement e4( "el4", IndiElement::Busy );
         REQUIRE( e4.getLightState() == IndiElement::Busy );

         IndiElement e5( "el5", IndiElement::On );
         REQUIRE( e5.getSwitchState() == IndiElement::On );

         IndiElement e6( "el6", 3.5 ); // templated numeric constructor
         REQUIRE( e6.get<double>() == 3.5 );

         IndiElement eCopy( e2 );
         REQUIRE( eCopy == e2 );
      }
   }

   GIVEN( "elements to assign and compare" )
   {
      WHEN( "assigning and comparing" )
      {
         IndiElement a( "one", "1" );
         a.setLabel( "label" );
         a.setFormat( "%d" );
         a.setMax( "10" );
         a.setMin( "0" );
         a.setStep( "1" );
         a.setSize( "4" );
         a.setLightState( IndiElement::Ok );
         a.setSwitchState( IndiElement::Off );

         IndiElement b;
         b = a;
         REQUIRE( b == a );
         REQUIRE( b == b ); // self-comparison branch
         b = b;             // self-assignment branch
         REQUIRE( b == a );

         IndiElement c( "other", "2" );
         REQUIRE( !( c == a ) );

         // templated operator= sets the value from a numeric
         c = 42;
         REQUIRE( c.get<int>() == 42 );

         // enum operator= overloads
         c = IndiElement::Alert;
         REQUIRE( c.getLightState() == IndiElement::Alert );
         c = IndiElement::On;
         REQUIRE( c.getSwitchState() == IndiElement::On );
      }
   }
}

SCENARIO( "IndiElement accessors, mutators, and validity checks", "[IndiElement]" )
{
   GIVEN( "an element with every attribute set" )
   {
      IndiElement el( "elname", "37" );
      el.setLabel( "A label" );
      el.setFormat( "%0.2f" );
      // explicit std::string arguments select the string overloads of these setters
      // (bare literals would select the templated ones, tested separately below)
      el.setMax( std::string( "100" ) );
      el.setMin( std::string( "-100" ) );
      el.setStep( std::string( "0.5" ) );
      el.setSize( std::string( "8" ) );
      el.setLightState( IndiElement::Idle );
      el.setSwitchState( IndiElement::Off );

      WHEN( "reading every attribute back" )
      {
         REQUIRE( el.getName() == "elname" );
         REQUIRE( el.getLabel() == "A label" );
         REQUIRE( el.getFormat() == "%0.2f" );
         REQUIRE( el.getMax() == "100" );
         REQUIRE( el.getMin() == "-100" );
         REQUIRE( el.getStep() == "0.5" );
         REQUIRE( el.getSize() == "8" );
         REQUIRE( el.getLightState() == IndiElement::Idle );
         REQUIRE( el.getSwitchState() == IndiElement::Off );
         REQUIRE( el.getValue() == "37" );
         REQUIRE( el.get() == "37" );
         REQUIRE( el.getValue<int>() == 37 );
         REQUIRE( el.get<int>() == 37 );

         // enum conversion operators
         IndiElement::LightStateType ls = el;
         REQUIRE( ls == IndiElement::Idle );
         IndiElement::SwitchStateType ss = el;
         REQUIRE( ss == IndiElement::Off );

         // char-buffer value accessor
         char cbuf[16];
         unsigned int sz = sizeof( cbuf );
         el.getValue( cbuf, sz );
         REQUIRE( sz == 2 );
         REQUIRE( std::string( cbuf, sz ) == "37" );

         REQUIRE( el.isNumeric() == false ); // "37" streams fully; stream not good() after eof
      }

      WHEN( "checking validity flags on a fully-populated element" )
      {
         REQUIRE( el.hasValidName() );
         REQUIRE( el.hasValidLabel() );
         REQUIRE( el.hasValidFormat() );
         REQUIRE( el.hasValidMax() );
         REQUIRE( el.hasValidMin() );
         REQUIRE( el.hasValidStep() );
         REQUIRE( el.hasValidSize() );
         REQUIRE( el.hasValidLightState() );
         REQUIRE( el.hasValidSwitchState() );
         REQUIRE( el.hasValidValue() );
      }

      WHEN( "clear() resets everything" )
      {
         el.clear();
         REQUIRE( el.getName() == "" );
         REQUIRE( el.getFormat() == "%g" );
         REQUIRE( el.getMax() == "0" );
         REQUIRE( el.getMin() == "0" );
         REQUIRE( el.getSize() == "0" );
         REQUIRE( el.getStep() == "0" );
         REQUIRE( !el.hasValidName() );
         REQUIRE( !el.hasValidLabel() );
         REQUIRE( !el.hasValidLightState() );
         REQUIRE( !el.hasValidSwitchState() );
         REQUIRE( !el.hasValidValue() );
      }

      WHEN( "createString renders all attributes" )
      {
         std::string s = el.createString();
         REQUIRE( s.find( "\"name\" : \"elname\"" ) != std::string::npos );
         REQUIRE( s.find( "\"value\" : \"37\"" ) != std::string::npos );
         REQUIRE( s.find( "\"lightstate\" : \"Idle\"" ) != std::string::npos );
      }
   }

   GIVEN( "the mutator overloads" )
   {
      IndiElement el( "m" );

      WHEN( "setting values in every form" )
      {
         el.setValue( std::string( "strval" ) );
         REQUIRE( el.getValue() == "strval" );

         el.setValue( "charval", 4 );
         REQUIRE( el.getValue() == "char" );

         el.setValue( 2.25 ); // templated
         REQUIRE( el.getValue<double>() == 2.25 );

         el.set( 7 ); // templated set
         REQUIRE( el.get<int>() == 7 );

         el.setName( "renamed" );
         REQUIRE( el.getName() == "renamed" );

         // templated numeric attribute setters
         el.setMax( 99 );
         REQUIRE( el.getMax() == "99" );
         el.setMin( -5 );
         REQUIRE( el.getMin() == "-5" );
         el.setSize( 16 );
         REQUIRE( el.getSize() == "16" );
         el.setStep( 2 );
         REQUIRE( el.getStep() == "2" );
      }
   }
}

SCENARIO( "IndiElement enum and type converters", "[IndiElement]" )
{
   GIVEN( "light states, switch states, and element types" )
   {
      WHEN( "converting light states both ways" )
      {
         REQUIRE( IndiElement::getLightStateType( "Idle" ) == IndiElement::Idle );
         REQUIRE( IndiElement::getLightStateType( "Ok" ) == IndiElement::Ok );
         REQUIRE( IndiElement::getLightStateType( "Busy" ) == IndiElement::Busy );
         REQUIRE( IndiElement::getLightStateType( "Alert" ) == IndiElement::Alert );
         REQUIRE( IndiElement::getLightStateType( "junk" ) == IndiElement::UnknownLightState );

         REQUIRE( IndiElement::getLightStateString( IndiElement::UnknownLightState ) == "" );
         REQUIRE( IndiElement::getLightStateString( IndiElement::Idle ) == "Idle" );
         REQUIRE( IndiElement::getLightStateString( IndiElement::Ok ) == "Ok" );
         REQUIRE( IndiElement::getLightStateString( IndiElement::Busy ) == "Busy" );
         REQUIRE( IndiElement::getLightStateString( IndiElement::Alert ) == "Alert" );
      }

      WHEN( "converting switch states both ways" )
      {
         REQUIRE( IndiElement::getSwitchStateType( "Off" ) == IndiElement::Off );
         REQUIRE( IndiElement::getSwitchStateType( "On" ) == IndiElement::On );
         REQUIRE( IndiElement::getSwitchStateType( "junk" ) == IndiElement::UnknownSwitchState );

         REQUIRE( IndiElement::getSwitchStateString( IndiElement::UnknownSwitchState ) == "" );
         REQUIRE( IndiElement::getSwitchStateString( IndiElement::Off ) == "Off" );
         REQUIRE( IndiElement::getSwitchStateString( IndiElement::On ) == "On" );
      }

      WHEN( "converting element types both ways" )
      {
         REQUIRE( IndiElement::convertTypeToString( IndiElement::UnknownType ) == "" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::DefBLOB ) == "defBLOB" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::DefLight ) == "defLight" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::DefNumber ) == "defNumber" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::DefSwitch ) == "defSwitch" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::DefText ) == "defText" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::OneBLOB ) == "oneBLOB" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::OneLight ) == "oneLight" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::OneNumber ) == "oneNumber" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::OneSwitch ) == "oneSwitch" );
         REQUIRE( IndiElement::convertTypeToString( IndiElement::OneText ) == "oneText" );

         REQUIRE( IndiElement::convertStringToType( "defBLOB" ) == IndiElement::DefBLOB );
         REQUIRE( IndiElement::convertStringToType( "defLight" ) == IndiElement::DefLight );
         REQUIRE( IndiElement::convertStringToType( "defNumber" ) == IndiElement::DefNumber );
         REQUIRE( IndiElement::convertStringToType( "defSwitch" ) == IndiElement::DefSwitch );
         REQUIRE( IndiElement::convertStringToType( "defText" ) == IndiElement::DefText );
         REQUIRE( IndiElement::convertStringToType( "oneBLOB" ) == IndiElement::OneBLOB );
         REQUIRE( IndiElement::convertStringToType( "oneLight" ) == IndiElement::OneLight );
         REQUIRE( IndiElement::convertStringToType( "oneNumber" ) == IndiElement::OneNumber );
         REQUIRE( IndiElement::convertStringToType( "oneSwitch" ) == IndiElement::OneSwitch );
         REQUIRE( IndiElement::convertStringToType( "oneText" ) == IndiElement::OneText );
         REQUIRE( IndiElement::convertStringToType( "nonsense" ) == IndiElement::UnknownType );
      }
   }
}

} //namespace IndiElement_test
