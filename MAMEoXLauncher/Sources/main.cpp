/**
	* \file			xbox_Main.cpp
	* \brief		MAMEoX entry point
	*
	*/


//= I N C L U D E S ====================================================
#include <Xtl.h>
#include <XGraphics.h>
#include <stdarg.h>
#include <stdio.h>
#include <crtdbg.h>

#ifdef _PROFILER
#include <xbdm.h>
#endif

#include "MAMEoX.h"
#include "Resource.h"

#include "InputManager.h"
#include "GraphicsManager.h"
#include "ROMList.h"
#include "Help.h"
#include "OptionsPage.h"
#include "System_IniFile.h"
#include "xbox_FileIO.h"
#include "xbox_Direct3DRenderer.h"
#include "xbox_Timing.h"
#include "DebugLogger.h"
#include "xbox_Network.h"

	// Font class from the XDK
#include "XBFont.h"

//= D E F I N E S =====================================================

	// Number of seconds between valid X button readings
#define TOGGLEBUTTON_TIMEOUT	0.50f

  // The deadzone for the screen usage percentage control (right analog)
#define SCREENRANGE_DEADZONE    15000

//= S T R U C T U R E S ===============================================
struct CUSTOMVERTEX
{
	D3DXVECTOR3   pos;      // The transformed position for the vertex
  FLOAT         tu, tv;   // The texture coordinates
};

//= G L O B A L = V A R S =============================================
extern CInputManager			g_inputManager;
extern CGraphicsManager	  g_graphicsManager;
extern CXBFont						g_font;
extern CXBFont            g_fixedWidthFont;

  // XBE Launch data
static DWORD              g_launchDataType;
static LAUNCH_DATA        g_launchData;

BYTE                      *g_pResourceSysMemData = NULL;
BYTE                      *g_pResourceVidMemData = NULL;

  // Defines the percentage of the total screen area that should actually be used
  // This is required because TV's have some overscan area that is not actually
  // visible
static FLOAT                      g_screenXPercentage = DEFAULT_SCREEN_X_PERCENTAGE;
static FLOAT                      g_screenYPercentage = DEFAULT_SCREEN_Y_PERCENTAGE;

static LPDIRECT3DVERTEXBUFFER8    g_pD3DVertexBuffer = NULL;

  //! The data for each driver
static MAMEDriverData_t   *g_driverData = NULL;


extern "C" {
  // Fake options struct for load/store options
GameOptions options;

  // Fake cheatfile name for load/store options
const char *cheatfile = NULL;
}
  
  // Fake D3D renderer options
RendererOptions_t    g_rendererOptions;

//= P R O T O T Y P E S ===============================================
BOOL CreateBackdrop( FLOAT xUsage, FLOAT yUsage );
void DestroyBackdrop( void );
void Die( LPDIRECT3DDEVICE8 pD3DDevice, const char *fmt, ... );
static BOOL Helper_LoadDriverInfoFile( void );
static HRESULT LoadPackedResources( void );
static void ShowSplashScreen( LPDIRECT3DDEVICE8 pD3DDevice );

//= F U N C T I O N S =================================================

#define TEST_LIGHTGUN 0
#if TEST_LIGHTGUN

static UINT32                   g_calibrationStep = 0;
static UINT32                   g_calibrationJoynum = 0;

void osd_joystick_start_calibration( void )
{
/* Preprocessing for joystick calibration. Returns 0 on success */
  const XINPUT_CAPABILITIES *gp;
  UINT32 i = 0;

  g_calibrationStep = 0;

    // Search for the first connected gun
  for( ; i < 4; ++i )
  {
    gp = GetGamepadCaps( 0 );  
    if( gp && gp->SubType == XINPUT_DEVSUBTYPE_GC_LIGHTGUN )
    {
      g_calibrationJoynum = i;
      return;
    }
  }
}

const char *osd_joystick_calibrate_next( void )
{
/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
  char retString[128];

    // When we hit 3, switch over to the next gun to be calibrated,
    //  or return NULL to exit the process
  if( g_calibrationStep == 3 )
  {
    const XINPUT_CAPABILITIES *gp;
    ++g_calibrationJoynum;
    for( ; g_calibrationJoynum < 4; ++g_calibrationJoynum )
    {
      gp = GetGamepadCaps( g_calibrationJoynum );
      if( gp && gp->SubType == XINPUT_DEVSUBTYPE_GC_LIGHTGUN )
      {
          // Found another gun
        g_calibrationStep = 0;
        break;
      }
    }

    if( g_calibrationJoynum == 4 )
      return NULL;
  }

  sprintf( retString, "Gun %d: ", g_calibrationJoynum + 1 );
  switch( g_calibrationStep++ )
  {
  case 0:
    strcat( retString, "Upper left" );
    break;

  case 1:
    strcat( retString, "Center" );
    break;

  case 2:
    strcat( retString, "Lower right" );
    break;
  }

	return retString;
}

void osd_joystick_calibrate( void )
{
/* Get the actual joystick calibration data for the current position */

  if( g_calibrationStep && g_calibrationStep < 4 )
  {
	  const XINPUT_GAMEPAD *gp;
    if( (gp = GetGamepadState( g_calibrationJoynum )) )
    {
      g_calibrationData[g_calibrationJoynum].m_xData[g_calibrationStep-1] = gp->sThumbLX;
      g_calibrationData[g_calibrationJoynum].m_yData[g_calibrationStep-1] = gp->sThumbLY;
    }
    PRINTMSG( T_INFO, "CALIB: STEP %d: %d, %d\n", g_calibrationStep - 1, gp->sThumbLX, gp->sThumbLY );
  }
}

void osd_joystick_end_calibration( void )
{
/* Postprocessing (e.g. saving joystick data to config) */
  UINT32 i = 0;

  for( ; i < 3; ++i )
  {
    g_calibrationData[i].m_xData[0] -= g_calibrationData[i].m_xData[1];
    g_calibrationData[i].m_xData[2] -= g_calibrationData[i].m_xData[1];
    g_calibrationData[i].m_xData[0] *= -1;  //!< Negate so that < 0 values stay < 0

    g_calibrationData[i].m_yData[0] -= g_calibrationData[i].m_yData[1];
    g_calibrationData[i].m_yData[2] -= g_calibrationData[i].m_yData[1];
    g_calibrationData[i].m_yData[2] *= -1;  //!< Negate so that < 0 values stay < 0
  }
}

void osd_lightgun_read(int player, int *deltax, int *deltay)
{
	const XINPUT_GAMEPAD *gp;

	if( (gp = GetGamepadState( player )) )
  {
    lightgunCalibration_t *calibData = &g_calibrationData[player];

    *deltax = gp->sThumbLX - calibData->m_xData[1];
    *deltay = -1 * (gp->sThumbLY - calibData->m_yData[1]);

      // Map from -128 to 128
    if( gp->sThumbLX < 0 )
      *deltax = (int)((FLOAT)*deltax * 128.0f / ((FLOAT)calibData->m_xData[0]+1.0f));
    else
      *deltax = (int)((FLOAT)*deltax * 128.0f / ((FLOAT)calibData->m_xData[2]+1.0f));

    if( gp->sThumbLY > 0 )
      *deltay = (int)((FLOAT)*deltay * 128.0f / ((FLOAT)calibData->m_yData[0]+1.0f));
    else
      *deltay = (int)((FLOAT)*deltay * 128.0f / ((FLOAT)calibData->m_yData[2]+1.0f));

      // Lock to the expected range
    if( *deltax > 128 )
      *deltax = 128;
    else if( *deltax < -128 )
      *deltax = -128;

    if( *deltay > 128 )
      *deltay = 128;
    else if( *deltay < -128 )
      *deltay = -128;
  }
  else  
	  *deltax = *deltay = 0;
}

#endif



//-------------------------------------------------------------
//	main
//-------------------------------------------------------------
void __cdecl main( void )
{
		// Start up the debug logger thread
	DebugLoggerInit();

		// Initialize the graphics subsystem
	g_graphicsManager.Create();
	LPDIRECT3DDEVICE8 pD3DDevice = g_graphicsManager.GetD3DDevice();

		// Create a general purpose font
// VC6 requires the 2 paramater call to create. _VC6 is defined in the VC6 dsp files
#ifdef _VC6
	g_font.Create( pD3DDevice, "FontAN12.xpr" );
  g_fixedWidthFont.Create( pD3DDevice, "FontCN10.xpr" );
#else
	g_font.Create( "FontAN12.xpr", 0 );
  g_fixedWidthFont.Create( "FontCN10.xpr" );
#endif

  LoadOptions();

		// Initialize the input subsystem
	g_inputManager.Create( 4, 0 );  // 4 controllers, no memory cards


	// Intialize the various MAME OSD-specific subsystems
	InitializeFileIO();
	InitializeTiming();

  if( !g_NetworkConfig.m_networkDisabled )
    InitializeNetwork();
  else
  {
      // If any paths have been set to samba, revert them
      // to the defaults
    if( g_FileIOConfig.m_ArtPath.Left(6) == "smb://" )
      g_FileIOConfig.m_ArtPath = DEFAULT_ARTPATH;

    if( g_FileIOConfig.m_AudioPath.Left(6) == "smb://" )
      g_FileIOConfig.m_AudioPath = DEFAULT_AUDIOPATH;
    
    if( g_FileIOConfig.m_ConfigPath.Left(6) == "smb://" )
      g_FileIOConfig.m_ConfigPath = DEFAULT_CONFIGPATH;
    
    if( g_FileIOConfig.m_DefaultRomListPath.Left(6) == "smb://" )
      g_FileIOConfig.m_DefaultRomListPath = DEFAULT_DEFAULTROMLISTPATH;
    
    if( g_FileIOConfig.m_GeneralPath.Left(6) == "smb://" )
      g_FileIOConfig.m_GeneralPath = DEFAULT_GENERALPATH;
    
    if( g_FileIOConfig.m_HDImagePath.Left(6) == "smb://" )
      g_FileIOConfig.m_HDImagePath = DEFAULT_HDIMAGEPATH;
    
    if( g_FileIOConfig.m_HiScorePath.Left(6) == "smb://" )
      g_FileIOConfig.m_HiScorePath = DEFAULT_HISCOREPATH;
    
    if( g_FileIOConfig.m_NVramPath.Left(6) == "smb://" )
      g_FileIOConfig.m_NVramPath = DEFAULT_NVRAMPATH;
    
    if( g_FileIOConfig.m_RomBackupPath.Left(6) == "smb://" )
      g_FileIOConfig.m_RomBackupPath = DEFAULT_ROMBACKUPPATH;
    
    if( g_FileIOConfig.m_RomPath.Left(6) == "smb://" )
      g_FileIOConfig.m_RomPath = DEFAULT_ROMPATH;
  }

  SaveOptions(); 


  if( FAILED( LoadPackedResources() ) )
  {
    Die( pD3DDevice, 
         "The Media/Resource.xpr file is missing or damaged.\n"
         "The original file may be found in the Media directory\n"
         "of the MAMEoX package (it is autogenerated\n"
         "during the build process, so you may have to rebuild\n"
         "to obtain it.\n"
         "Please place the correct file in the Media\n"
         "directory on your XBOX and restart." );
  }

    // Get the launch data
  DWORD ret = XGetLaunchInfo( &g_launchDataType, &g_launchData );
  MAMEoXLaunchData_t *mameoxLaunchData = (MAMEoXLaunchData_t*)g_launchData.Data;

    // See who launched us
  if( ret != ERROR_SUCCESS || 
      g_launchDataType != LDT_TITLE || 
      mameoxLaunchData->m_command == LAUNCH_RUN_AS_IF_REBOOTED )
  {
		  // Show a splash screen if we were started from the dashboard and not from MAMEoX
	  ShowSplashScreen( pD3DDevice );

      // Create the MAME driver list
    memset( mameoxLaunchData, 0, sizeof(*mameoxLaunchData) );
    mameoxLaunchData->m_command = LAUNCH_CREATE_MAME_GAME_LIST;

      // Try to load the MAME driver info, asking MAMEoX to produce
      // it if we can't
    if( !Helper_LoadDriverInfoFile() )
    {
      ShowLoadingScreen( pD3DDevice );
      XLaunchNewImage( "D:\\MAMEoX.xbe", &g_launchData );
		  Die( pD3DDevice, "Could not execute MAMEoX.xbe!" );
    }
  }
  else
  {
      // Load the driver info file
    if( !Helper_LoadDriverInfoFile() )
      Die( pD3DDevice, "Could not load driver info file!" );
  }

  // At this point the MAMEoX process is guaranteed to have run, setting up
  // our totalMAMEGames member, as well as producing the driver info file

    // Wait for controller 0 to be inserted
  RequireController( 0 );
  CGamepad *gp0 = g_inputManager.GetGamepad( 0 );

#if TEST_LIGHTGUN

    // Create the vertex buffer
  g_graphicsManager.GetD3DDevice()->CreateVertexBuffer( (sizeof(CUSTOMVERTEX) << 2),
																		                    D3DUSAGE_WRITEONLY,
																		                    D3DFVF_XYZ | D3DFVF_TEX1,
																		                    D3DPOOL_MANAGED,
																		                    &g_pD3DVertexBuffer );

	CUSTOMVERTEX *pVertices;


  while( 1 )
  {
  	WCHAR wBuf[1024];
    WCHAR calibBuf[1024];

		g_inputManager.PollDevices();
    RequireController( 0 );
	  gp0 = g_inputManager.GetGamepadDeviceState( 0 );

    pD3DDevice->Clear(	0L,																// Count
											  NULL,															// Rects to clear
											  D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,	// Flags
                        D3DCOLOR_XRGB(105,105,105),				// Color
											  1.0f,															// Z
											  0L );															// Stencil

		g_pD3DVertexBuffer->Lock( 0,										// Offset to lock
															0,										// Size to lock
															(BYTE**)&pVertices,		// ppbData
															0 );									// Flags

    FLOAT lx = (FLOAT)((gp0->sThumbLX - 300)) / 32767.0f;
    FLOAT rx = (FLOAT)((gp0->sThumbLX + 300)) / 32767.0f;

    FLOAT ty = (FLOAT)((gp0->sThumbLY - 300)) / 32767.0f;
    FLOAT by = (FLOAT)((gp0->sThumbLY + 300)) / 32767.0f;

      //-- Draw the backdrop -------------------------------------------------
		pVertices[0].pos.x = lx;
		pVertices[0].pos.y = ty;
		pVertices[0].pos.z = 1.0f;
    pVertices[0].tu = 0.0f;
    pVertices[0].tv = 0.0f;

		pVertices[1].pos.x = rx;
		pVertices[1].pos.y = ty;
		pVertices[1].pos.z = 1.0f;
    pVertices[1].tu = 1.0f;
    pVertices[1].tv = 0.0f;
		
		pVertices[2].pos.x = rx;
		pVertices[2].pos.y = by;
		pVertices[2].pos.z = 1.0f;
    pVertices[2].tu = 1.0f;
    pVertices[2].tv = 1.0f;
		
		pVertices[3].pos.x = lx;
		pVertices[3].pos.y = by;
		pVertices[3].pos.z = 1.0f;
    pVertices[3].tu = 0.0f;
    pVertices[3].tv = 1.0f;

    g_pD3DVertexBuffer->Unlock();

    g_font.Begin();

    if( gp0->bAnalogButtons[XINPUT_GAMEPAD_B] > 10 )
    {
      osd_joystick_start_calibration();
      WaitForNoButton();
      const char *ptr = osd_joystick_calibrate_next();
      if( !ptr )
        osd_joystick_end_calibration();
      else
     	  mbstowcs( calibBuf, ptr, strlen(ptr) + 1 );
    }
    if( gp0->bAnalogButtons[XINPUT_GAMEPAD_A] > 10 && g_calibrationStep )
    {
      osd_joystick_calibrate();
      WaitForNoButton();
      const char *ptr = osd_joystick_calibrate_next();
      if( !ptr )
        osd_joystick_end_calibration();
      else
     	  mbstowcs( calibBuf, ptr, strlen(ptr) + 1 );
    }
    if( gp0->bAnalogButtons[XINPUT_GAMEPAD_X] > 10 )
    {
      SaveOptions();
    }

      g_font.DrawText( 320, 180, D3DCOLOR_RGBA( 255, 255, 255, 255), calibBuf, XBFONT_CENTER_X );

      swprintf( wBuf, L"LX: %d", gp0->sThumbLX );
	    g_font.DrawText( 320, 80, D3DCOLOR_RGBA( 255, 255, 255, 255), wBuf, XBFONT_CENTER_X );

      swprintf( wBuf, L"LY: %d", gp0->sThumbLY );
	    g_font.DrawText( 320, 100, D3DCOLOR_RGBA( 255, 255, 255, 255), wBuf, XBFONT_CENTER_X );



      int deltaX, deltaY;
      osd_lightgun_read( 0, &deltaX, &deltaY );

      swprintf( wBuf, L"LX: %d", deltaX );
	    g_font.DrawText( 320, 280, D3DCOLOR_RGBA( 255, 255, 255, 255), wBuf, XBFONT_CENTER_X );

      swprintf( wBuf, L"LY: %d", deltaY );
	    g_font.DrawText( 320, 300, D3DCOLOR_RGBA( 255, 255, 255, 255), wBuf, XBFONT_CENTER_X );

    g_font.End();

    pD3DDevice->SetVertexShader( D3DFVF_XYZ | D3DFVF_TEX1 );
    pD3DDevice->SetStreamSource(	0,												  // Stream number
																	g_pD3DVertexBuffer,					// Stream data
																	sizeof(CUSTOMVERTEX) );		  // Vertex stride

    pD3DDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

    g_graphicsManager.GetD3DDevice()->Present( NULL, NULL, NULL, NULL );
  }
#endif




		// Load the Help file
  CHelp help( pD3DDevice, g_fixedWidthFont );
	if( !help.LoadHelpFile() )
  {
    Die( pD3DDevice, 
         "The Media/help.txt file is missing or damaged.\n"
         "The original file may be found in the Media directory\n"
         "of the MAMEoX package.\n"
         "Please place the correct file in the Media\n"
         "directory on your XBOX and restart." );
  }

		// Load/Generate the ROM listing
  CROMList romList( pD3DDevice, 
                    g_font, 
                    g_driverData, 
                    mameoxLaunchData->m_totalMAMEGames,
                    &g_launchData );
	if( !romList.LoadROMList( TRUE ) )
		Die( pD3DDevice, "Could not generate ROM list!" );

    // Reposition the romList cursor to its previous value
  romList.SetCursorPosition(  mameoxLaunchData->m_cursorPosition, 
                              mameoxLaunchData->m_pageOffset,
                              mameoxLaunchData->m_superscrollIndex );

    // Grab the current screen usage so we can render a border
  FLOAT xPercentage, yPercentage;
  GetScreenUsage( &xPercentage, &yPercentage );
  CreateBackdrop( xPercentage, yPercentage );

  COptionsPage optionsPage( pD3DDevice,
                            g_font,
                            options );

    //-- Initialize the rendering engine -------------------------------
  D3DXMATRIX matWorld;
  D3DXMatrixIdentity( &matWorld );

    // Create a texture for the render target
  LPDIRECT3DTEXTURE8 renderTargetTexture = NULL;
  if( FAILED( D3DXCreateTexture(  pD3DDevice,
                                  512, 
                                  512, 
                                  1, 
                                  0L, 
                                  D3DFMT_LIN_X8R8G8B8, 
                                  D3DPOOL_DEFAULT, 
                                  &renderTargetTexture ) ) )
  {
    Die( pD3DDevice, "Fatal error while creating render target texture!" );
  }

  LPDIRECT3DVERTEXBUFFER8    backdropVertexBuffer = NULL;

    // Create a vertex buffer to render the backdrop image to the renderTargetTexture
  pD3DDevice->CreateVertexBuffer( (sizeof(CUSTOMVERTEX) << 2),
																  D3DUSAGE_WRITEONLY,
																	D3DFVF_XYZ | D3DFVF_TEX1,
																	D3DPOOL_MANAGED,
																	&backdropVertexBuffer );

	CUSTOMVERTEX *pVertices;
	backdropVertexBuffer->Lock( 0,										// Offset to lock
														  0,										// Size to lock
														  (BYTE**)&pVertices,		// ppbData
														  0 );									// Flags

		pVertices[0].pos.x = -1.0f;
		pVertices[0].pos.y = 1.0f;
		pVertices[0].pos.z = 1.0f;
    pVertices[0].tu = 0.0f;
    pVertices[0].tv = 0.0f;

		pVertices[1].pos.x = 1.0f;
		pVertices[1].pos.y = 1.0f;
		pVertices[1].pos.z = 1.0f;
    pVertices[1].tu = 1.0f;
    pVertices[1].tv = 0.0f;
		
		pVertices[2].pos.x = 1.0f;
		pVertices[2].pos.y = -1.0f;
		pVertices[2].pos.z = 1.0f;
    pVertices[2].tu = 1.0f;
    pVertices[2].tv = 1.0f;
		
		pVertices[3].pos.x = -1.0f;
		pVertices[3].pos.y = -1.0f;
		pVertices[3].pos.z = 1.0f;
    pVertices[3].tu = 0.0f;
    pVertices[3].tv = 1.0f;

  backdropVertexBuffer->Unlock();


    // Toggle for whether or not we're in a given mode
  BOOL optionsMode = FALSE;
  BOOL helpMode = FALSE;
	FLOAT toggleButtonTimeout = 0.0f;
	UINT64 lastTime = osd_cycles();


    // Store the current screen rotation value so we can
    // rotate as soon as we notice that the option is changed
  screenrotation_t oldRotation = g_rendererOptions.m_screenRotation;


		//--- Main loop ------------------------------------------------------
	while( 1 )
	{
		g_inputManager.PollDevices();
    RequireController( 0 );
	  gp0 = g_inputManager.GetGamepad( 0 );

	  UINT64 curTime = osd_cycles();
	  FLOAT elapsedTime = (FLOAT)(curTime - lastTime) / (FLOAT)osd_cycles_per_second();
	  lastTime = curTime;

		  // Decrement the dpad movement timer
	  if( toggleButtonTimeout > 0.0f )
	  {
		  toggleButtonTimeout -= elapsedTime;
		  if( toggleButtonTimeout < 0.0f )
			  toggleButtonTimeout = 0.0f;
	  }


			// Reboot on LT+RT+Black
    if( gp0->IsButtonPressed( GP_LEFT_TRIGGER | GP_RIGHT_TRIGGER | GP_BLACK ) )
		{
      SaveOptions();
      LD_LAUNCH_DASHBOARD LaunchData = { XLD_LAUNCH_DASHBOARD_MAIN_MENU };
      XLaunchNewImage( NULL, (LAUNCH_DATA*)&LaunchData );
		}



    if( gp0->IsButtonPressed( GP_B | GP_Y ) && toggleButtonTimeout == 0.0f )
    {
        // Toggle options mode
      optionsMode = !optionsMode;
      toggleButtonTimeout = TOGGLEBUTTON_TIMEOUT;
    }
    else if(  gp0->IsButtonPressed( GP_X ) && toggleButtonTimeout == 0.0f )
    {
        // Toggle help mode
      helpMode = !helpMode;
      toggleButtonTimeout = TOGGLEBUTTON_TIMEOUT;
    }
    else if(  gp0->GetAnalogAxisState( GP_ANALOG_RIGHT, GP_AXIS_X ) < -SCREENRANGE_DEADZONE || 
              gp0->GetAnalogAxisState( GP_ANALOG_RIGHT, GP_AXIS_X ) > SCREENRANGE_DEADZONE || 
              gp0->GetAnalogAxisState( GP_ANALOG_RIGHT, GP_AXIS_Y ) < -SCREENRANGE_DEADZONE || 
              gp0->GetAnalogAxisState( GP_ANALOG_RIGHT, GP_AXIS_Y ) > SCREENRANGE_DEADZONE ||
              oldRotation != g_rendererOptions.m_screenRotation )
    {
      FLOAT xPercentage, yPercentage;
      GetScreenUsage( &xPercentage, &yPercentage );

      if( gp0->GetAnalogAxisState( GP_ANALOG_RIGHT, GP_AXIS_X ) < -SCREENRANGE_DEADZONE )
        xPercentage -= 0.00025f;
      else if( gp0->GetAnalogAxisState( GP_ANALOG_RIGHT, GP_AXIS_X ) > SCREENRANGE_DEADZONE )
        xPercentage += 0.00025f;

      if( gp0->GetAnalogAxisState( GP_ANALOG_RIGHT, GP_AXIS_Y ) < -SCREENRANGE_DEADZONE )
        yPercentage -= 0.00025f;
      else if( gp0->GetAnalogAxisState( GP_ANALOG_RIGHT, GP_AXIS_Y ) > SCREENRANGE_DEADZONE )
        yPercentage += 0.00025f;

      if( xPercentage < 0.25f )
        xPercentage = 0.25f;
      else if( xPercentage > 1.0f )
        xPercentage = 1.0f;

      if( yPercentage < 0.25f )
        yPercentage = 0.25f;
      else if( yPercentage > 1.0f )
        yPercentage = 1.0f;

      SetScreenUsage( xPercentage, yPercentage );
      DestroyBackdrop();
      CreateBackdrop( xPercentage, yPercentage );
      oldRotation = g_rendererOptions.m_screenRotation;
    }
		

      // Set up to render to texture
    RenderToTextureToken_t token;
    D3DVIEWPORT8 vp = { 0, 0, 512, 512, 0.0f, 1.0f };
    RenderToTextureStart( token, pD3DDevice, renderTargetTexture, vp );

			// Move the cursor position and render
    pD3DDevice->SetTransform( D3DTS_WORLD, &matWorld );
    pD3DDevice->SetTransform( D3DTS_VIEW, &matWorld );
    pD3DDevice->SetTransform( D3DTS_PROJECTION, &matWorld );

	  pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	  pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	  pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );

    pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    
      // Render whatever screen happens to be active to the
      // texture
    pD3DDevice->Clear(	0L,																// Count
											  NULL,															// Rects to clear
											  D3DCLEAR_TARGET,	                // Flags
                        D3DCOLOR_XRGB(105,105,105),				// Color
											  1.0f,															// Z
											  0L );															// Stencil

	  pD3DDevice->SetTexture( 0, (LPDIRECT3DTEXTURE8)&g_pResourceSysMemData[resource_ROMListBackdrop_OFFSET] );

    pD3DDevice->SetVertexShader( D3DFVF_XYZ | D3DFVF_TEX1 );
    pD3DDevice->SetStreamSource(	0,												  // Stream number
																	backdropVertexBuffer,	      // Stream data
																	sizeof(CUSTOMVERTEX) );		  // Vertex stride

    pD3DDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

    if( optionsMode )
    {
      optionsPage.MoveCursor( *gp0 );
      optionsPage.Draw( FALSE, FALSE );
    }
    else if( helpMode )
    {
      help.MoveCursor( *gp0 );
      help.Draw( FALSE, FALSE );
    }
    else
    {
		  romList.MoveCursor( *gp0 );
		  romList.Draw( FALSE, FALSE );
    }


      // Restore the render target
    RenderToTextureEnd( token );

      // Now render the texture to the screen 
    pD3DDevice->Clear(	0L,																// Count
											  NULL,															// Rects to clear
											  D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,	// Flags
                        D3DCOLOR_XRGB(105,105,105),				// Color
											  1.0f,															// Z
											  0L );															// Stencil

			// Move the cursor position and render
    pD3DDevice->SetTransform( D3DTS_WORLD, &matWorld );
    pD3DDevice->SetTransform( D3DTS_VIEW, &matWorld );
    pD3DDevice->SetTransform( D3DTS_PROJECTION, &matWorld );

	  pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	  pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	  pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );

    pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP );

    pD3DDevice->SetVertexShader( D3DFVF_XYZ | D3DFVF_TEX1 );
    pD3DDevice->SetStreamSource(	0,												  // Stream number
																	g_pD3DVertexBuffer,	        // Stream data
																	sizeof(CUSTOMVERTEX) );		  // Vertex stride
	  pD3DDevice->SetTexture( 0, renderTargetTexture );
//	  pD3DDevice->SetTexture( 0, (LPDIRECT3DTEXTURE8)&g_pResourceSysMemData[resource_ROMListBackdrop_OFFSET] );
    pD3DDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

    pD3DDevice->Present( NULL, NULL, NULL, NULL );
	}
}

//-------------------------------------------------------------
// Helper_LoadDriverInfoFile
//-------------------------------------------------------------
static BOOL Helper_LoadDriverInfoFile( void )
{
	DWORD len;
  std::string		driverListFile = g_ROMListPath;
	driverListFile += "\\";
	driverListFile += DRIVERLISTFILENAME;

	PRINTMSG( T_INFO, "Load driver list: %s", driverListFile.c_str() );

	HANDLE hFile = CreateFile(	driverListFile.c_str(),
															GENERIC_READ,
															0,
															NULL,
															OPEN_EXISTING,
															FILE_ATTRIBUTE_NORMAL,
															NULL );
	if( !hFile )
	{
		PRINTMSG( T_ERROR, "Could not open file %s!", driverListFile.c_str() );
		return FALSE;
	}

    // Read in the signature
	DWORD BytesRead = 0;
  char signature[64] = {0};
  ReadFile( hFile, signature, 6 + strlen(VERSION_STRING), &BytesRead, NULL );
  if( BytesRead != 6 + strlen(VERSION_STRING) || 
      strcmp( signature, "MAMEoX" VERSION_STRING ) )
  {
    CloseHandle( hFile );
    return FALSE;
  }


  MAMEoXLaunchData_t *mameoxLaunchData = (MAMEoXLaunchData_t*)g_launchData.Data;
  ReadFile( hFile, 
            &mameoxLaunchData->m_totalMAMEGames, 
            sizeof(mameoxLaunchData->m_totalMAMEGames), 
            &BytesRead, 
            NULL );
  if( BytesRead != sizeof(mameoxLaunchData->m_totalMAMEGames) )
  {
    CloseHandle( hFile );
    return FALSE;
  }

    // Read in the driver info
  g_driverData = new MAMEDriverData_t[ mameoxLaunchData->m_totalMAMEGames ];
  for( DWORD i = 0; i < mameoxLaunchData->m_totalMAMEGames; ++i )
  {
    ReadFile( hFile, 
              &g_driverData[i].m_index, 
              sizeof( g_driverData[i].m_index ),
              &BytesRead, 
              NULL );
    if( BytesRead != sizeof(g_driverData[i].m_index) )
    {
      CloseHandle( hFile );
      return FALSE;
    }

      // Read the filename length
    ReadFile( hFile, 
              &len, 
              sizeof( len ), 
              &BytesRead, 
              NULL );
    if( BytesRead != sizeof( len ) )
    {
      CloseHandle( hFile );
      return FALSE;
    }

      // Read the filename data
    g_driverData[i].m_romFileName = new char[ len ];
    ReadFile( hFile, 
              g_driverData[i].m_romFileName, 
              len, 
              &BytesRead, 
              NULL );
    if( BytesRead != len )
    {
      CloseHandle( hFile );
      return FALSE;
    }

      // Read the description length
    ReadFile( hFile, 
              &len, 
              sizeof( len ), 
              &BytesRead, 
              NULL );
    if( BytesRead != sizeof( len ) )
    {
      CloseHandle( hFile );
      return FALSE;
    }

      // Read the description data
    g_driverData[i].m_description = new char[ len ];
    ReadFile( hFile, 
              g_driverData[i].m_description, 
              len, 
              &BytesRead, 
              NULL );
    if( BytesRead != len )
    {
      CloseHandle( hFile );
      return FALSE;
    }
    
    ReadFile( hFile, 
              &g_driverData[i].m_isClone, 
              sizeof(g_driverData[i].m_isClone), 
              &BytesRead, 
              NULL );
    if( BytesRead != sizeof( g_driverData[i].m_isClone ) )
    {
      CloseHandle( hFile );
      return FALSE;
    }

			// Read the clonename length
    ReadFile( hFile, 
              &len, 
              sizeof( len ), 
              &BytesRead, 
              NULL );
    if( BytesRead != sizeof( len ) )
    {
      CloseHandle( hFile );
      return FALSE;
    }

      // Read the clonename
    g_driverData[i].m_cloneFileName = new char[ len ];
    ReadFile( hFile, 
              g_driverData[i].m_cloneFileName, 
              len,
              &BytesRead, 
              NULL );
    if( BytesRead != len )
    {
      CloseHandle( hFile );
      return FALSE;
    }

			// Read the manufacturer length
    ReadFile( hFile, 
              &len, 
              sizeof( len ), 
              &BytesRead, 
              NULL );
    if( BytesRead != sizeof( len ) )
    {
      CloseHandle( hFile );
      return FALSE;
    }

      // Read the manufacturer
    g_driverData[i].m_manufacturer = new char[ len ];
    ReadFile( hFile, 
              g_driverData[i].m_manufacturer,
              len,
              &BytesRead,
              NULL );
    if( BytesRead != len )
    {
      CloseHandle( hFile );
      return FALSE;
    }

			// Read the year length
    ReadFile( hFile, 
              &len, 
              sizeof( len ), 
              &BytesRead, 
              NULL );
    if( BytesRead != sizeof( len ) )
    {
      CloseHandle( hFile );
      return FALSE;
    }

      // Read the year
    g_driverData[i].m_year = new char[ len ];
    ReadFile( hFile, 
              g_driverData[i].m_year,
              len,
              &BytesRead,
              NULL );
    if( BytesRead != len )
    {
      CloseHandle( hFile );
      return FALSE;
    }
  }

  CloseHandle( hFile );

  return TRUE;
}

//-------------------------------------------------------------
//	Die
//-------------------------------------------------------------
void Die( LPDIRECT3DDEVICE8 pD3DDevice, const char *fmt, ... )
{
	char buf[1024];

  va_list arg;
  va_start( arg, fmt );
  vsprintf( buf, fmt, arg );
  va_end( arg );

	PRINTMSG( T_ERROR, "Die: %s", buf );

		// Display the error to the user
	pD3DDevice->Clear(	0L,																// Count
											NULL,															// Rects to clear
											D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,	// Flags
											D3DCOLOR_XRGB(0,0,0),							// Color
											1.0f,															// Z
											0L );															// Stencil

	g_font.Begin();
	
	WCHAR wBuf[1024];
	mbstowcs( wBuf, buf, strlen(buf) + 1 );

	g_font.DrawText( 320, 60, D3DCOLOR_RGBA( 255, 255, 255, 255), wBuf, XBFONT_CENTER_X );
	g_font.DrawText( 320, 320, D3DCOLOR_RGBA( 255, 125, 125, 255), L"Press any button to reboot.", XBFONT_CENTER_X );

	g_font.End();
	pD3DDevice->Present( NULL, NULL, NULL, NULL );

	g_inputManager.WaitForNoButton( 0 );
	g_inputManager.WaitForAnyButton( 0 );
	g_inputManager.WaitForNoButton( 0 );

    // Reboot
  LD_LAUNCH_DASHBOARD LaunchData = { XLD_LAUNCH_DASHBOARD_MAIN_MENU };
  XLaunchNewImage( NULL, (LAUNCH_DATA*)&LaunchData );
}



//-------------------------------------------------------------
//  CreateBackdrop
//-------------------------------------------------------------
BOOL CreateBackdrop( FLOAT xUsage, FLOAT yUsage )
{
  if( g_pD3DVertexBuffer )
  {
    g_pD3DVertexBuffer->Release();
    g_pD3DVertexBuffer = NULL;
  }

    // Create the vertex buffer
  g_graphicsManager.GetD3DDevice()->CreateVertexBuffer( (sizeof(CUSTOMVERTEX) << 2),
																		                    D3DUSAGE_WRITEONLY,
																		                    D3DFVF_XYZ | D3DFVF_TEX1,
																		                    D3DPOOL_MANAGED,
																		                    &g_pD3DVertexBuffer );

	CUSTOMVERTEX *pVertices;
	g_pD3DVertexBuffer->Lock( 0,										// Offset to lock
														0,										// Size to lock
														(BYTE**)&pVertices,		// ppbData
														0 );									// Flags

      //-- Draw the backdrop -------------------------------------------------
		pVertices[0].pos.x = -xUsage;
		pVertices[0].pos.y = yUsage;
		pVertices[0].pos.z = 1.0f;

		pVertices[1].pos.x = xUsage;
		pVertices[1].pos.y = yUsage;
		pVertices[1].pos.z = 1.0f;
		
		pVertices[2].pos.x = xUsage;
		pVertices[2].pos.y = -yUsage;
		pVertices[2].pos.z = 1.0f;
		
		pVertices[3].pos.x = -xUsage;
		pVertices[3].pos.y = -yUsage;
		pVertices[3].pos.z = 1.0f;

    FLOAT tu_l = 0.0f, tu_r = 512.0f, tv_t = 0.0f, tv_b = 512.0f;

    switch( g_rendererOptions.m_screenRotation )
    {
    case SR_0:
      pVertices[0].tu = tu_l;
      pVertices[0].tv = tv_t;
      pVertices[1].tu = tu_r;
      pVertices[1].tv = tv_t;
      pVertices[2].tu = tu_r;
      pVertices[2].tv = tv_b;
      pVertices[3].tu = tu_l;
      pVertices[3].tv = tv_b;
      break;

    case SR_90:
      pVertices[0].tu = tu_l;
      pVertices[0].tv = tv_b;
      pVertices[1].tu = tu_l;
      pVertices[1].tv = tv_t;
      pVertices[2].tu = tu_r;
      pVertices[2].tv = tv_t;
      pVertices[3].tu = tu_r;
      pVertices[3].tv = tv_b;
      break;

    case SR_180:
      pVertices[0].tu = tu_r;
      pVertices[0].tv = tv_b;
      pVertices[1].tu = tu_l;
      pVertices[1].tv = tv_b;
      pVertices[2].tu = tu_l;
      pVertices[2].tv = tv_t;
      pVertices[3].tu = tu_r;
      pVertices[3].tv = tv_t;
      break;

    case SR_270:
      pVertices[0].tu = tu_r;
      pVertices[0].tv = tv_t;
      pVertices[1].tu = tu_r;
      pVertices[1].tv = tv_b;
      pVertices[2].tu = tu_l;
      pVertices[2].tv = tv_b;
      pVertices[3].tu = tu_l;
      pVertices[3].tv = tv_t;
      break;
    }


	g_pD3DVertexBuffer->Unlock();


  return TRUE;
}

//-------------------------------------------------------------
//  DestroyBackdrop
//-------------------------------------------------------------
void DestroyBackdrop( void )
{
  if( g_pD3DVertexBuffer )
  {
    g_pD3DVertexBuffer->Release();
    g_pD3DVertexBuffer = NULL;
  }
}

//-------------------------------------------------------------
//	SetScreenUsage
//-------------------------------------------------------------
void SetScreenUsage( FLOAT xPercentage, FLOAT yPercentage )
{
  g_screenXPercentage = xPercentage;
  g_screenYPercentage = yPercentage;
}

//-------------------------------------------------------------
//	GetScreenUsage
//-------------------------------------------------------------
void GetScreenUsage( FLOAT *xPercentage, FLOAT *yPercentage )
{
  if( xPercentage )
    *xPercentage = g_screenXPercentage;
  if( yPercentage )
    *yPercentage = g_screenYPercentage;
}

//-----------------------------------------------------------------------------
// LoadPackedResources
//-----------------------------------------------------------------------------
static HRESULT LoadPackedResources( void )
{
  FILE *file = fopen( "D:\\Media\\Resource.xpr", "rb" );
  if( !file )
    return E_FAIL;

    // Read in and verify the XPR magic header
  XPR_HEADER xprh;
  if( fread( &xprh, sizeof(XPR_HEADER), 1, file ) != 1 )
  {
    fclose( file );
    return E_FAIL;
  }

  if( xprh.dwMagic != XPR_MAGIC_VALUE )
  {
    PRINTMSG( T_INFO, "ERROR: Invalid Xbox Packed Resource (.xpr) file" );
    fclose( file );
    return E_INVALIDARG;
  }

    // Compute memory requirements
  DWORD dwSysMemDataSize = xprh.dwHeaderSize - sizeof(XPR_HEADER);
  DWORD dwVidMemDataSize = xprh.dwTotalSize - xprh.dwHeaderSize;

    // Allocate memory
  g_pResourceSysMemData = new BYTE[dwSysMemDataSize];
  g_pResourceVidMemData = (BYTE*)D3D_AllocContiguousMemory( dwVidMemDataSize, D3DTEXTURE_ALIGNMENT );

    // Read in the data from the file
  if( fread( g_pResourceSysMemData, dwSysMemDataSize, 1, file ) != 1 ||
      fread( g_pResourceVidMemData, dwVidMemDataSize, 1, file ) != 1 )
  {
    delete[] g_pResourceSysMemData;
    D3D_FreeContiguousMemory( g_pResourceVidMemData );
    fclose( file );
    return E_FAIL;
  }

  fclose( file );
  
    // Loop over resources, calling Register()
  BYTE *pData = g_pResourceSysMemData;

  for( DWORD i = 0; i < resource_NUM_RESOURCES; ++i )
  {
    LPDIRECT3DRESOURCE8 pResource = (LPDIRECT3DRESOURCE8)pData;
    pResource->Register( g_pResourceVidMemData );

    switch( pResource->GetType() )
    {
      case D3DRTYPE_TEXTURE:       
        pData += sizeof(D3DTexture);       
        break;

      case D3DRTYPE_VOLUMETEXTURE: 
        pData += sizeof(D3DVolumeTexture); 
        break;

      case D3DRTYPE_CUBETEXTURE:   
        pData += sizeof(D3DCubeTexture);   
        break;

      case D3DRTYPE_VERTEXBUFFER:  
        pData += sizeof(D3DVertexBuffer);  
        break;

      case D3DRTYPE_INDEXBUFFER:   
        pData += sizeof(D3DIndexBuffer);   
        break;

      case D3DRTYPE_PALETTE:       
        pData += sizeof(D3DPalette);       
        break;

      default:
        return E_FAIL;
    }
  }
  return S_OK;
}

//-------------------------------------------------------------
//	ShowSplashScreen
//-------------------------------------------------------------
static void ShowSplashScreen( LPDIRECT3DDEVICE8 pD3DDevice )
{
  LPDIRECT3DVERTEXBUFFER8 pD3DVertexBuffer = NULL;
  RequireController( 0 );
  CGamepad *gp = g_inputManager.GetGamepad( 0 );


    // Create the vertex buffer
  struct CUSTOMVERTEX
  {
	  D3DXVECTOR3   pos;      // The transformed position for the vertex
    FLOAT         tu, tv;   // The texture coordinates
  };

  pD3DDevice->CreateVertexBuffer( (sizeof(CUSTOMVERTEX) << 2),
							                    D3DUSAGE_WRITEONLY,
																	D3DFVF_XYZ | D3DFVF_TEX1,
																	D3DPOOL_MANAGED,
																	&pD3DVertexBuffer );

	CUSTOMVERTEX *pVertices;
	pD3DVertexBuffer->Lock( 0,										// Offset to lock
												  0,										// Size to lock
													(BYTE**)&pVertices,		// ppbData
													0 );									// Flags

      //-- Draw the backdrop -------------------------------------------------
		pVertices[0].pos.x = -0.80f;
		pVertices[0].pos.y = 0.80f;
		pVertices[0].pos.z = 1.0f;
    pVertices[0].tu = 0.0f;
    pVertices[0].tv = 0.0f;

		pVertices[1].pos.x = 0.80f;
		pVertices[1].pos.y = 0.80f;
		pVertices[1].pos.z = 1.0f;
    pVertices[1].tu = 1.0f;
    pVertices[1].tv = 0.0f;

		pVertices[2].pos.x = 0.80f;
		pVertices[2].pos.y = 0.0f;
		pVertices[2].pos.z = 1.0f;
    pVertices[2].tu = 1.0f;
    pVertices[2].tv = 1.0f;

		pVertices[3].pos.x = -0.80f;
		pVertices[3].pos.y = 0.0f;
		pVertices[3].pos.z = 1.0f;
    pVertices[3].tu = 0.0f;
    pVertices[3].tv = 1.0f;

	pD3DVertexBuffer->Unlock();

  LPDIRECT3DTEXTURE8 pTexture = (LPDIRECT3DTEXTURE8)&g_pResourceSysMemData[resource_MAMEoXLogo_OFFSET];


  while( !gp->IsAnyButtonPressed() )
  {
    RequireController( 0 );

		  // Clear the backbuffer
    pD3DDevice->Clear(	0L,																// Count
											  NULL,															// Rects to clear
											  D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,	// Flags
                        D3DCOLOR_XRGB(255,255,255),			  // Color
											  1.0f,															// Z
											  0L );															// Stencil

	  pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	  pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	  pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

    pD3DDevice->SetVertexShader( D3DFVF_XYZ | D3DFVF_TEX1 );
    pD3DDevice->SetStreamSource(	0,												// Stream number
																  pD3DVertexBuffer,					// Stream data
																  sizeof(CUSTOMVERTEX) );		// Vertex stride
	  pD3DDevice->SetTexture( 0, pTexture );

    pD3DDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

    g_font.Begin();
      g_font.DrawText( 320, 258, D3DCOLOR_XRGB( 0, 0, 80 ),     L"Version " LVERSION_STRING L" " LBUILDCONFIG_STRING, XBFONT_CENTER_X );
      g_font.DrawText( 320, 352, D3DCOLOR_XRGB( 10, 90, 100 ),  L"Portions based on:", XBFONT_CENTER_X );
      g_font.DrawText( 320, 376, D3DCOLOR_XRGB( 10, 90, 100 ),  L"\"MAMEX(b5): updated by superfro, original port by opcode\"", XBFONT_CENTER_X );
	    g_font.DrawText( 320, 400, D3DCOLOR_XRGB( 60, 105, 225 ), L"Press any button to continue.", XBFONT_CENTER_X );
    g_font.End();

    pD3DDevice->Present( NULL, NULL, NULL, NULL );
  }

	g_inputManager.WaitForNoButton( 0 );

  pD3DVertexBuffer->Release();
}


extern "C" {
//-------------------------------------------------------------
//	osd_init
//-------------------------------------------------------------
int osd_init( void )
{
	return 0;
}

//-------------------------------------------------------------
//	osd_exit
//-------------------------------------------------------------
void osd_exit( void )
{
}

}	// End Extern "C"

