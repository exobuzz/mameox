/**
  * \file      Sections.h
  * \brief     Registration of MAME files for creation and usage of XBOX
  *             loadable sections.
  *
  * \note      This file is autogenerated via Sectionize.pl DO NOT EDIT!
  */
#pragma once
//= I N C L U D E S ====================================================
#ifdef __cplusplus
extern "C" {
#endif
#include "osd_cpu.h"
#ifdef __cplusplus
}
#endif
//= P R O T O T Y P E S ================================================

#ifdef _DEBUG
//-------------------------------------------------------------
//	CheckDriverSectionRAM
//! \brief    Prints the size of each segment
//-------------------------------------------------------------
void CheckDriverSectionRAM( void );
#endif


//-------------------------------------------------------------
//	RegisterDriverSectionNames
//! \brief    Registers all of the segments for use by
//!            LoadDriverSectionByName and UnloadDriverSectionByName
//-------------------------------------------------------------
void RegisterDriverSectionNames( void );

//-------------------------------------------------------------
//	LoadDriverSectionByName
//! \brief    Loads the section associated with the passed name
//!
//! \param    DriverFileName - The name of the file whose section
//!                             should be loaded
//!
//! \return   BOOL - Operation status
//! \retval   TRUE - success
//! \return   FALSE - Failure
//-------------------------------------------------------------
BOOL LoadDriverSectionByName( const char *DriverFileName );

//-------------------------------------------------------------
//	UnloadDriverSectionByName
//! \brief    Unloads the section associated with the passed name
//!
//! \param    DriverFileName - The name of the file whose section
//!                             should be unloaded
//!
//! \return   BOOL - Operation status
//! \retval   TRUE - success
//! \return   FALSE - Failure
//-------------------------------------------------------------
BOOL UnloadDriverSectionByName( const char *DriverFileName );

//-------------------------------------------------------------
//	LoadDriverDataSections
//! \brief    Loads all of the driver data sections
//!
//! \return   BOOL - Operation status
//! \retval   TRUE - success
//! \return   FALSE - Failure
//-------------------------------------------------------------
BOOL LoadDriverDataSections( void );

//-------------------------------------------------------------
//	LoadDriverDataSections
//! \brief    Unloads all of the driver data sections
//!
//! \return   BOOL - Operation status
//! \retval   TRUE - success
//! \return   FALSE - Failure
//-------------------------------------------------------------
BOOL UnloadDriverDataSections( void );

//-------------------------------------------------------------
//	UnloadDriverNonDataSections
//! \brief    Unloads all non-data driver sections
//!
//! \return   BOOL - Operation status
//! \retval   TRUE - success
//! \return   FALSE - Failure
//-------------------------------------------------------------
BOOL UnloadDriverNonDataSections( void );
