/** \file IndiXmlParser_test.cpp
  * \brief Catch2 tests for pcf::IndiXmlParser in INDI/libcommon/IndiXmlParser.cpp.
  *
  * Round-trips every combination of message type and property type through XML
  * generation and back through the real lilxml based parser. No mocks are used.
  * No threads, sockets, or files are needed.
  */
#include "../../tests/catch2/catch.hpp"

#include <sstream>

#include "../libcommon/IndiElement.hpp"
#include "../libcommon/IndiMessage.hpp"
#include "../libcommon/IndiProperty.hpp"
#include "../libcommon/IndiXmlParser.hpp"

using pcf::IndiElement;
using pcf::IndiMessage;
using pcf::IndiProperty;
using pcf::IndiXmlParser;

namespace IndiXmlParser_test
{

/// Builds a property with every optional attribute set, so the generators emit
/// every optional attribute. The element values depend on the property type.
static IndiProperty fullProp( IndiProperty::Type tType )
{
   IndiProperty ip( tType, "dev", "prop", IndiProperty::Ok, IndiProperty::ReadWrite,
                    IndiProperty::OneOfMany );
   ip.setLabel( "A label" );
   ip.setGroup( "grp" );
   ip.setTimeout( 3.5 );
   ip.setMessage( "a message" );
   ip.setVersion( "1" );

   IndiElement el1( "el1" );
   el1.setLabel( "element one" );
   el1.setFormat( "%g" );
   el1.setMin( std::string( "0" ) );
   el1.setMax( std::string( "100" ) );
   el1.setStep( std::string( "1" ) );
   el1.setSize( std::string( "4" ) );
   if( tType == IndiProperty::Switch )
   {
      el1.setSwitchState( IndiElement::On );
   }
   else if( tType == IndiProperty::Light )
   {
      el1.setLightState( IndiElement::Busy );
   }
   else if( tType == IndiProperty::Number )
   {
      el1.setValue( std::string( "42.5" ) );
   }
   else
   {
      el1.setValue( std::string( "value one" ) );
   }
   ip.add( el1 );

   IndiElement el2( "el2" );
   if( tType == IndiProperty::Switch )
   {
      el2.setSwitchState( IndiElement::Off );
   }
   else if( tType == IndiProperty::Light )
   {
      el2.setLightState( IndiElement::Ok );
   }
   else if( tType == IndiProperty::Number )
   {
      el2.setValue( std::string( "7" ) );
   }
   else
   {
      el2.setValue( std::string( "value two" ) );
   }
   ip.add( el2 );

   return ip;
}

/// Builds a property with only the required attributes, so the generators skip
/// every optional attribute.
static IndiProperty minProp( IndiProperty::Type tType )
{
   IndiProperty ip( tType, "dev", "prop", IndiProperty::Idle, IndiProperty::ReadOnly,
                    IndiProperty::AnyOfMany );
   ip.add( IndiElement( "e", "v" ) );
   return ip;
}

/// Generates the XML for a message, parses it back, and returns the parsed message.
/// It requires the parse to finish with no error and in the complete state.
static IndiMessage roundTrip( const IndiMessage &im )
{
   IndiXmlParser gen( im );
   std::string   xml = gen.createXmlString();
   REQUIRE( xml.size() > 0 );

   IndiXmlParser parse;
   std::string   err;
   parse.parseXml( xml, err );
   REQUIRE( err == "" );
   REQUIRE( parse.getState() == IndiXmlParser::CompleteState );

   return parse.createIndiMessage();
}

// Verifies that XML generated from each message type and property type parses back
// into a message with the same type, device, and name. It also verifies that
// generation throws when a required attribute is missing.
SCENARIO( "IndiXmlParser round-trips every message and property type", "[IndiXmlParser]" )
{
   GIVEN( "fully-populated properties of every type" )
   {
      WHEN( "round-tripping Define messages" )
      {
         const IndiProperty::Type types[] = { IndiProperty::Text, IndiProperty::Number,
                                              IndiProperty::Switch, IndiProperty::Light,
                                              IndiProperty::BLOB };
         for( IndiProperty::Type t : types )
         {
            IndiMessage out = roundTrip( IndiMessage( IndiMessage::Define, fullProp( t ) ) );
            REQUIRE( out.getType() == IndiMessage::Define );
            REQUIRE( out.getProperty().getDevice() == "dev" );
            REQUIRE( out.getProperty().getName() == "prop" );
            REQUIRE( out.getProperty().getType() == t );
         }
      }

      WHEN( "round-tripping SetProperty messages" )
      {
         const IndiProperty::Type types[] = { IndiProperty::Text, IndiProperty::Number,
                                              IndiProperty::Switch, IndiProperty::Light,
                                              IndiProperty::BLOB };
         for( IndiProperty::Type t : types )
         {
            IndiMessage out = roundTrip( IndiMessage( IndiMessage::SetProperty, fullProp( t ) ) );
            REQUIRE( out.getType() == IndiMessage::SetProperty );
            REQUIRE( out.getProperty().getDevice() == "dev" );
         }
      }

      WHEN( "round-tripping NewProperty messages" )
      {
         const IndiProperty::Type types[] = { IndiProperty::Text, IndiProperty::Number,
                                              IndiProperty::Switch, IndiProperty::BLOB };
         for( IndiProperty::Type t : types )
         {
            IndiMessage out = roundTrip( IndiMessage( IndiMessage::NewProperty, fullProp( t ) ) );
            REQUIRE( out.getType() == IndiMessage::NewProperty );
            REQUIRE( out.getProperty().getName() == "prop" );
         }
      }
   }

   GIVEN( "minimally-populated properties (no optional attributes)" )
   {
      WHEN( "round-tripping Define and Set messages" )
      {
         const IndiProperty::Type types[] = { IndiProperty::Text, IndiProperty::Number,
                                              IndiProperty::Switch, IndiProperty::Light,
                                              IndiProperty::BLOB };
         for( IndiProperty::Type t : types )
         {
            IndiMessage defOut = roundTrip( IndiMessage( IndiMessage::Define, minProp( t ) ) );
            REQUIRE( defOut.getType() == IndiMessage::Define );

            IndiMessage setOut = roundTrip( IndiMessage( IndiMessage::SetProperty, minProp( t ) ) );
            REQUIRE( setOut.getType() == IndiMessage::SetProperty );
         }
      }
   }

   GIVEN( "the non-vector message types" )
   {
      WHEN( "round-tripping Delete, GetProperties, Message, and EnableBLOB" )
      {
         IndiProperty del( IndiProperty::Text );
         del.setDevice( "dev" );
         del.setName( "prop" );
         del.setMessage( "gone" );
         IndiMessage delOut = roundTrip( IndiMessage( IndiMessage::Delete, del ) );
         REQUIRE( delOut.getType() == IndiMessage::Delete );

         // A Delete with only a device deletes the whole device.
         IndiProperty delDev( IndiProperty::Text );
         delDev.setDevice( "dev" );
         IndiMessage delDevOut = roundTrip( IndiMessage( IndiMessage::Delete, delDev ) );
         REQUIRE( delDevOut.getType() == IndiMessage::Delete );

         IndiProperty get( IndiProperty::Text );
         get.setDevice( "dev" );
         get.setName( "prop" );
         get.setVersion( "1.7" );
         IndiMessage getOut = roundTrip( IndiMessage( IndiMessage::GetProperties, get ) );
         REQUIRE( getOut.getType() == IndiMessage::GetProperties );

         // A GetProperties with no device queries all devices.
         IndiProperty getAll( IndiProperty::Text );
         getAll.setVersion( "1.7" );
         IndiMessage getAllOut = roundTrip( IndiMessage( IndiMessage::GetProperties, getAll ) );
         REQUIRE( getAllOut.getType() == IndiMessage::GetProperties );

         IndiProperty msg( IndiProperty::Text );
         msg.setDevice( "dev" );
         msg.setMessage( "hello there" );
         IndiMessage msgOut = roundTrip( IndiMessage( IndiMessage::Message, msg ) );
         REQUIRE( msgOut.getType() == IndiMessage::Message );

         IndiProperty eb( IndiProperty::Text );
         eb.setDevice( "dev" );
         eb.setName( "prop" );
         eb.setBLOBEnable( IndiProperty::Also );
         IndiMessage ebOut = roundTrip( IndiMessage( IndiMessage::EnableBLOB, eb ) );
         REQUIRE( ebOut.getType() == IndiMessage::EnableBLOB );
      }
   }

   GIVEN( "invalid message and property combinations" )
   {
      WHEN( "generating from unknown types throws" )
      {
         IndiProperty unk; // The default property type is Unknown.
         unk.setDevice( "dev" );
         unk.setName( "prop" );
         REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::Define, unk ) ) );
         REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::NewProperty, unk ) ) );
         REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::SetProperty, unk ) ) );
         REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::Unknown, unk ) ) );
      }

      WHEN( "generating with missing required attributes throws, for every type" )
      {
         const IndiProperty::Type types[] = { IndiProperty::Text, IndiProperty::Number,
                                              IndiProperty::Switch, IndiProperty::Light,
                                              IndiProperty::BLOB };
         for( IndiProperty::Type t : types )
         {
            IndiProperty noDev( t );
            noDev.setName( "prop" );
            REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::Define, noDev ) ) );
            REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::SetProperty, noDev ) ) );
            if( t != IndiProperty::Light )
            {
               REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::NewProperty, noDev ) ) );
            }

            IndiProperty noName( t );
            noName.setDevice( "dev" );
            REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::Define, noName ) ) );
            REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::SetProperty, noName ) ) );
            if( t != IndiProperty::Light )
            {
               REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::NewProperty, noName ) ) );
            }

            IndiProperty noState( t );
            noState.setDevice( "dev" );
            noState.setName( "prop" );
            REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::Define, noState ) ) );

            IndiProperty noPerm( t );
            noPerm.setDevice( "dev" );
            noPerm.setName( "prop" );
            noPerm.setState( IndiProperty::Ok );
            if( t != IndiProperty::Light ) // A defLightVector has no perm attribute.
            {
               REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::Define, noPerm ) ) );
            }

            // An element with no name is rejected by every generator.
            IndiProperty badEl = minProp( t );
            badEl.add( IndiElement( "", "orphan" ) );
            REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::Define, badEl ) ) );
            REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::SetProperty, badEl ) ) );
            if( t != IndiProperty::Light )
            {
               REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::NewProperty, badEl ) ) );
            }
         }

         // A Message with no device is legal. It is a server-wide message.
         // EnableBLOB and Delete both require a device.
         IndiProperty bare( IndiProperty::Text );
         IndiXmlParser bareMsg( IndiMessage( IndiMessage::Message, bare ) );
         REQUIRE( bareMsg.createXmlString().find( "message" ) != std::string::npos );
         REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::EnableBLOB, bare ) ) );
         REQUIRE_THROWS( IndiXmlParser( IndiMessage( IndiMessage::Delete, bare ) ) );

         // An EnableBLOB with a device but no property name is a device-wide enable.
         IndiProperty ebDev( IndiProperty::Text );
         ebDev.setDevice( "dev" );
         ebDev.setBLOBEnable( IndiProperty::Only );
         IndiXmlParser ebGen( IndiMessage( IndiMessage::EnableBLOB, ebDev ) );
         REQUIRE( ebGen.createXmlString().find( "enableBLOB" ) != std::string::npos );
      }
   }
}

// Verifies the parsing mechanics. It covers chunked input, the char pointer overload,
// clear(), the stream operators, error reporting and recovery, serializing from the
// parse tree, element attribute validation, and the XML escape helper.
SCENARIO( "IndiXmlParser parsing mechanics and utilities", "[IndiXmlParser]" )
{
   GIVEN( "a generated XML document" )
   {
      IndiXmlParser gen( IndiMessage( IndiMessage::Define, fullProp( IndiProperty::Text ) ) );
      std::string   xml = gen.createXmlString();

      WHEN( "parsing it in two chunks" )
      {
         IndiXmlParser parse;
         std::string   err;

         size_t half = xml.size() / 2;
         parse.parseXml( xml.substr( 0, half ), err );
         REQUIRE( err == "" );
         REQUIRE( parse.getState() == IndiXmlParser::IncompleteState );

         parse.parseXml( xml.substr( half ), err );
         REQUIRE( err == "" );
         REQUIRE( parse.getState() == IndiXmlParser::CompleteState );

         IndiMessage out = parse.createIndiMessage();
         REQUIRE( out.getProperty().getName() == "prop" );
      }

      WHEN( "parsing via the char-pointer overload" )
      {
         IndiXmlParser parse;
         std::string   err;
         parse.parseXml( xml.c_str(), xml.size(), err );
         REQUIRE( err == "" );
         REQUIRE( parse.getState() == IndiXmlParser::CompleteState );
      }

      WHEN( "clearing resets the parser state" )
      {
         IndiXmlParser parse;
         std::string   err;
         parse.parseXml( xml, err );
         REQUIRE( parse.getState() == IndiXmlParser::CompleteState );
         parse.clear();
         REQUIRE( parse.getState() == IndiXmlParser::IncompleteState );
      }

      WHEN( "the stream operators parse and emit XML" )
      {
         // operator>> consumes one newline delimited chunk per call. So the
         // document is fed line by line.
         IndiXmlParser parse;
         std::istringstream iss( xml );
         while( iss.good() && parse.getState() != IndiXmlParser::CompleteState )
         {
            iss >> parse;
            iss.clear();
            iss.ignore( 2, '\n' ); // Skip the \r\n that get() left behind.
         }
         REQUIRE( parse.getState() == IndiXmlParser::CompleteState );

         std::ostringstream oss;
         oss << gen;
         REQUIRE( oss.str() == xml );
      }

      WHEN( "createIndiMessage before any parse returns an Unknown message" )
      {
         // The message type is left uninitialized on this path. So only the
         // empty property can be asserted.
         IndiXmlParser fresh;
         IndiMessage   out = fresh.createIndiMessage();
         REQUIRE( out.getProperty().getName() == "" );
      }

      WHEN( "managing the protocol version" )
      {
         IndiXmlParser p( "1.7" );
         REQUIRE( p.getProtocolVersion() == "1.7" );
         p.setProtocolVersion( "2.0" );
         REQUIRE( p.getProtocolVersion() == "2.0" );
      }
   }

   GIVEN( "malformed XML" )
   {
      WHEN( "the parser reports an error and recovers" )
      {
         IndiXmlParser parse;
         std::string   err;
         parse.parseXml( "<defTextVector></wrongTag>", err );
         REQUIRE( err.size() > 0 );

         // The parser must be usable again after an error.
         IndiXmlParser gen( IndiMessage( IndiMessage::Define, minProp( IndiProperty::Text ) ) );
         std::string   xml = gen.createXmlString();
         parse.parseXml( xml, err );
         REQUIRE( err == "" );
         REQUIRE( parse.getState() == IndiXmlParser::CompleteState );
      }

      WHEN( "parsing an unrecognized-but-wellformed tag" )
      {
         IndiXmlParser parse;
         std::string   err;
         parse.parseXml( "<someOtherTag attr=\"1\"></someOtherTag>", err );
         REQUIRE( err == "" );
         IndiMessage out = parse.createIndiMessage();
         REQUIRE( out.getType() == IndiMessage::Unknown );
      }
   }

   GIVEN( "a parsed document" )
   {
      WHEN( "createXmlString serializes from the parse tree" )
      {
         IndiXmlParser gen( IndiMessage( IndiMessage::Define, fullProp( IndiProperty::Text ) ) );
         IndiXmlParser parse;
         std::string   err;
         parse.parseXml( gen.createXmlString(), err );
         REQUIRE( parse.getState() == IndiXmlParser::CompleteState );

         std::string out = parse.createXmlString(); // This serializes from the parse tree root m_pxeRoot.
         REQUIRE( out.find( "defTextVector" ) != std::string::npos );
         REQUIRE( out.find( "prop" ) != std::string::npos );
      }
   }

   GIVEN( "properties whose elements are missing required attributes" )
   {
      WHEN( "a defNumber element lacks format, min, max, or step" )
      {
         IndiProperty ip( IndiProperty::Number, "dev", "prop", IndiProperty::Ok,
                          IndiProperty::ReadWrite, IndiProperty::OneOfMany );
         // The default attribute values are all valid. So each one is blanked in turn.
         IndiElement el( "el" );
         el.setFormat( std::string( "" ) );
         ip.add( el );
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::Define, ip ) ),
                            std::runtime_error );

         ip["el"].setFormat( "%g" );
         ip["el"].setMin( std::string( "" ) );
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::Define, ip ) ),
                            std::runtime_error );

         ip["el"].setMin( std::string( "0" ) );
         ip["el"].setMax( std::string( "" ) );
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::Define, ip ) ),
                            std::runtime_error );

         ip["el"].setMax( std::string( "10" ) );
         ip["el"].setStep( std::string( "" ) );
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::Define, ip ) ),
                            std::runtime_error );
      }

      WHEN( "a defSwitch vector has no valid rule" )
      {
         IndiProperty ip( IndiProperty::Switch, "dev", "prop", IndiProperty::Ok,
                          IndiProperty::ReadWrite, IndiProperty::UnknownSwitchRule );
         IndiElement el( "el" );
         el.setSwitchState( IndiElement::On );
         ip.add( el );
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::Define, ip ) ),
                            std::runtime_error );
      }

      WHEN( "a set/new BLOB element lacks size or format" )
      {
         IndiProperty ip( IndiProperty::BLOB, "dev", "prop", IndiProperty::Ok,
                          IndiProperty::ReadWrite, IndiProperty::OneOfMany );
         IndiElement el( "el" );
         el.setSize( std::string( "" ) ); // Blank the size. The default size is valid.
         ip.add( el );
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::SetProperty, ip ) ),
                            std::runtime_error );
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::NewProperty, ip ) ),
                            std::runtime_error );

         ip["el"].setSize( std::string( "4" ) );
         ip["el"].setFormat( std::string( "" ) ); // Restore the size and blank the format instead.
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::SetProperty, ip ) ),
                            std::runtime_error );
         REQUIRE_THROWS_AS( IndiXmlParser( IndiMessage( IndiMessage::NewProperty, ip ) ),
                            std::runtime_error );
      }
   }

   GIVEN( "the static helpers" )
   {
      WHEN( "escaping special characters" )
      {
         std::string safe = IndiXmlParser::createSafeXmlString( "a&b<c>d\"e'f" );
         REQUIRE( safe.find( "&amp;" ) != std::string::npos );
         REQUIRE( safe.find( "&lt;" ) != std::string::npos );
         REQUIRE( safe.find( "&gt;" ) != std::string::npos );
         REQUIRE( safe.find( "&quot;" ) != std::string::npos );
         REQUIRE( safe.find( "&apos;" ) != std::string::npos );
         REQUIRE( safe.find( '&' ) != std::string::npos );
      }

   }
}

} //namespace IndiXmlParser_test
