#!/bin/perl



@FILEs = `find ./MAME/src/drivers/*.c`;

use constant DATA_PREFIX  => 'D';
use constant CODE_PREFIX  => 'C';
use constant BSS_PREFIX   => 'B';
use constant CONST_PREFIX => 'K';


	# Create the imagebld.switch file
open( PRELOADFILE, ">imagebld.switch" );
print PRELOADFILE "/OUT:\"c:\\MAMEoX\\MAMEoX\\Debug\\MAMEoX.xbe\"\r\n";
print PRELOADFILE "/STACK:\"0x10000\"\r\n";
print PRELOADFILE "/DEBUG\r\n";
print PRELOADFILE "/LIMITMEM\r\n";
print PRELOADFILE "/NOLOGO\r\n";


	# Create the Sections.h file
open( GENERATEDFILE, ">./MAMEoX/includes/Sections.h" );
print GENERATEDFILE "/**\r\n";
print GENERATEDFILE "  * \\file      Sections.h\r\n";
print GENERATEDFILE "  * \\brief     Registration of MAME files for creation and usage of XBOX\r\n";
print GENERATEDFILE "  *             loadable sections.\r\n";
print GENERATEDFILE "  *\r\n";
print GENERATEDFILE "  * \\note      This file is autogenerated via Sectionize.pl DO NOT EDIT!\r\n";
print GENERATEDFILE "  */\r\n";
print GENERATEDFILE "#pragma once\r\n";
print GENERATEDFILE "//= I N C L U D E S ====================================================\r\n";
print GENERATEDFILE "#ifdef __cplusplus\r\n";
print GENERATEDFILE "extern \"C\" {\r\n";
print GENERATEDFILE "#endif\r\n";
print GENERATEDFILE "#include \"osd_cpu.h\"\r\n";
print GENERATEDFILE "#ifdef __cplusplus\r\n";
print GENERATEDFILE "}\r\n";
print GENERATEDFILE "#endif\r\n";
print GENERATEDFILE "//= P R O T O T Y P E S ================================================\r\n";
print GENERATEDFILE "\r\n#ifdef _DEBUG\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	CheckDriverSectionRAM\r\n";
print GENERATEDFILE "//! \\brief    Prints the size of each segment\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "void CheckDriverSectionRAM( void );\r\n";
print GENERATEDFILE "#endif\r\n\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	RegisterDriverSectionNames\r\n";
print GENERATEDFILE "//! \\brief    Registers all of the segments for use by\r\n";
print GENERATEDFILE "//!            LoadDriverSectionByName and UnloadDriverSectionByName\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "void RegisterDriverSectionNames( void );\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	LoadDriverSectionByName\r\n";
print GENERATEDFILE "//! \\brief    Loads the section associated with the passed name\r\n";
print GENERATEDFILE "//!\r\n";
print GENERATEDFILE "//! \\param    DriverFileName - The name of the file whose section\r\n";
print GENERATEDFILE "//!                             should be loaded\r\n";
print GENERATEDFILE "//!\r\n";
print GENERATEDFILE "//! \\return   BOOL - Operation status\r\n";
print GENERATEDFILE "//! \\retval   TRUE - success\r\n";
print GENERATEDFILE "//! \\return   FALSE - Failure\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL LoadDriverSectionByName( const char *DriverFileName );\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	UnloadDriverSectionByName\r\n";
print GENERATEDFILE "//! \\brief    Unloads the section associated with the passed name\r\n";
print GENERATEDFILE "//!\r\n";
print GENERATEDFILE "//! \\param    DriverFileName - The name of the file whose section\r\n";
print GENERATEDFILE "//!                             should be unloaded\r\n";
print GENERATEDFILE "//!\r\n";
print GENERATEDFILE "//! \\return   BOOL - Operation status\r\n";
print GENERATEDFILE "//! \\retval   TRUE - success\r\n";
print GENERATEDFILE "//! \\return   FALSE - Failure\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL UnloadDriverSectionByName( const char *DriverFileName );\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	LoadDriverDataSections\r\n";
print GENERATEDFILE "//! \\brief    Loads all of the driver data sections\r\n";
print GENERATEDFILE "//!\r\n";
print GENERATEDFILE "//! \\return   BOOL - Operation status\r\n";
print GENERATEDFILE "//! \\retval   TRUE - success\r\n";
print GENERATEDFILE "//! \\return   FALSE - Failure\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL LoadDriverDataSections( void );\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	LoadDriverDataSections\r\n";
print GENERATEDFILE "//! \\brief    Unloads all of the driver data sections\r\n";
print GENERATEDFILE "//!\r\n";
print GENERATEDFILE "//! \\return   BOOL - Operation status\r\n";
print GENERATEDFILE "//! \\retval   TRUE - success\r\n";
print GENERATEDFILE "//! \\return   FALSE - Failure\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL UnloadDriverDataSections( void );\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	UnloadDriverNonDataSections\r\n";
print GENERATEDFILE "//! \\brief    Unloads all non-data driver sections\r\n";
print GENERATEDFILE "//!\r\n";
print GENERATEDFILE "//! \\return   BOOL - Operation status\r\n";
print GENERATEDFILE "//! \\retval   TRUE - success\r\n";
print GENERATEDFILE "//! \\return   FALSE - Failure\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL UnloadDriverNonDataSections( void );\r\n";
close( GENERATEDFILE );


	# Create the DriverSections.cpp file
open( GENERATEDFILE, ">./MAMEoX/sources/DriverSections.cpp" );
print GENERATEDFILE "/**\r\n";
print GENERATEDFILE "  * \\file      DriverSections.cpp\r\n";
print GENERATEDFILE "  * \\brief     Registration of driver files for creation and usage of XBOX\r\n";
print GENERATEDFILE "  *             loadable sections\r\n";
print GENERATEDFILE "  *\r\n";
print GENERATEDFILE "  * \\note      This file is autogenerated via Sectionize.pl DO NOT EDIT!\r\n";
print GENERATEDFILE "  */\r\n\r\n";
print GENERATEDFILE "//= I N C L U D E S ====================================================\r\n";
print GENERATEDFILE "#include <Xtl.h>\r\n";
print GENERATEDFILE "#include <stdio.h>\r\n";
print GENERATEDFILE "#include <map>\r\n";
print GENERATEDFILE "#include <string>\r\n";
print GENERATEDFILE "#include \"MAMEoX.h\"\r\n";
print GENERATEDFILE "#include \"DebugLogger.h\"\r\n";
print GENERATEDFILE "extern \"C\" {\r\n";
print GENERATEDFILE "#include \"osd_cpu.h\"\r\n";
print GENERATEDFILE "}\r\n";
print GENERATEDFILE "//= D E F I N E S ======================================================\r\n";
print GENERATEDFILE "#define DATA_PREFIX      \"".DATA_PREFIX."\"\r\n";
print GENERATEDFILE "#define CODE_PREFIX      \"".CODE_PREFIX."\"\r\n";
print GENERATEDFILE "#define BSS_PREFIX       \"".BSS_PREFIX."\"\r\n";
print GENERATEDFILE "#define CONST_PREFIX     \"".CONST_PREFIX."\"\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "//= G L O B A L = V A R S ==============================================\r\n";
print GENERATEDFILE "static std::map< std::string, std::string >  g_nameToSectionMap;\r\n";
print GENERATEDFILE "//= F U N C T I O N S ==================================================\r\n";
print GENERATEDFILE "\r\n#ifdef _DEBUG\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	CheckDriverSectionRAM\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "void CheckDriverSectionRAM( void )\r\n";
print GENERATEDFILE "{\r\n";
print GENERATEDFILE "  std::map< std::string, std::string >::iterator i = g_nameToSectionMap.begin();\r\n";
print GENERATEDFILE "  for( ; i != g_nameToSectionMap.end(); ++i )\r\n";
print GENERATEDFILE "  {\r\n";
print GENERATEDFILE "    std::string sectionName;\r\n";
print GENERATEDFILE "    sectionName = CODE_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    HANDLE h = XGetSectionHandle( sectionName.c_str() );\r\n";
print GENERATEDFILE "    if( h != INVALID_HANDLE_VALUE )\r\n";
print GENERATEDFILE "    {\r\n";
print GENERATEDFILE "      UINT32 sz = XGetSectionSize( h );\r\n";
print GENERATEDFILE "      PRINTMSG( T_INFO, \"Driver %s [CODE]: %lu\", (*i).first.c_str(), sz );\r\n";
print GENERATEDFILE "    }\r\n";
print GENERATEDFILE "    else\r\n";
print GENERATEDFILE "      PRINTMSG( T_ERROR, \"Invalid section %s for file %s!\", (*i).second.c_str(), (*i).first.c_str() );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "    sectionName = DATA_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    h = XGetSectionHandle( sectionName.c_str() );\r\n";
print GENERATEDFILE "    if( h != INVALID_HANDLE_VALUE )\r\n";
print GENERATEDFILE "    {\r\n";
print GENERATEDFILE "      UINT32 sz = XGetSectionSize( h );\r\n";
print GENERATEDFILE "      PRINTMSG( T_INFO, \"Driver %s [DATA]: %lu\", (*i).first.c_str(), sz );\r\n";
print GENERATEDFILE "    }\r\n";
print GENERATEDFILE "    else\r\n";
print GENERATEDFILE "      PRINTMSG( T_ERROR, \"Invalid section %s for file %s!\", (*i).second.c_str(), (*i).first.c_str() );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "    sectionName = BSS_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    h = XGetSectionHandle( sectionName.c_str() );\r\n";
print GENERATEDFILE "    if( h != INVALID_HANDLE_VALUE )\r\n";
print GENERATEDFILE "    {\r\n";
print GENERATEDFILE "      UINT32 sz = XGetSectionSize( h );\r\n";
print GENERATEDFILE "      PRINTMSG( T_INFO, \"Driver %s [BSS]: %lu\", (*i).first.c_str(), sz );\r\n";
print GENERATEDFILE "    }\r\n";
print GENERATEDFILE "    else\r\n";
print GENERATEDFILE "      PRINTMSG( T_ERROR, \"Invalid section %s for file %s!\", (*i).second.c_str(), (*i).first.c_str() );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "    sectionName = CONST_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    h = XGetSectionHandle( sectionName.c_str() );\r\n";
print GENERATEDFILE "    if( h != INVALID_HANDLE_VALUE )\r\n";
print GENERATEDFILE "    {\r\n";
print GENERATEDFILE "      UINT32 sz = XGetSectionSize( h );\r\n";
print GENERATEDFILE "      PRINTMSG( T_INFO, \"Driver %s [CONST]: %lu\", (*i).first.c_str(), sz );\r\n";
print GENERATEDFILE "    }\r\n";
print GENERATEDFILE "    else\r\n";
print GENERATEDFILE "      PRINTMSG( T_ERROR, \"Invalid section %s for file %s!\", (*i).second.c_str(), (*i).first.c_str() );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "    DebugLoggerFlush();\r\n";
print GENERATEDFILE "  }\r\n";
print GENERATEDFILE "}\r\n";
print GENERATEDFILE "#endif\r\n\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	RegisterSectionName\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "static void RegisterSectionName( const char *DriverFileName, const char *DataSectionName )\r\n";
print GENERATEDFILE "{\r\n";
print GENERATEDFILE "    // Add the section name to the map\r\n";
print GENERATEDFILE "  g_nameToSectionMap[ DriverFileName ] = DataSectionName;\r\n";
print GENERATEDFILE "}\r\n\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	LoadDriverSectionByName\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL LoadDriverSectionByName( const char *DriverFileName )\r\n";
print GENERATEDFILE "{\r\n";
print GENERATEDFILE "  std::map< std::string, std::string >::iterator i = g_nameToSectionMap.find( DriverFileName );\r\n";
print GENERATEDFILE "  if( i == g_nameToSectionMap.end() )\r\n";
print GENERATEDFILE "    return FALSE;\r\n";
print GENERATEDFILE "  std::string sectionName;\r\n";
print GENERATEDFILE "  void *addr;\r\n";
print GENERATEDFILE "  sectionName = DATA_PREFIX;\r\n";
print GENERATEDFILE "  sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "  addr = XLoadSection( sectionName.c_str() );\r\n";
print GENERATEDFILE "  if( !addr )\r\n";
print GENERATEDFILE "  {\r\n";
print GENERATEDFILE "    UINT32 lastErr = GetLastError();\r\n";
print GENERATEDFILE "    PRINTMSG( T_ERROR, \"XLoadSection failed! 0x%X\\r\\n\", lastErr );\r\n";
print GENERATEDFILE "    osd_print_error( \"Failed to load section %s!\", sectionName.c_str() );\r\n";
print GENERATEDFILE "    //return FALSE;\r\n";
print GENERATEDFILE "  }\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "  sectionName = CODE_PREFIX;\r\n";
print GENERATEDFILE "  sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "  addr = XLoadSection( sectionName.c_str() );\r\n";
print GENERATEDFILE "  if( !addr )\r\n";
print GENERATEDFILE "  {\r\n";
print GENERATEDFILE "    UINT32 lastErr = GetLastError();\r\n";
print GENERATEDFILE "    PRINTMSG( T_ERROR, \"XLoadSection failed! 0x%X\\r\\n\", lastErr );\r\n";
print GENERATEDFILE "    osd_print_error( \"Failed to load section %s!\", sectionName.c_str() );\r\n";
print GENERATEDFILE "    //return FALSE;\r\n";
print GENERATEDFILE "  }\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "  sectionName = BSS_PREFIX;\r\n";
print GENERATEDFILE "  sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "  addr = XLoadSection( sectionName.c_str() );\r\n";
print GENERATEDFILE "  if( !addr )\r\n";
print GENERATEDFILE "  {\r\n";
print GENERATEDFILE "    UINT32 lastErr = GetLastError();\r\n";
print GENERATEDFILE "    PRINTMSG( T_ERROR, \"XLoadSection failed! 0x%X\\r\\n\", lastErr );\r\n";
print GENERATEDFILE "    osd_print_error( \"Failed to load section %s!\", sectionName.c_str() );\r\n";
print GENERATEDFILE "    //return FALSE;\r\n";
print GENERATEDFILE "  }\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "  sectionName = CONST_PREFIX;\r\n";
print GENERATEDFILE "  sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "  addr = XLoadSection( sectionName.c_str() );\r\n";
print GENERATEDFILE "  if( !addr )\r\n";
print GENERATEDFILE "  {\r\n";
print GENERATEDFILE "    UINT32 lastErr = GetLastError();\r\n";
print GENERATEDFILE "    PRINTMSG( T_ERROR, \"XLoadSection failed! 0x%X\\r\\n\", lastErr );\r\n";
print GENERATEDFILE "    osd_print_error( \"Failed to load section %s!\", sectionName.c_str() );\r\n";
print GENERATEDFILE "    //return FALSE;\r\n";
print GENERATEDFILE "  }\r\n";
print GENERATEDFILE "  return TRUE;\r\n";
print GENERATEDFILE "}\r\n\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	UnloadDriverSectionByName\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL UnloadDriverSectionByName( const char *DriverFileName )\r\n";
print GENERATEDFILE "{\r\n";
print GENERATEDFILE "  std::map< std::string, std::string >::iterator i = g_nameToSectionMap.find( DriverFileName );\r\n";
print GENERATEDFILE "  if( i == g_nameToSectionMap.end() )\r\n";
print GENERATEDFILE "    return FALSE;\r\n";
print GENERATEDFILE "  BOOL dataRet, codeRet, bssRet, constRet;\r\n";
print GENERATEDFILE "  std::string sectionName;\r\n";
print GENERATEDFILE "  sectionName = DATA_PREFIX;\r\n";
print GENERATEDFILE "  sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "  dataRet = XFreeSection( sectionName.c_str() );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "  sectionName = CODE_PREFIX;\r\n";
print GENERATEDFILE "  sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "  codeRet = XFreeSection( sectionName.c_str() );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "  sectionName = BSS_PREFIX;\r\n";
print GENERATEDFILE "  sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "  bssRet = XFreeSection( sectionName.c_str() );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "  sectionName = CONST_PREFIX;\r\n";
print GENERATEDFILE "  sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "  constRet = XFreeSection( sectionName.c_str() );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "  return (dataRet && codeRet && bssRet && constRet);\r\n";
print GENERATEDFILE "}\r\n\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	LoadDriverDataSections\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL LoadDriverDataSections( void )\r\n";
print GENERATEDFILE "{\r\n";
print GENERATEDFILE "  std::map< std::string, std::string >::iterator i = g_nameToSectionMap.begin();\r\n";
print GENERATEDFILE "  for( ; i != g_nameToSectionMap.end(); ++i )\r\n";
print GENERATEDFILE "  {\r\n";
print GENERATEDFILE "    std::string sectionName;\r\n";
print GENERATEDFILE "    sectionName = DATA_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    if( !XLoadSection( sectionName.c_str() ) )\r\n";
print GENERATEDFILE "    {\r\n";
print GENERATEDFILE "      PRINTMSG( T_ERROR, \"Failed to load section %s!\", sectionName.c_str() );\r\n";
print GENERATEDFILE "      //return FALSE;\r\n";
print GENERATEDFILE "    }\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "    sectionName = CONST_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    if( !XLoadSection( sectionName.c_str() ) )\r\n";
print GENERATEDFILE "    {\r\n";
print GENERATEDFILE "      PRINTMSG( T_ERROR, \"Failed to load section %s!\", sectionName.c_str() );\r\n";
print GENERATEDFILE "      //return FALSE;\r\n";
print GENERATEDFILE "    }\r\n";
print GENERATEDFILE "  }\r\n";
print GENERATEDFILE "  return TRUE;\r\n";
print GENERATEDFILE "}\r\n\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	UnloadDriverNonDataSections\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL UnloadDriverNonDataSections( void )\r\n";
print GENERATEDFILE "{\r\n";
print GENERATEDFILE "  std::map< std::string, std::string >::iterator i = g_nameToSectionMap.begin();\r\n";
print GENERATEDFILE "  for( ; i != g_nameToSectionMap.end(); ++i )\r\n";
print GENERATEDFILE "  {\r\n";
print GENERATEDFILE "    std::string sectionName;\r\n";
print GENERATEDFILE "    sectionName = CODE_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    HANDLE h = XGetSectionHandle( sectionName.c_str() );\r\n";
print GENERATEDFILE "    if( h != INVALID_HANDLE_VALUE )\r\n";
print GENERATEDFILE "      XFreeSectionByHandle( h );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "    sectionName = BSS_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    h = XGetSectionHandle( sectionName.c_str() );\r\n";
print GENERATEDFILE "    if( h != INVALID_HANDLE_VALUE )\r\n";
print GENERATEDFILE "      XFreeSectionByHandle( h );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "  }\r\n";
print GENERATEDFILE "  return TRUE;\r\n";
print GENERATEDFILE "}\r\n\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	UnloadDriverDataSections\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "BOOL UnloadDriverDataSections( void )\r\n";
print GENERATEDFILE "{\r\n";
print GENERATEDFILE "  std::map< std::string, std::string >::iterator i = g_nameToSectionMap.begin();\r\n";
print GENERATEDFILE "  for( ; i != g_nameToSectionMap.end(); ++i )\r\n";
print GENERATEDFILE "  {\r\n";
print GENERATEDFILE "    std::string sectionName;\r\n";
print GENERATEDFILE "    sectionName = DATA_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    HANDLE h = XGetSectionHandle( sectionName.c_str() );\r\n";
print GENERATEDFILE "    if( h != INVALID_HANDLE_VALUE )\r\n";
print GENERATEDFILE "      XFreeSectionByHandle( h );\r\n";
print GENERATEDFILE "\r\n";
print GENERATEDFILE "    sectionName = CONST_PREFIX;\r\n";
print GENERATEDFILE "    sectionName += (*i).second.c_str();\r\n";
print GENERATEDFILE "    h = XGetSectionHandle( sectionName.c_str() );\r\n";
print GENERATEDFILE "    if( h != INVALID_HANDLE_VALUE )\r\n";
print GENERATEDFILE "      XFreeSectionByHandle( h );\r\n";
print GENERATEDFILE "  }\r\n";
print GENERATEDFILE "  return TRUE;\r\n";
print GENERATEDFILE "}\r\n\r\n";
print GENERATEDFILE "\r\n//-------------------------------------------------------------\r\n";
print GENERATEDFILE "//	RegisterDriverSectionNames\r\n";
print GENERATEDFILE "//-------------------------------------------------------------\r\n";
print GENERATEDFILE "void RegisterDriverSectionNames( void )\r\n";
print GENERATEDFILE "{\r\n";

# Auto generate an 8 character unique name for each driver file
$autoNameNumber = 0;
foreach( @FILEs ) {
	chomp( $_ );

		# Skip the fake jrcrypt.c file
	if( !($_ =~ /.*jrcrypt\.c$/) ) {
		$DriverFileName = $_;
		$DriverFileName = $_;

		$DriverName = $_;
			# Change the DriverName to what will be present in the actual MAME code

			# Drop the ./MAME/ portion
		$DriverName =~ /^\.\/MAME\/src\/drivers\/(.*\.c)$/;
		$DriverName = "src\\\\drivers\\\\$1";

		print PRELOADFILE "/NOPRELOAD:\"".CODE_PREFIX."$autoNameNumber\"\r\n";
		print PRELOADFILE "/NOPRELOAD:\"".BSS_PREFIX."$autoNameNumber\"\r\n";
#		print PRELOADFILE "/NOPRELOAD:\"".DATA_PREFIX."$autoNameNumber\"\r\n";
#		print PRELOADFILE "/NOPRELOAD:\"".CONST_PREFIX."$autoNameNumber\"\r\n";
		print GENERATEDFILE "  RegisterSectionName( \"$DriverName\", \"$autoNameNumber\" );\r\n";

		($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
		 $atime,$mtime,$ctime,$blksize,$blocks) = stat( $DriverFileName );

		open( FILE, "<$DriverFileName" ) || die "Failed to open file $DriverFileName!\r\n";
		sysread( FILE, $File, $size );
		close( FILE );

		if( !($File =~ /\#pragma code_seg/) ) {
			WriteSectionData( $DriverFileName, $File, $autoNameNumber );
		}


			# Also do the vidhdrw file, if one exists
		$VidHardwareName = $DriverFileName;
		$VidHardwareName =~ s/\/drivers\//\/vidhrdw\//;

		($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
		 $atime,$mtime,$ctime,$blksize,$blocks) = stat( $VidHardwareName );

		if( open( FILE, "<$VidHardwareName" ) ) {
			$File = "";
			sysread( FILE, $File, $size );
			close( FILE );

			if( !($File =~ /\#pragma code_seg/) ) {
				WriteSectionData( $VidHardwareName, $File, $autoNameNumber );
			}
		}

			# Also do the sndhrdw file, if one exists
		$SoundHardwareName = $DriverFileName;
		$SoundHardwareName =~ s/\/drivers\//\/sndhrdw\//;

		($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
		 $atime,$mtime,$ctime,$blksize,$blocks) = stat( $SoundHardwareName );

		if( open( FILE, "<$SoundHardwareName" ) ) {
			$File = "";
			sysread( FILE, $File, $size );
			close( FILE );

			if( !($File =~ /\#pragma code_seg/) ) {
				WriteSectionData( $SoundHardwareName, $File, $autoNameNumber );
			}
		}

		$autoNameNumber++;
	}
}
print GENERATEDFILE "}\r\n\r\n\r\n";


close( GENERATEDFILE );
close( PRELOADFILE );


#------------------------------------------------------------------------
#	WriteSectionData
#------------------------------------------------------------------------
sub WriteSectionData( $$$ ) {
	my $FileName = $_[0];
	my $File = $_[1];
	my $autoNameNumber = $_[2];
	my $DataSectionName = "\"D$autoNameNumber\"";
	my $CodeSectionName = "\"C$autoNameNumber\"";
	my $BSSSectionName = "\"B$autoNameNumber\"";
	my $ConstSectionName= "\"K$autoNameNumber\"";

	open( FILE, ">$FileName" ) || die "Could not open %s for output!\r\n";

		#open the segment
	my $SegLine = "#pragma code_seg($CodeSectionName)\r\n";
	syswrite( FILE, $SegLine, length($SegLine) );
	$SegLine = "#pragma bss_seg($BSSSectionName)\r\n";
	syswrite( FILE, $SegLine, length($SegLine) );
	$SegLine = "#pragma data_seg($DataSectionName)\r\n";
				syswrite( FILE, $SegLine, length($SegLine) );
	$SegLine = "#pragma const_seg($ConstSectionName)\r\n";
				syswrite( FILE, $SegLine, length($SegLine) );

		#write the old file data
	syswrite( FILE, $File, $size );

		#Close the segment
	$SegLine = "#pragma data_seg()\r\n";
	syswrite( FILE, $SegLine, length($SegLine) );
	$SegLine = "#pragma code_seg()\r\n";
	syswrite( FILE, $SegLine, length($SegLine) );
	$SegLine = "#pragma bss_seg()\r\n";
	syswrite( FILE, $SegLine, length($SegLine) );
	$SegLine = "#pragma const_seg()\r\n";
	syswrite( FILE, $SegLine, length($SegLine) );

	close( FILE );
}
