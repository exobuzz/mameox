/**
	* \file		Gamepad.h
  *         Simple wrapper around the XINPUT_STATE and XINPUT_CAPABILITIES
  *         structures, providing utility functions such as IsButtonPressed
	*/

#pragma once

//= I N C L U D E S ===========================================================
#include <Xtl.h>
#include "osd_cpu.h"


//= D E F I N E S =============================================================
#define BUTTON_PRESS_THRESHOLD      50    //!< Press threshold for analog buttons

  //! \enum   gamepadButtonID_t 
  //! \brief  Gamepad Button ID's
typedef enum gamepadButtonID_t {
  GP_DPAD_UP          = 1,
  GP_DPAD_RIGHT       = (1<<1),
  GP_DPAD_DOWN        = (1<<2),
  GP_DPAD_LEFT        = (1<<3),
  GP_BACK             = (1<<4),
  GP_START            = (1<<5),
  GP_LEFT_ANALOG      = (1<<6),   //!<  Left analog button
  GP_RIGHT_ANALOG     = (1<<7),   //!<  Right analog button
  GP_A                = (1<<8),
  GP_B                = (1<<9),
  GP_X                = (1<<10),
  GP_Y                = (1<<11),
  GP_WHITE            = (1<<12),
  GP_BLACK            = (1<<13),
  GP_LEFT_TRIGGER     = (1<<14),
  GP_RIGHT_TRIGGER    = (1<<15),
  GP_LA_UP            = (1<<16),
  GP_LA_RIGHT         = (1<<17),
  GP_LA_DOWN          = (1<<18),
  GP_LA_LEFT          = (1<<19),
  GP_RA_UP            = (1<<20),
  GP_RA_RIGHT         = (1<<21),
  GP_RA_DOWN          = (1<<22),
  GP_RA_LEFT          = (1<<23)
} gamepadButtonID_t;

typedef enum gamepadAnalogID_t {
  GP_ANALOG_LEFT = 0x00,
  GP_ANALOG_RIGHT
} gamepadAnalogID_t;

typedef enum gamepadAxisID_t {
  GP_AXIS_X = 0x00,
  GP_AXIS_Y
} gamepadAxisID_t;


//= C L A S S E S =============================================================

  //! \class    CGamepad
  //! \brief    Simple wrapper around the XINPUT_STATE and XINPUT_CAPABILITIES
  //!           structures, providing utility functions such as IsButtonPressed
class CGamepad
{
public:
		//------------------------------------------------------
		//	Constructor
		//------------------------------------------------------
	CGamepad( void );

		//------------------------------------------------------
		//	Create
    //! \brief    Sets up the gamepad instance, must be
    //!           called before any other functions will become
    //!           valid.
    //!
    //! \param    gpIndex - 0 based index of this gamepad
    //! \param    maxMemUnits - Max number of mem units for this gp 
    //! \param    gpBitmap - Pointer to the gamepad bitmap for the system
    //! \param    muBitmap - Pointer to the mem unit bitmap for the system
		//------------------------------------------------------
	BOOL Create(  DWORD gpIndex, 
                DWORD maxMemUnits, 
                const DWORD *gpBitmap, 
                const DWORD *muBitmap );

		//------------------------------------------------------
		//	IsConnected
    //! \brief    Checks to see if this gamepad is connected
    //!
    //! \return   BOOL - TRUE if the gamepad is connected
		//------------------------------------------------------
  BOOL IsConnected( void );

		//------------------------------------------------------
		//	IsMUConnected
    //! \brief    Returns TRUE if a memory unit is inserted
    //!           in the requested slot
    //!
    //! \param    bottomMU - Selects the slot to check (top or bottom)
    //!
    //! \return   BOOL - TRUE if an MU is inserted
		//------------------------------------------------------
  BOOL IsMUConnected( BOOL bottomMU = FALSE );

		//------------------------------------------------------
		//	GetAnalogButtonState
    //! \brief    Returns the selected analog button value
    //!
    //! \param    buttonID - ID of the button to query
    //!
    //! \return   UINT8 - Selected axis position value
		//------------------------------------------------------
  UINT8 GetAnalogButtonState( gamepadButtonID_t buttonID );

		//------------------------------------------------------
		//	GetAnalogAxisState
    //! \brief    Returns the selected analog axis position
    //!
    //! \param    analogID - ID of the analog joystick to query
    //! \param    axisID - ID of the axis value to be returned
    //!
    //! \return   SHORT - Selected axis position value
		//------------------------------------------------------
  SHORT GetAnalogAxisState( gamepadAnalogID_t analogID, gamepadAxisID_t axisID );

		//------------------------------------------------------
		//	IsAnyButtonPressed
    //! \brief    Returns TRUE if any button is pressed on the gamepad
    //!
    //! \return   BOOL - TRUE if any button is pressed, else FALSE
		//------------------------------------------------------
  BOOL IsAnyButtonPressed( void );

		//------------------------------------------------------
		//	IsButtonPressed
    //! \brief    Returns TRUE if the given button is pressed 
    //!           on the gamepad
    //!
    //! \return   BOOL - TRUE if button is pressed, else FALSE
		//------------------------------------------------------
  BOOL IsButtonPressed( UINT32 buttonID );

		//------------------------------------------------------
		//	IsOneOfButtonsPressed
    //! \brief    Returns TRUE if any of the given buttons   
    //!           is pressed on the gamepad
    //!
    //! \return   BOOL - TRUE if button is pressed, else FALSE
		//------------------------------------------------------
  BOOL IsOneOfButtonsPressed( UINT32 buttonID );

		//------------------------------------------------------
		//	WaitForAnyButton
		//! \brief		Wait for any button to be pressed on the
		//!            gamepad
		//------------------------------------------------------
	void WaitForAnyButton( void );

		//------------------------------------------------------
		//	WaitForNoButton
		//! \brief		Wait for all buttons to be released on the
		//!            gamepad
		//------------------------------------------------------
	void WaitForNoButton( void );

		//------------------------------------------------------
		//	SetGamepadFeedbackState
    //! Send a force feedback effect to a gamepad
    //!
    //! \param  feedback - Struct describing the effect to send
		//------------------------------------------------------
	inline BOOL SetGamepadFeedbackState( const XINPUT_FEEDBACK &feedback ) {

			// Make sure an op isn't already in progress
		if( m_feedback.Header.dwStatus == ERROR_IO_PENDING )
			return FALSE;

			// Store the var to ensure persistency (XInputSetState is async)
		m_feedback = feedback;
		if( m_gamepadDeviceHandle )
		{
			if( XInputSetState( m_gamepadDeviceHandle, &m_feedback ) != ERROR_IO_PENDING )
				return FALSE;
		}

		return TRUE;
	}

		//------------------------------------------------------
		//	PollDevice
    //! \brief    Poll the associated physical device for
    //!           its current state
		//------------------------------------------------------
	inline void PollDevice( void ) {
    if( m_gamepadDeviceHandle )
      XInputGetState( m_gamepadDeviceHandle, &m_state );
	}

		//------------------------------------------------------
		//	AttachRemoveDevices
    //! \brief    Attach or remove physical devices at
    //!           the associated gameport
		//------------------------------------------------------
	inline void AttachRemoveDevices( void ) {
			// Attach/Remove gamepads
		AttachRemoveGamepadDevice();

			// Attach/Remove MemUnit sets
		AttachRemoveMemUnitDevicePair();
	}

		//------------------------------------------------------
		//	GetGamepadDeviceState
    //! Return the current state of a given gamepad
    //!
    //! \retval   const XINPUT_GAMEPAD * - The requested
    //!                                    gamepad state object
		//------------------------------------------------------
	const XINPUT_GAMEPAD *GetGamepadDeviceState( void ) const;

		//------------------------------------------------------
		//	GetGamepadDeviceCaps
    //! Return the capabilities of a given gamepad
    //!
    //! \retval   const XINPUT_CAPABILITIES * - The requested
    //!                                    gamepad caps object
		//------------------------------------------------------
	const XINPUT_CAPABILITIES *GetGamepadDeviceCaps( void ) const;



  protected:
		//------------------------------------------------------
		//	AttachRemoveGamepadDevice
		//------------------------------------------------------
	inline void AttachRemoveGamepadDevice( void ) {
		if(	(*m_gamepadDeviceBitmap & m_portMask ) && !m_gamepadDeviceHandle )
		{
				// Attach
			m_gamepadDeviceHandle = XInputOpen( XDEVICE_TYPE_GAMEPAD,
																							             m_portName,				
																							             XDEVICE_NO_SLOT,			// Gamepad, so no slot
																							             NULL );								// No special polling params
      XInputGetCapabilities( m_gamepadDeviceHandle, &m_caps );
		}
		else if( !(*m_gamepadDeviceBitmap & m_portMask ) && m_gamepadDeviceHandle )
		{
				// Detach
			XInputClose( m_gamepadDeviceHandle );
			m_gamepadDeviceHandle = NULL;
			m_state.dwPacketNumber = 0;
		}
	}

		//------------------------------------------------------
		//	AttachRemoveMemUnitDevicePair
		//------------------------------------------------------
	inline void AttachRemoveMemUnitDevicePair( void ) {

			// -- Top --------------------------------
		if( (*m_memunitDeviceBitmap & m_topMemPortMask) && !m_memunitDeviceHandles[0] )
		{
				// Attach
			m_memunitDeviceHandles[0] = XInputOpen( XDEVICE_TYPE_MEMORY_UNIT,
																						  m_portName,				
																							XDEVICE_TOP_SLOT,			// Gamepad, so no slot
																							NULL );								// No special polling params
		}
		else if( !(*m_memunitDeviceBitmap & m_topMemPortMask ) && m_memunitDeviceHandles[0] )
		{
				// Detach
			XInputClose( m_memunitDeviceHandles[0] );
			m_memunitDeviceHandles[0] = NULL;
		}

			// -- Bottom --------------------------------
		if( (*m_memunitDeviceBitmap & m_bottomMemPortMask) && !m_memunitDeviceHandles[1] )
		{
				// Attach
			m_memunitDeviceHandles[1] = XInputOpen( XDEVICE_TYPE_MEMORY_UNIT,
																							m_portName,				
																							XDEVICE_BOTTOM_SLOT,			// Gamepad, so no slot
																							NULL );								// No special polling params
		}
		else if( !(*m_memunitDeviceBitmap & m_bottomMemPortMask ) && m_memunitDeviceHandles[1] )
		{
				// Detach
			XInputClose( m_memunitDeviceHandles[1] );
			m_memunitDeviceHandles[1] = NULL;
		}
	}

	const DWORD			    *m_gamepadDeviceBitmap;			//!<	Bitmap storing which gamepad devices are currently attached
	const DWORD			    *m_memunitDeviceBitmap;			//!<	Bitmap storing which mem unit devices are currently attached

	DWORD               m_portMask, m_portName;
	DWORD               m_topMemPortMask, m_bottomMemPortMask;

  DWORD               m_gamepadIndex;             //!<  Index of this gamepad
	HANDLE					    m_gamepadDeviceHandle;	    //!<	Input handles for gamepads
	HANDLE					    m_memunitDeviceHandles[2];	//!<	Input handles for mem units


	XINPUT_STATE		    m_state;	      //!<	Gamepad device state struct
	XINPUT_FEEDBACK     m_feedback;			//!<	Feedback struct
  XINPUT_CAPABILITIES m_caps;         //!<  Gamepad device capabilities

};
