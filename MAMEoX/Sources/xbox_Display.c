/**
	* \file			xbox_Display.c
	* \brief		Implementation of the "display" section of
	*           osdepend.h
	*/

//= I N C L U D E S ====================================================
#include <Xtl.h>

#include "osd_cpu.h"
#include "osdepend.h"
#include "driver.h"
#include "usrintrf.h"

#include "MAMEoX.h"

#include "xbox_Direct3DRenderer.h"
#include "DebugLogger.h"



//= D E F I N E S ======================================================
#define FRAMESKIP_LEVELS			12 


//= G L O B A L = V A R S ==============================================

// frameskipping tables
static const BOOL g_skipTable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] = { { 0,0,0,0,0,0,0,0,0,0,0,0 },
	                                                                    { 0,0,0,0,0,0,0,0,0,0,0,1 },
	                                                                    { 0,0,0,0,0,1,0,0,0,0,0,1 },
	                                                                    { 0,0,0,1,0,0,0,1,0,0,0,1 },
	                                                                    { 0,0,1,0,0,1,0,0,1,0,0,1 },
	                                                                    { 0,1,0,0,1,0,1,0,0,1,0,1 },
	                                                                    { 0,1,0,1,0,1,0,1,0,1,0,1 },
	                                                                    { 0,1,0,1,1,0,1,0,1,1,0,1 },
	                                                                    { 0,1,1,0,1,1,0,1,1,0,1,1 },
	                                                                    { 0,1,1,1,0,1,1,1,0,1,1,1 },
	                                                                    { 0,1,1,1,1,1,0,1,1,1,1,1 },
	                                                                    { 0,1,1,1,1,1,1,1,1,1,1,1 } };

  //!< Whether or not to use auto frame skipping
BOOL              g_autoFrameSkip = TRUE;

static INT32			g_frameskip = 0;
static INT32			g_frameskipCounter = 0;
static INT32			g_frameskipAdjust = 0;

static BOOL				g_throttleFrameRate = TRUE;

  // The desired framerate for the title
static FLOAT      g_desiredFPS = 0.0f;

	// palette lookups
UINT32	          g_pal32Lookup[65536] = {0};


//= P R O T O T Y P E S ================================================
static void InitializePalettes( void );
static void Helper_UpdatePalette( struct mame_display *display );



//= F U N C T I O N S ==================================================

//---------------------------------------------------------------------
//	osd_create_display
//---------------------------------------------------------------------
int osd_create_display( const struct osd_create_params *params, UINT32 *rgb_components )
{
/*
  Create a display screen, or window, of the given dimensions (or larger). It is
  acceptable to create a smaller display if necessary, in that case the user must
  have a way to move the visibility window around.

  The params contains all the information the
  Attributes are the ones defined in driver.h, they can be used to perform
  optimizations, e.g. dirty rectangle handling if the game supports it, or faster
  blitting routines with fixed palette if the game doesn't change the palette at
  run time. The VIDEO_PIXEL_ASPECT_RATIO flags should be honored to produce a
  display of correct proportions.
  Orientation is the screen orientation (as defined in driver.h) which will be done
  by the core. This can be used to select thinner screen modes for vertical games
  (ORIENTATION_SWAP_XY set), or even to ask the user to rotate the monitor if it's
  a pivot model. Note that the OS dependant code must NOT perform any rotation,
  this is done entirely in the core.
  Depth can be 8 or 16 for palettized modes, meaning that the core will store in the
  bitmaps logical pens which will have to be remapped through a palette at blit time,
  and 15 or 32 for direct mapped modes, meaning that the bitmaps will contain RGB
  triplets (555 or 888). For direct mapped modes, the VIDEO_RGB_DIRECT flag is set
  in the attributes field.

  Returns 0 on success.
*/

	PRINTMSG( T_TRACE, "osd_create_display" );

  g_desiredFPS = params->fps;

	InitializePalettes();

		// Pass the new coords on to the UI
	set_ui_visarea( 0,0,0,0 );

	// fill in the resulting RGB components
	if( rgb_components )
	{
		if( params->depth == 32 /*|| !(params->video_attributes & VIDEO_RGB_DIRECT)*/ )
		{
			rgb_components[0] = WIN_COLOR32(0xff, 0x00, 0x00);
			rgb_components[1] = WIN_COLOR32(0x00, 0xff, 0x00);
			rgb_components[2] = WIN_COLOR32(0x00, 0x00, 0xff);
		}
		else
		{
			rgb_components[0] = 0x7c00;
			rgb_components[1] = 0x03e0;
			rgb_components[2] = 0x001f;
		}
	}


		// Initialize the renderer
	if( !D3DRendererCreateSession( params ) )
	{
		PRINTMSG( T_ERROR, "D3DRendererCreateSession failed!" );
		return 1;
	}


  return 0;
}

//---------------------------------------------------------------------
//	osd_close_display
//---------------------------------------------------------------------
void osd_close_display(void)
{
	PRINTMSG( T_TRACE, "osd_close_display" );
	D3DRendererDestroySession();
}

//---------------------------------------------------------------------
//	osd_skip_this_frame
//---------------------------------------------------------------------
int osd_skip_this_frame(void)
{
/*
  osd_skip_this_frame() must return 0 if the current frame will be displayed.
  This can be used by drivers to skip cpu intensive processing for skipped
  frames, so the function must return a consistent result throughout the
  current frame. The function MUST NOT check timers and dynamically determine
  whether to display the frame: such calculations must be done in
  osd_update_video_and_audio(), and they must affect the FOLLOWING frames, not
  the current one. At the end of osd_update_video_and_audio(), the code must
  already know exactly whether the next frame will be skipped or not.
*/
	return g_skipTable[g_frameskip][g_frameskipCounter];
}

//---------------------------------------------------------------------
//	osd_update_video_and_audio
//---------------------------------------------------------------------
void osd_update_video_and_audio(struct mame_display *display)
{
/*
  Update video and audio. game_bitmap contains the game display, while
  debug_bitmap an image of the debugger window (if the debugger is active; NULL
  otherwise). They can be shown one at a time, or in two separate windows,
  depending on the OS limitations. If only one is shown, the user must be able
  to toggle between the two by pressing IPT_UI_TOGGLE_DEBUG; moreover,
  osd_debugger_focus() will be used by the core to force the display of a
  specific bitmap, e.g. the debugger one when the debugger becomes active.

  leds_status is a bitmask of lit LEDs, usually player start lamps. They can be
  simulated using the keyboard LEDs, or in other ways e.g. by placing graphics
  on the window title bar.
*/

  static cycles_t lastFrameEndTime = 0;
  const struct performance_info *performance = mame_get_performance_info();

//	PRINTMSG( T_TRACE, "osd_update_video_and_audio" );

	if( display->changed_flags & GAME_VISIBLE_AREA_CHANGED )
	{
		D3DRendererSetOutputRect( display->game_visible_area.min_x,
															display->game_visible_area.min_y,
															display->game_visible_area.max_x,
															display->game_visible_area.max_y );

			// Pass the new coords on to the UI
		set_ui_visarea( display->game_visible_area.min_x,
										display->game_visible_area.min_y,
										display->game_visible_area.max_x,
										display->game_visible_area.max_y );
	}

	if( display->changed_flags & GAME_PALETTE_CHANGED )
		Helper_UpdatePalette( display );

		// New bitmap data
	if( display->changed_flags & GAME_BITMAP_CHANGED )
	{
		if( display->changed_flags & VECTOR_PIXELS_CHANGED )
			D3DRendererRender(	display->game_bitmap,
													&display->game_bitmap_update,
													display->vector_dirty_pixels );
		else
			D3DRendererRender(	display->game_bitmap,
													&display->game_bitmap_update,
													NULL );


      // Update autoframeskip
    if( g_autoFrameSkip && cpu_getcurrentframe() > (FRAMESKIP_LEVELS << 1) )
    {

		    // if we're too fast, attempt to decrease the frameskip
		  if( performance->game_speed_percent >= 99.5f )
		  {
			  ++g_frameskipAdjust;

			    // but only after 3 consecutive frames where we are too fast
			  if( g_frameskipAdjust >= 3 )
			  {
				  g_frameskipAdjust = 0;
				  if( g_frameskip )
          {
            --g_frameskip;
            PRINTMSG( T_INFO, "Decreasing frameskip level to %lu\n", g_frameskip );
          }
			  }
		  }
		  else
		  {
			    // if below 80% speed, be more aggressive
			  if( performance->game_speed_percent < 80.0f )
				  g_frameskipAdjust -= (INT32)((90.0f - performance->game_speed_percent) / 5.0f);			  
			  else if( g_frameskip < 8 ) 
        {
            // if we're close, only force it up to frameskip 8
				  --g_frameskipAdjust;
        }

			    // perform the adjustment
			  while( g_frameskipAdjust <= -2 )
			  {
				  g_frameskipAdjust += 2;
				  if( g_frameskip < FRAMESKIP_LEVELS - 1 )
					  ++g_frameskip;
          else
          {
            g_frameskipAdjust = 0;
            break;
          }
			  }
        PRINTMSG( T_INFO, "Increasing frameskip level to %lu\n", g_frameskip );
      }
    }


      // Wait out the remaining time for this frame
    if( lastFrameEndTime && !g_frameskip && performance->game_speed_percent >= 99.5f && g_desiredFPS != 0.0f )
    {
      cycles_t targetFrameCycles = (cycles_t)(((double)osd_cycles_per_second()) / g_desiredFPS);
      cycles_t actualFrameCycles = osd_cycles() - lastFrameEndTime;

        // Note that this loop could easily be "optimized" to be
        //  a while( statement ) ; - style loop, but I'm worried that the
        //  optimizing compiler would thing that it's not doing anything
        //  useful (after all, it's a busy wait) and throw it out altogether
      while( actualFrameCycles < targetFrameCycles )
      {
          // Catch wraparound (which won't happen for a long time :))
        if( osd_cycles() < lastFrameEndTime )
          break;
        actualFrameCycles = osd_cycles() - lastFrameEndTime;
      }
    }
      // Tag the end of this frame
    lastFrameEndTime = osd_cycles();
  }

	g_frameskipCounter = (g_frameskipCounter + 1) % FRAMESKIP_LEVELS;


		// Poll input
	PollGamepads();
	
}

//---------------------------------------------------------------------
//	osd_save_snapshot
//---------------------------------------------------------------------
void osd_save_snapshot( struct mame_bitmap *bitmap, const struct rectangle *bounds )
{
/*
  Save a screen shot of the game display. It is suggested to use the core
  function save_screen_snapshot() or save_screen_snapshot_as(), so the format
  of the screen shots will be consistent across ports. This hook is provided
  only to allow the display of a file requester to let the user choose the
  file name. This isn't scrictly necessary, so you can just call
  save_screen_snapshot() to let the core automatically pick a default name.
*/
}


//---------------------------------------------------------------------
//	osd_get_fps_text
//---------------------------------------------------------------------
const char *osd_get_fps_text( const struct performance_info *performance )
{
/*
  Returns a pointer to the text to display when the FPS display is toggled.
  This normally includes information about the frameskip, FPS, and percentage
  of full game speed.
*/
	static char buffer[1024];
	char *dest = buffer;

	// display the FPS, frameskip, percent, fps and target fps
	dest += sprintf(dest, 
                  "Auto%2d%4d%%%4d/%d fps",
			            g_frameskip,
			            (int)(performance->game_speed_percent + 0.5),
			            (int)(performance->frames_per_second + 0.5),
			            (int)(Machine->drv->frames_per_second + 0.5));

	/* for vector games, add the number of vector updates */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		dest += sprintf(dest, "\n %d vector updates", performance->vector_updates_last_second);
	}
	else if (performance->partial_updates_this_frame > 1)
	{
		dest += sprintf(dest, "\n %d partial updates", performance->partial_updates_this_frame);
	}

	/* return a pointer to the static buffer */
	return buffer;
}





//---------------------------------------------------------------------
//	InitializePalettes
//---------------------------------------------------------------------
static void InitializePalettes( void )
{
    // Initialize the palette to a debug mapping
  UINT32 i = 0;
  for( ; i < 65535; ++i )
    g_pal32Lookup[i] = i;
}



//------------------------------------------------------------
//	Helper_UpdatePalette
//------------------------------------------------------------
static void Helper_UpdatePalette( struct mame_display *display )
{
	UINT32 i, j;

	PRINTMSG( T_TRACE, "Helper_UpdatePalette" );

		// The game_palette_dirty entry is a bitflag specifying which
		// palette entries need to be updated

	for( i = 0, j = 0; i < display->game_palette_entries; i += 32, ++j )
	{
		UINT32 palDirty = display->game_palette_dirty[j];
		if( palDirty )
		{
			UINT32 idx = 0;
			for( ; idx < 32 && i + idx < display->game_palette_entries; ++idx )
			{
				if( palDirty & (1<<idx) )
					g_pal32Lookup[i+idx] = display->game_palette[i+idx];
			}

			display->game_palette_dirty[ j ] = 0;
		}
	}
}
