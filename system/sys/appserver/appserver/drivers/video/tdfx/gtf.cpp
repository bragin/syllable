
/*
 * Calculate timing parameters of any given video mode using the VESA GTF.
 *
 * taken from the Linux Kernel 2.6.0:
 * linux/drivers/video/fbmon.c
 *
 * Copyright (C) 2002 James Simmons <jsimmons@users.sf.net>
 *
 * Generalized Timing Formula is derived from:
 *
 *      GTF Spreadsheet by Andy Morrish (1/5/97)
 *      available at http://www.vesa.org
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <atheos/types.h>
#include <string.h>
#include "gtf.h"


/**
 * gtf_get_vblank - get vertical blank time
 * @hfreq: horizontal freq
 *
 * DESCRIPTION:
 * vblank = right_margin + vsync_len + left_margin
 *
 *    given: right_margin = 1 (V_FRONTPORCH)
 *           vsync_len    = 3
 *           flyback      = 550
 *
 *                          flyback * hfreq
 *           left_margin  = --------------- - vsync_len
 *                           1000000
 */
static uint32 gtf_get_vblank( uint32 hfreq )
{
	uint32 vblank;

	vblank = ( hfreq * FLYBACK ) / 1000;
	vblank = ( vblank + 500 ) / 1000;
	return ( vblank + V_FRONTPORCH );
}

/**
 * gtf_get_hblank_by_freq - get horizontal blank time given hfreq
 * @hfreq: horizontal freq
 * @xres: horizontal resolution in pixels
 *
 * DESCRIPTION:
 *
 *           xres * duty_cycle
 * hblank = ------------------
 *           100 - duty_cycle
 *
 * duty cycle = percent of htotal assigned to inactive display
 * duty cycle = C - (M/Hfreq)
 *
 * where: C = ((offset - scale factor) * blank_scale)
 *            -------------------------------------- + scale factor
 *                        256
 *        M = blank_scale * gradient
 *
 */
static uint32 gtf_get_hblank_by_hfreq( uint32 hfreq, uint32 xres )
{
	uint32 c_val, m_val, duty_cycle, hblank;

	c_val = ( ( ( H_OFFSET - H_SCALEFACTOR ) * H_BLANKSCALE ) / 256 + H_SCALEFACTOR ) * 1000;
	m_val = ( H_BLANKSCALE * H_GRADIENT ) / 256;
	m_val = ( m_val * 1000000 ) / hfreq;
	duty_cycle = c_val - m_val;
	hblank = ( xres * duty_cycle ) / ( 100000 - duty_cycle );
	return ( hblank );
}


/**
 * gtf_get_hfreq - estimate hsync
 * @vfreq: vertical refresh rate
 * @yres: vertical resolution
 *
 * DESCRIPTION:
 *
 *          (yres + front_port) * vfreq * 1000000
 * hfreq = -------------------------------------
 *          (1000000 - (vfreq * FLYBACK)
 *
 */

static uint32 gtf_get_hfreq( uint32 vfreq, uint32 yres )
{
	uint32 divisor, hfreq;

	divisor = ( 1000000 - ( vfreq * FLYBACK ) ) / 1000;
	hfreq = ( yres + V_FRONTPORCH ) * vfreq * 1000;
	return ( hfreq / divisor );
}

static void gtf_timings_vfreq( struct __fb_timings *timings )
{
	timings->hfreq = gtf_get_hfreq( timings->vfreq, timings->vactive );
	timings->vblank = gtf_get_vblank( timings->hfreq );
	timings->vtotal = timings->vactive + timings->vblank;
	timings->hblank = gtf_get_hblank_by_hfreq( timings->hfreq, timings->hactive );
	timings->htotal = timings->hactive + timings->hblank;
	timings->dclk = timings->htotal * timings->hfreq;
}


/*
 * gtf_get_mode - calculates video mode using VESA GTF
 *
 * DESCRIPTION:
 * Calculates video mode based on monitor specs using VESA GTF.
 * The GTF is best for VESA GTF compliant monitors but is
 * specifically formulated to work for older monitors as well.
 *
 * All calculations are based on the VESA GTF Spreadsheet
 * available at VESA's public ftp (http://www.vesa.org).
 *
 * NOTES:
 * The timings generated by the GTF will be different from VESA
 * DMT.  It might be a good idea to keep a table of standard
 * VESA modes as well.  The GTF may also not work for some displays,
 * such as, and especially, analog TV.
 */
void gtf_get_mode( int nWidth, int nHeight, int nRefresh, TimingParams * t )
{
	struct __fb_timings timings;

	memset( &timings, 0, sizeof( struct __fb_timings ) );
	timings.hactive = nWidth;
	timings.vactive = nHeight;
	timings.vfreq = nRefresh;

	gtf_timings_vfreq( &timings );

	t->pixclock = KHZ2PICOS( timings.dclk / 1000 );
	t->hsync_len = ( timings.htotal * 8 ) / 100;
	t->right_margin = ( timings.hblank / 2 ) - t->hsync_len;
	t->left_margin = timings.hblank - t->right_margin - t->hsync_len;
	t->vsync_len = 3;
	t->lower_margin = 1;
	t->upper_margin = timings.vblank - ( t->vsync_len + t->lower_margin );
}
