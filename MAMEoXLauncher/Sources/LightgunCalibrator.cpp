/**
	* \file			LightgunCalibrator.cpp
	* \brief		Helper class to handle lightgun calibration
	*
	*/

//= I N C L U D E S ====================================================
#include "LightgunCalibrator.h"
#include "DebugLogger.h"
#include "XBFont.h"

#include <string>

extern "C" {
#include "osdepend.h"
#include "driver.h"
}


//= D E F I N E S ======================================================
  // The number of calibration steps per device
#define NUM_CALIBRATIONSTEPS    3

  // Note: These values are halved for the cursor
#define TARGET_WIDTH    64
#define TARGET_HEIGHT   64

#define TARGET_UL_X   20
#define TARGET_UL_Y   15

#define TARGET_C_X    320 - (TARGET_WIDTH>>1)
#define TARGET_C_Y    240 - (TARGET_HEIGHT>>1)

#define TARGET_LR_X   555
#define TARGET_LR_Y   400


//= G L O B A L = V A R S ==============================================

//= P R O T O T Y P E S ================================================

//= F U N C T I O N S ==================================================

//---------------------------------------------------------------------
//  MoveCursor
//---------------------------------------------------------------------
void CLightgunCalibrator::MoveCursor( CInputManager &inputManager, BOOL unused )
{
  const XINPUT_CAPABILITIES *gpCaps;
    // Make sure we've got a lightgun to calibrate
  if( !(gpCaps = GetGamepadCaps( m_currentInputDeviceIndex )) || gpCaps->SubType != XINPUT_DEVSUBTYPE_GC_LIGHTGUN )
  {
    m_currentInputDeviceIndex = 0;
    if( !FindNextGun() )
    {
      m_currentInputDeviceIndex = 0;
      m_calibrationCompleted = TRUE;
      return;
    }
  }

  CGamepad *gp = inputManager.GetGamepad( m_currentInputDeviceIndex );
  if( !gp )
  {
    PRINTMSG( T_ERROR, "Could not retrieve CGamepad object for index %d", m_currentInputDeviceIndex );
    return;
  }

  lightgunCalibration_t &calibData = g_calibrationData[m_currentInputDeviceIndex];

    // Update the cursor position
  m_currentGunX = gp->GetAnalogAxisState( GP_ANALOG_LEFT, GP_AXIS_X );
  m_currentGunY = gp->GetAnalogAxisState( GP_ANALOG_LEFT, GP_AXIS_Y );

  if( gp->IsButtonPressed( GP_B ) )
  {
      // Throw away calibration data for this gun
    calibData.m_xData[0] = -32767;
    calibData.m_xData[1] = 0;
    calibData.m_xData[2] = 32767;

    calibData.m_yData[0] = 32767;
    calibData.m_yData[1] = 0;
    calibData.m_yData[2] = -32767;

      // Reset calibration step to 0
    m_calibrationStep = 0;
  }
  else if( gp->IsButtonPressed( GP_A ) )
  {
      // Store the value
    if( m_calibrationStep < 3 )
    {
      calibData.m_xData[m_calibrationStep] = m_currentGunX;
      calibData.m_yData[m_calibrationStep] = m_currentGunY;
    }    

      // Move on to the next step
    if( ++m_calibrationStep == NUM_CALIBRATIONSTEPS )
    {
        // This gun is finished, try to find another one,
        // quitting if we can't
      m_calibrationStep = 0;
      ++m_currentInputDeviceIndex;
      if( !FindNextGun() )
      {
        m_currentInputDeviceIndex = 0;
        m_calibrationCompleted = TRUE;
        gp->WaitForNoButton();
        return;        
      }
    }
    else
      gp->WaitForNoButton();  // Wait for the trigger to be released
  }

  GetCalibratedCursorPosition( inputManager );
}

//---------------------------------------------------------------------
//  Draw
//---------------------------------------------------------------------
void CLightgunCalibrator::Draw( BOOL clearScreen, BOOL flipOnCompletion )
{
  if( clearScreen )  
	  m_displayDevice->Clear(	0L,																// Count
		  											NULL,															// Rects to clear
			  										D3DCLEAR_TARGET,	                // Flags
				  									D3DCOLOR_XRGB(243,243,243),				// Color
					  								1.0f,															// Z
						  							0L );															// Stencil

    // Render the backdrop texture
  RenderBackdrop();


  #define TEXTCOLOR                     D3DCOLOR_XRGB( 10,10,10 );
  #define CURSOR_COLOR                  D3DCOLOR_RGBA( 255, 100, 100, 255 )
  static WCHAR *calibrationStepText[NUM_CALIBRATIONSTEPS] = { L"Shoot the upper left corner", 
                                                              L"Shoot the center", 
//                                                              L"Shoot the lower right corner",
                                                              L"Shoot anywhere to accept" };
  WCHAR wBuf[256];

  m_fontSet.DefaultFont().Begin();

    swprintf( wBuf, L"Gun in port %d", m_currentInputDeviceIndex );
    m_fontSet.DefaultFont().DrawText( 320, 160, TEXTCOLOR, wBuf, XBFONT_CENTER_X );

    m_fontSet.DefaultFont().DrawText( 320, 180, TEXTCOLOR, calibrationStepText[m_calibrationStep], XBFONT_CENTER_X );

    m_fontSet.DefaultFont().DrawText( 320, 240, TEXTCOLOR, L"Press B to discard changes.", XBFONT_CENTER_X );

      swprintf( wBuf, L"X: %d maps to %d", m_currentGunX, m_currentGunCalibratedX );
	    m_fontSet.DefaultFont().DrawText( 320, 80, TEXTCOLOR, wBuf, XBFONT_CENTER_X );

      swprintf( wBuf, L"Y: %d maps to %d", m_currentGunY, m_currentGunCalibratedY );
	    m_fontSet.DefaultFont().DrawText( 320, 100, TEXTCOLOR, wBuf, XBFONT_CENTER_X );

  m_fontSet.DefaultFont().End();


    // Render the cursor
  m_displayDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  m_displayDevice->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_SRCALPHA );
  m_displayDevice->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_INVSRCALPHA );
  m_displayDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
  m_displayDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
  m_displayDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
  m_displayDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

  m_displayDevice->SetTexture( 0, m_cursorTexture );
  m_displayDevice->SetVertexShader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX0 );

    // Display a target/cursor if the gun is pointed at the screen
  if( m_currentGunX || m_currentGunY )
  {
    m_displayDevice->Begin( D3DPT_QUADLIST );


      switch( m_calibrationStep )
      {
      case 0:
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 0.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_UL_X, TARGET_UL_Y, 1.0f, 1.0f );
        
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 1.0f, 0.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_UL_X + TARGET_WIDTH, TARGET_UL_Y, 1.0f, 1.0f );
        
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 1.0f, 1.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_UL_X + TARGET_WIDTH, TARGET_UL_Y + TARGET_HEIGHT, 1.0f, 1.0f );

        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 1.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_UL_X, TARGET_UL_Y + TARGET_HEIGHT, 1.0f, 1.0f );
        break;

      case 1:
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 0.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_C_X, TARGET_C_Y, 1.0f, 1.0f );
        
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 1.0f, 0.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_C_X + TARGET_WIDTH, TARGET_C_Y, 1.0f, 1.0f );
        
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 1.0f, 1.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_C_X + TARGET_WIDTH, TARGET_C_Y + TARGET_HEIGHT, 1.0f, 1.0f );

        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 1.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_C_X, TARGET_C_Y + TARGET_HEIGHT, 1.0f, 1.0f );
        break;
/*
      case 2:
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 0.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_LR_X, TARGET_LR_Y, 1.0f, 1.0f );
        
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 1.0f, 0.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_LR_X + TARGET_WIDTH, TARGET_LR_Y, 1.0f, 1.0f );
        
        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 1.0f, 1.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_LR_X + TARGET_WIDTH, TARGET_LR_Y + TARGET_HEIGHT, 1.0f, 1.0f );

        m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
        m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 1.0f );
        m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, TARGET_LR_X, TARGET_LR_Y + TARGET_HEIGHT, 1.0f, 1.0f );
        break;
*/

      case (NUM_CALIBRATIONSTEPS - 1):
        {
          FLOAT x = m_currentGunCalibratedX;
          FLOAT y = -m_currentGunCalibratedY;  // Y values are negated for MAME

            // Map the cursor to screen coords
          x = ((x+128.0f) * 640.0f / 256.0f);
          y = ((y+128.0f) * 480.0f / 256.0f);
          
          x -= (TARGET_WIDTH >> 2);
          y -= (TARGET_HEIGHT >> 2);

          m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
          m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 0.0f );
          m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, x, y, 1.0f, 1.0f );
          
          m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
          m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 1.0f, 0.0f );
          m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, x + (TARGET_WIDTH>>1), y, 1.0f, 1.0f );
          
          m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
          m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 1.0f, 1.0f );
          m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, x + (TARGET_WIDTH>>1), y + (TARGET_HEIGHT>>1), 1.0f, 1.0f );

          m_displayDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, CURSOR_COLOR );
          m_displayDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 1.0f );
          m_displayDevice->SetVertexData4f( D3DVSDE_VERTEX, x, y + (TARGET_HEIGHT>>1), 1.0f, 1.0f );
        }
        break;
      }
    m_displayDevice->End();
  }

  if( flipOnCompletion )
	  m_displayDevice->Present( NULL, NULL, NULL, NULL );	
}


//---------------------------------------------------------------------
//  UpdateLightgunCursorPosition
//---------------------------------------------------------------------
void CLightgunCalibrator::UpdateLightgunCursorPosition( CInputManager &inputManager )
{
	const XINPUT_GAMEPAD *gpCaps;
  if( (gpCaps = inputManager.GetGamepadDeviceState( m_currentInputDeviceIndex )) )
  {
    lightgunCalibration_t &calibData = g_calibrationData[m_currentInputDeviceIndex];

    m_currentGunX = gpCaps->sThumbLX - calibData.m_xData[1];
    m_currentGunY = -1 * (gpCaps->sThumbLY - calibData.m_yData[1]);

      // Map from -128 to 128
    if( gpCaps->sThumbLX < 0 )
      m_currentGunX = (int)((FLOAT)m_currentGunX * 128.0f / ((FLOAT)calibData.m_xData[0]+1.0f));
    else
      m_currentGunX = (int)((FLOAT)m_currentGunX * 128.0f / ((FLOAT)calibData.m_xData[2]+1.0f));

    if( gpCaps->sThumbLY > 0 )
      m_currentGunY = (int)((FLOAT)m_currentGunY * 128.0f / ((FLOAT)calibData.m_yData[0]+1.0f));
    else
      m_currentGunY = (int)((FLOAT)m_currentGunY * 128.0f / ((FLOAT)calibData.m_yData[2]+1.0f));

      // Lock to the expected range
    if( m_currentGunX > 128 )
      m_currentGunX = 128;
    else if( m_currentGunX < -128 )
      m_currentGunX = -128;

    if( m_currentGunY > 128 )
      m_currentGunY = 128;
    else if( m_currentGunY < -128 )
      m_currentGunY = -128;
  }
  else  
	  m_currentGunX = m_currentGunY = 0;
}

//---------------------------------------------------------------------
//  FindNextGun
//---------------------------------------------------------------------
BOOL CLightgunCalibrator::FindNextGun( void )
{
    // Find the first attached lightgun
  for( ; m_currentInputDeviceIndex < 4; ++m_currentInputDeviceIndex )
  {
    const XINPUT_CAPABILITIES *gpCaps;
    if( (gpCaps = GetGamepadCaps( m_currentInputDeviceIndex )) && gpCaps->SubType == XINPUT_DEVSUBTYPE_GC_LIGHTGUN )
      return TRUE;
  }

  return FALSE;
}

//---------------------------------------------------------------------
//  GetCalibratedCursorPosition
//---------------------------------------------------------------------
void CLightgunCalibrator::GetCalibratedCursorPosition( CInputManager &inputManager )
{
  lightgunCalibration_t &calibData = g_calibrationData[m_currentInputDeviceIndex];

    // Don't bother if we're not pointing at the screen
  if( !m_currentGunX && !m_currentGunY )
  {
    m_currentGunCalibratedX = m_currentGunCalibratedY = 0;
    return;
  }

  m_currentGunCalibratedX = m_currentGunX;// + calibData.m_xData[1];
  m_currentGunCalibratedY = m_currentGunY;// + calibData.m_yData[1]);

    // Map from -128 to 128
  FLOAT xMap = calibData.m_xData[0] - calibData.m_xData[1];
  FLOAT yMap = calibData.m_yData[0] - calibData.m_yData[1];

  if( xMap )
    m_currentGunCalibratedX = (int)((FLOAT)m_currentGunCalibratedX * 128.0f / -xMap );
  else
    m_currentGunCalibratedX = 0;

  if( yMap )
    m_currentGunCalibratedY = (int)((FLOAT)m_currentGunCalibratedY * 128.0f / yMap );
  else
    m_currentGunCalibratedY = 0;

    // Lock to the expected range
  if( m_currentGunCalibratedX > 128 )
    m_currentGunCalibratedX = 128;
  else if( m_currentGunCalibratedX < -128 )
    m_currentGunCalibratedX = -128;

  if( m_currentGunCalibratedY > 128 )
    m_currentGunCalibratedY = 128;
  else if( m_currentGunCalibratedY < -128 )
    m_currentGunCalibratedY = -128;
}