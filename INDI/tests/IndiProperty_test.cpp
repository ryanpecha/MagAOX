/** \file IndiProperty_test.cpp
  * \brief Catch2 tests for pcf::IndiProperty (INDI/libcommon/IndiProperty.cpp).
  *
  * Exercises every constructor, comparison, element operation, validity check,
  * accessor/mutator, and enum<->string converter.
  */
#include "../../tests/catch2/catch.hpp"

#include "../libcommon/IndiProperty.hpp"

using pcf::IndiElement;
using pcf::IndiProperty;

namespace IndiProperty_test
{

static IndiProperty makeProp()
{
   IndiProperty ip( IndiProperty::Number, "dev", "prop" );
   ip.add( IndiElement( "a", "1" ) );
   ip.add( IndiElement( "b", "2" ) );
   return ip;
}

SCENARIO( "IndiProperty construction, assignment, and comparison", "[IndiProperty]" )
{
   GIVEN( "the various constructors" )
   {
      WHEN( "constructing each way" )
      {
         IndiProperty p0;
         REQUIRE( p0.getType() == IndiProperty::Unknown );

         IndiProperty p1( IndiProperty::Text );
         REQUIRE( p1.getType() == IndiProperty::Text );

         IndiProperty p2( IndiProperty::Number, "dev", "nm" );
         REQUIRE( p2.getDevice() == "dev" );
         REQUIRE( p2.getName() == "nm" );

         IndiProperty p3( IndiProperty::Switch, "dev", "sw", IndiProperty::Ok,
                          IndiProperty::ReadWrite, IndiProperty::OneOfMany );
         REQUIRE( p3.getState() == IndiProperty::Ok );
         REQUIRE( p3.getPerm() == IndiProperty::ReadWrite );
         REQUIRE( p3.getRule() == IndiProperty::OneOfMany );

         IndiProperty p4( p3 );
         REQUIRE( p4.getName() == "sw" );

         IndiProperty p5;
         p5 = p3;
         REQUIRE( p5.getDevice() == "dev" );
         p5 = p5; // self-assignment branch
         REQUIRE( p5.getName() == "sw" );

         // BLOBEnable assignment operator
         p5 = IndiProperty::Also;
         REQUIRE( p5.getBLOBEnable() == IndiProperty::Also );
      }
   }

   GIVEN( "properties with elements to compare" )
   {
      WHEN( "using operator== and the compare methods" )
      {
         IndiProperty a = makeProp();
         REQUIRE( a == a ); // self

         IndiProperty b = makeProp();
         REQUIRE( a == b );

         IndiProperty fewer( IndiProperty::Number, "dev", "prop" );
         fewer.add( IndiElement( "a", "1" ) );
         REQUIRE( !( a == fewer ) ); // different sizes

         IndiProperty renamed = makeProp();
         renamed.remove( "b" );
         renamed.add( IndiElement( "c", "2" ) );
         REQUIRE( !( a == renamed ) ); // same size, an element name differs

         IndiProperty valDiff = makeProp();
         valDiff[ "a" ].setValue( std::string( "999" ) );
         REQUIRE( !( a == valDiff ) ); // same names, an element value differs

         // compareProperty: type, device/name, sizes, element names -- not values
         REQUIRE( a.compareProperty( a ) );
         REQUIRE( a.compareProperty( b ) );
         IndiProperty difType( IndiProperty::Text, "dev", "prop" );
         REQUIRE( !a.compareProperty( difType ) );
         IndiProperty difName( IndiProperty::Number, "dev", "other" );
         REQUIRE( !a.compareProperty( difName ) );
         REQUIRE( !a.compareProperty( fewer ) );
         REQUIRE( !a.compareProperty( renamed ) );
         b["a"] = 99; // value changes don't matter to compareProperty
         REQUIRE( a.compareProperty( b ) );

         // compareValue: one named element's value
         REQUIRE( a.compareValue( a, "a" ) );
         REQUIRE( !a.compareValue( difType, "a" ) );
         REQUIRE( !a.compareValue( difName, "a" ) );
         REQUIRE( !a.compareValue( b, "a" ) ); // b["a"] now 99
         REQUIRE( a.compareValue( b, "b" ) );
         REQUIRE( !a.compareValue( b, "zz" ) );      // not in either
         IndiProperty extra = makeProp();
         extra.add( IndiElement( "onlyhere", "5" ) );
         REQUIRE( !extra.compareValue( a, "onlyhere" ) ); // in this, not in comp

         // compareValues: all element values
         REQUIRE( a.compareValues( a ) );
         IndiProperty c = makeProp();
         REQUIRE( a.compareValues( c ) );
         REQUIRE( !a.compareValues( difType ) );
         REQUIRE( !a.compareValues( difName ) );
         REQUIRE( !a.compareValues( fewer ) );
         REQUIRE( !a.compareValues( renamed ) );
         REQUIRE( !a.compareValues( b ) ); // value differs

         // hasNewValue
         REQUIRE( !a.hasNewValue( a, "a" ) ); // self: no new value
         REQUIRE( !a.hasNewValue( difType, "a" ) );
         REQUIRE( !a.hasNewValue( difName, "a" ) );
         REQUIRE( !a.hasNewValue( c, "a" ) );  // same value
         REQUIRE( a.hasNewValue( b, "a" ) );   // 99 is new and non-blank
         REQUIRE( !a.hasNewValue( b, "zz" ) ); // not in this
         REQUIRE( !extra.hasNewValue( a, "onlyhere" ) ); // not in comp
         IndiProperty blank = makeProp();
         blank["a"] = IndiElement( "a", "" );
         blank.update( IndiElement( "a", "" ) );
         REQUIRE( !a.hasNewValue( blank, "a" ) ); // differs but blank
      }
   }
}

SCENARIO( "IndiProperty element access and manipulation", "[IndiProperty]" )
{
   GIVEN( "a property with two elements" )
   {
      IndiProperty ip = makeProp();

      WHEN( "accessing elements by name and index" )
      {
         REQUIRE( ip.getNumElements() == 2 );
         REQUIRE( ip.at( "a" ).getValue() == "1" );
         REQUIRE( ip.at( (unsigned int)1 ).getValue() == "2" );
         REQUIRE( ip[ "b" ].getValue() == "2" );
         REQUIRE( ip[ (unsigned int)0 ].getValue() == "1" );

         const IndiProperty &cip = ip;
         REQUIRE( cip.at( "a" ).getValue() == "1" );
         REQUIRE( cip.at( (unsigned int)0 ).getValue() == "1" );
         REQUIRE( cip[ "a" ].getValue() == "1" );
         REQUIRE( cip[ (unsigned int)1 ].getValue() == "2" );

         REQUIRE_THROWS( ip.at( "nope" ) );
         REQUIRE_THROWS( ip.at( (unsigned int)5 ) );
         REQUIRE_THROWS( ip[ "nope" ] );
         REQUIRE_THROWS( ip[ (unsigned int)5 ] );
         REQUIRE_THROWS( cip.at( "nope" ) );
         REQUIRE_THROWS( cip.at( (unsigned int)5 ) );
         REQUIRE_THROWS( cip[ "nope" ] );
         REQUIRE_THROWS( cip[ (unsigned int)5 ] );

         REQUIRE( ip.find( "a" ) );
         REQUIRE( !ip.find( "nope" ) );
      }

      WHEN( "adding, updating, and removing elements" )
      {
         ip.update( IndiElement( "c", "3" ) ); // update-or-add path adds
         REQUIRE( ip.find( "c" ) );
         ip.update( IndiElement( "c", "4" ) ); // update existing
         REQUIRE( ip[ "c" ].getValue() == "4" );

         ip.addIfNoExist( IndiElement( "c", "5" ) ); // exists: no-op
         REQUIRE( ip[ "c" ].getValue() == "4" );
         ip.addIfNoExist( IndiElement( "d", "6" ) ); // doesn't exist: added
         REQUIRE( ip[ "d" ].getValue() == "6" );

         REQUIRE_THROWS( ip.add( IndiElement( "a", "9" ) ) ); // already exists

         ip.update( "d", IndiElement( "d", "7" ) );
         REQUIRE( ip[ "d" ].getValue() == "7" );
         REQUIRE_THROWS( ip.update( "nope", IndiElement( "nope", "0" ) ) );

         ip.remove( "d" );
         REQUIRE( !ip.find( "d" ) );
         REQUIRE_THROWS( ip.remove( "nope" ) );

         // whole-map get/set
         std::map<std::string, IndiElement> els = ip.getElements();
         els[ "e" ] = IndiElement( "e", "8" );
         ip.setElements( els );
         REQUIRE( ip.find( "e" ) );
      }
   }
}

SCENARIO( "IndiProperty attributes, validity, and strings", "[IndiProperty]" )
{
   GIVEN( "a fully-populated property" )
   {
      IndiProperty ip( IndiProperty::Number );
      ip.setDevice( "adevice" );
      ip.setName( "aname" );
      ip.setGroup( "agroup" );
      ip.setLabel( "alabel" );
      ip.setMessage( "amessage" );
      ip.setPerm( IndiProperty::ReadWrite );
      ip.setRule( IndiProperty::AnyOfMany );
      ip.setState( IndiProperty::Busy );
      ip.setTimeout( 5.5 );
      ip.setTimeStamp( pcf::TimeStamp::now() );
      ip.setVersion( "1.7" );
      ip.setBLOBEnable( IndiProperty::Only );
      ip.setRequested( true );
      ip.add( IndiElement( "x", "10" ) );
      ip.add( IndiElement( "y", "20" ) );

      WHEN( "reading attributes back" )
      {
         REQUIRE( ip.getDevice() == "adevice" );
         REQUIRE( ip.getName() == "aname" );
         REQUIRE( ip.getGroup() == "agroup" );
         REQUIRE( ip.getLabel() == "alabel" );
         REQUIRE( ip.getMessage() == "amessage" );
         REQUIRE( ip.getPerm() == IndiProperty::ReadWrite );
         REQUIRE( ip.getRule() == IndiProperty::AnyOfMany );
         REQUIRE( ip.getState() == IndiProperty::Busy );
         REQUIRE( ip.getTimeout() == 5.5 );
         REQUIRE( ip.getVersion() == "1.7" );
         REQUIRE( ip.getBLOBEnable() == IndiProperty::Only );
         REQUIRE( ip.isRequested() == true );
         REQUIRE( ip.getTimeStamp().getTimeVal().tv_sec > 0 );
         REQUIRE( ip.createUniqueKey() == "adevice.aname" );
      }

      WHEN( "checking validity flags, populated and empty" )
      {
         REQUIRE( ip.hasValidBLOBEnable() );
         REQUIRE( ip.hasValidDevice() );
         REQUIRE( ip.hasValidGroup() );
         REQUIRE( ip.hasValidLabel() );
         REQUIRE( ip.hasValidMessage() );
         REQUIRE( ip.hasValidName() );
         REQUIRE( ip.hasValidPerm() );
         REQUIRE( ip.hasValidRule() );
         REQUIRE( ip.hasValidState() );
         REQUIRE( ip.hasValidTimeout() );
         REQUIRE( ip.hasValidTimeStamp() );
         REQUIRE( ip.hasValidVersion() );

         IndiProperty empty;
         REQUIRE( !empty.hasValidBLOBEnable() );
         REQUIRE( !empty.hasValidDevice() );
         REQUIRE( !empty.hasValidGroup() );
         REQUIRE( !empty.hasValidLabel() );
         REQUIRE( !empty.hasValidMessage() );
         REQUIRE( !empty.hasValidName() );
         REQUIRE( !empty.hasValidPerm() );
         REQUIRE( !empty.hasValidRule() );
         REQUIRE( !empty.hasValidState() );
         REQUIRE( !empty.hasValidTimeout() );
         REQUIRE( !empty.hasValidVersion() );
      }

      WHEN( "createString renders attributes and elements" )
      {
         std::string s = ip.createString();
         REQUIRE( s.find( "\"device\" : \"adevice\"" ) != std::string::npos );
         REQUIRE( s.find( "\"name\" : \"aname\"" ) != std::string::npos );
         REQUIRE( s.find( "\"perm\" : \"rw\"" ) != std::string::npos );
         REQUIRE( s.find( "\"name\" : \"x\"" ) != std::string::npos );
         REQUIRE( s.find( "\"name\" : \"y\"" ) != std::string::npos );
      }

      WHEN( "clear removes the elements (attributes are untouched)" )
      {
         ip.clear();
         REQUIRE( ip.getNumElements() == 0 );
         REQUIRE( ip.hasValidDevice() );
      }
   }

   GIVEN( "names to scrub" )
   {
      WHEN( "scrubbing names with dots and non-alphanumerics" )
      {
         REQUIRE( IndiProperty::scrubName( "dev.prop" ) == "dev___prop" );
         REQUIRE( IndiProperty::scrubName( "a b-c" ) == "a_b_c" );
         REQUIRE( IndiProperty::scrubName( "clean123" ) == "clean123" );
      }
   }
}

SCENARIO( "IndiProperty error messages and enum converters", "[IndiProperty]" )
{
   GIVEN( "each error code and enum value" )
   {
      WHEN( "converting error codes to messages" )
      {
         REQUIRE( IndiProperty::getErrorMsg( IndiProperty::ErrNone ) == "No Error" );
         REQUIRE( IndiProperty::getErrorMsg( IndiProperty::ErrCouldntFindElement ) == "Could not find element" );
         REQUIRE( IndiProperty::getErrorMsg( IndiProperty::ErrElementAlreadyExists ) == "Element already exists" );
         REQUIRE( IndiProperty::getErrorMsg( IndiProperty::ErrIndexOutOfBounds ) == "Index out of bounds" );
         REQUIRE( IndiProperty::getErrorMsg( -12345 ) == "Unknown error" );

         // The Excep exception type itself: thrown by add()/at()/remove(),
         // caught here to exercise its constructor, what(), and destructor.
         IndiProperty ip = makeProp();
         try
         {
            ip.add( IndiElement( "a", "9" ) ); // duplicate: throws Excep
            REQUIRE( false );
         }
         catch( const IndiProperty::Excep &e )
         {
            REQUIRE( e.getCode() == IndiProperty::ErrElementAlreadyExists );
            static_cast<void>( e.what() );
         }
      }

      WHEN( "converting BLOB enables both ways" )
      {
         REQUIRE( IndiProperty::getBLOBEnableType( "Never" ) == IndiProperty::Never );
         REQUIRE( IndiProperty::getBLOBEnableType( "Also" ) == IndiProperty::Also );
         REQUIRE( IndiProperty::getBLOBEnableType( "Only" ) == IndiProperty::Only );
         REQUIRE( IndiProperty::getBLOBEnableType( "junk" ) == IndiProperty::UnknownBLOBEnable );

         REQUIRE( IndiProperty::getBLOBEnableString( IndiProperty::UnknownBLOBEnable ) == "" );
         REQUIRE( IndiProperty::getBLOBEnableString( IndiProperty::Never ) == "Never" );
         REQUIRE( IndiProperty::getBLOBEnableString( IndiProperty::Also ) == "Also" );
         REQUIRE( IndiProperty::getBLOBEnableString( IndiProperty::Only ) == "Only" );
      }

      WHEN( "converting property states both ways" )
      {
         REQUIRE( IndiProperty::getPropertyStateType( "Idle" ) == IndiProperty::Idle );
         REQUIRE( IndiProperty::getPropertyStateType( "Ok" ) == IndiProperty::Ok );
         REQUIRE( IndiProperty::getPropertyStateType( "Busy" ) == IndiProperty::Busy );
         REQUIRE( IndiProperty::getPropertyStateType( "Alert" ) == IndiProperty::Alert );
         REQUIRE( IndiProperty::getPropertyStateType( "junk" ) == IndiProperty::UnknownPropertyState );

         REQUIRE( IndiProperty::getPropertyStateString( IndiProperty::UnknownPropertyState ) == "" );
         REQUIRE( IndiProperty::getPropertyStateString( IndiProperty::Idle ) == "Idle" );
         REQUIRE( IndiProperty::getPropertyStateString( IndiProperty::Ok ) == "Ok" );
         REQUIRE( IndiProperty::getPropertyStateString( IndiProperty::Busy ) == "Busy" );
         REQUIRE( IndiProperty::getPropertyStateString( IndiProperty::Alert ) == "Alert" );
      }

      WHEN( "converting switch rules both ways" )
      {
         REQUIRE( IndiProperty::getSwitchRuleType( "OneOfMany" ) == IndiProperty::OneOfMany );
         REQUIRE( IndiProperty::getSwitchRuleType( "AtMostOne" ) == IndiProperty::AtMostOne );
         REQUIRE( IndiProperty::getSwitchRuleType( "AnyOfMany" ) == IndiProperty::AnyOfMany );
         REQUIRE( IndiProperty::getSwitchRuleType( "junk" ) == IndiProperty::UnknownSwitchRule );

         REQUIRE( IndiProperty::getSwitchRuleString( IndiProperty::OneOfMany ) == "OneOfMany" );
         REQUIRE( IndiProperty::getSwitchRuleString( IndiProperty::AtMostOne ) == "AtMostOne" );
         REQUIRE( IndiProperty::getSwitchRuleString( IndiProperty::AnyOfMany ) == "AnyOfMany" );
         REQUIRE( IndiProperty::getSwitchRuleString( IndiProperty::UnknownSwitchRule ) == "" );
      }

      WHEN( "converting permissions both ways" )
      {
         REQUIRE( IndiProperty::getPropertyPermType( "ro" ) == IndiProperty::ReadOnly );
         REQUIRE( IndiProperty::getPropertyPermType( "wo" ) == IndiProperty::WriteOnly );
         REQUIRE( IndiProperty::getPropertyPermType( "rw" ) == IndiProperty::ReadWrite );
         REQUIRE( IndiProperty::getPropertyPermType( "junk" ) == IndiProperty::UnknownPropertyPerm );

         REQUIRE( IndiProperty::getPropertyPermString( IndiProperty::UnknownPropertyPerm ) == "" );
         REQUIRE( IndiProperty::getPropertyPermString( IndiProperty::ReadOnly ) == "ro" );
         REQUIRE( IndiProperty::getPropertyPermString( IndiProperty::WriteOnly ) == "wo" );
         REQUIRE( IndiProperty::getPropertyPermString( IndiProperty::ReadWrite ) == "rw" );
      }

      WHEN( "converting property types both ways" )
      {
         REQUIRE( IndiProperty::convertTypeToString( IndiProperty::Unknown ) == "" );
         REQUIRE( IndiProperty::convertTypeToString( IndiProperty::BLOB ) == "BLOB" );
         REQUIRE( IndiProperty::convertTypeToString( IndiProperty::Light ) == "Light" );
         REQUIRE( IndiProperty::convertTypeToString( IndiProperty::Number ) == "Number" );
         REQUIRE( IndiProperty::convertTypeToString( IndiProperty::Switch ) == "Switch" );
         REQUIRE( IndiProperty::convertTypeToString( IndiProperty::Text ) == "Text" );

         REQUIRE( IndiProperty::convertStringToType( "BLOB" ) == IndiProperty::BLOB );
         REQUIRE( IndiProperty::convertStringToType( "Light" ) == IndiProperty::Light );
         REQUIRE( IndiProperty::convertStringToType( "Number" ) == IndiProperty::Number );
         REQUIRE( IndiProperty::convertStringToType( "Switch" ) == IndiProperty::Switch );
         REQUIRE( IndiProperty::convertStringToType( "Text" ) == IndiProperty::Text );
         REQUIRE( IndiProperty::convertStringToType( "junk" ) == IndiProperty::Unknown );
      }
   }
}

} //namespace IndiProperty_test
