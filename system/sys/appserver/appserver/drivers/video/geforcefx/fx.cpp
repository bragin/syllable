
/*
 *  nVidia GeForce FX driver for Syllable
 *  Copyright (C) 2001  Joseph Artsimovich (joseph_a@mail.ru)
 *  Copyright 1993-2003 NVIDIA, Corporation.  All rights reserved.
 *
 *  Based on the rivafb linux kernel driver and the xfree86 nv driver.
 *  Video overlay code is from the rivatv project.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/vesa_gfx.h>
#include <atheos/udelay.h>
#include <atheos/time.h>
#include <appserver/pci_graphics.h>

#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <gui/bitmap.h>
#include "fx.h"
#include "fx_dma.h"

static const struct chip_info asChipInfos[] = { 
	{0x10DE0300, NV_ARCH_30, "0x0300"}, 
	{0x10DE0301, NV_ARCH_30, "GeForce FX 5800 Ultra"}, 
	{0x10DE0302, NV_ARCH_30, "GeForce FX 5800"}, 
	{0x10DE0308, NV_ARCH_30, "Quadro FX 2000"}, 
	{0x10DE0309, NV_ARCH_30, "Quadro FX 1000"}, 
	{0x10DE0311, NV_ARCH_30, "GeForce FX 5600 Ultra"}, 
	{0x10DE0312, NV_ARCH_30, "GeForce FX 5600"}, 
	{0x10DE0314, NV_ARCH_30, "GeForce FX 5600SE"}, 
	{0x10DE0316, NV_ARCH_30, "0x0316"}, 
	{0x10DE0317, NV_ARCH_30, "0x0317"}, 
	{0x10DE0318, NV_ARCH_30, "0x0318"}, 
	{0x10DE0319, NV_ARCH_30, "0x0319"}, 
	{0x10DE031A, NV_ARCH_30, "GeForce FX Go5600"}, 
	{0x10DE031B, NV_ARCH_30, "GeForce FX Go5650"}, 
	{0x10DE031C, NV_ARCH_30, "Quadro FX Go700"}, 
	{0x10DE031D, NV_ARCH_30, "0x031D"}, 
	{0x10DE031E, NV_ARCH_30, "0x031E"}, 
	{0x10DE031F, NV_ARCH_30, "0x031F"}, 
	{0x10DE0321, NV_ARCH_30, "GeForce FX 5200 Ultra"}, 
	{0x10DE0322, NV_ARCH_30, "GeForce FX 5200"}, 
	{0x10DE0323, NV_ARCH_30, "GeForce FX 5200SE"}, 
	{0x10DE0324, NV_ARCH_30, "GeForce FX Go5200"}, 
	{0x10DE0325, NV_ARCH_30, "GeForce FX Go5250"}, 
	{0x10DE0328, NV_ARCH_30, "GeForce FX Go5200 32M/64M"},
	{0x10DE032A, NV_ARCH_30, "0x032A"}, 
	{0x10DE032B, NV_ARCH_30, "Quadro FX 500"}, 
	{0x10DE032C, NV_ARCH_30, "Quadro FX Go5300"}, 
	{0x10DE032D, NV_ARCH_30, "Quadro FX Go5100"}, 
	{0x10DE032F, NV_ARCH_30, "0x032F"}, 
	{0x10DE0330, NV_ARCH_30, "GeForce FX 5900 Ultra"}, 
	{0x10DE0331, NV_ARCH_30, "GeForce FX 5900"}, 
	{0x10DE0333, NV_ARCH_30, "GeForce FX 5950 Ultra"}, 
	{0x10DE0333, NV_ARCH_30, "Quadro FX 3000"}
};


enum {
	FX_GET_DMA_ADDRESS = PCI_GFX_LAST_IOCTL
};

inline uint32 pci_size( uint32 base, uint32 mask )
{
	uint32 size = base & mask;

	return size & ~( size - 1 );
}

static uint32 get_pci_memory_size( int nFd, const PCI_Info_s * pcPCIInfo, int nResource )
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource * 4;
	uint32 nBase = pci_gfx_read_config( nFd, nBus, nDev, nFnc, nOffset, 4 );

	pci_gfx_write_config( nFd, nBus, nDev, nFnc, nOffset, 4, ~0 );
	uint32 nSize = pci_gfx_read_config( nFd, nBus, nDev, nFnc, nOffset, 4 );

	pci_gfx_write_config( nFd, nBus, nDev, nFnc, nOffset, 4, nBase );
	if ( !nSize || nSize == 0xffffffff )
		return 0;
	if ( nBase == 0xffffffff )
		nBase = 0;
	if ( !( nSize & PCI_ADDRESS_SPACE ) )
	{
		return pci_size( nSize, PCI_ADDRESS_MEMORY_32_MASK );
	}
	else
	{
		return pci_size( nSize, PCI_ADDRESS_IO_MASK & 0xffff );
	}
}


FX::FX( int nFd ):m_cGELock( "fx_ge_lock" ), m_hRegisterArea( -1 ), m_hFrameBufferArea( -1 ), m_cCursorPos( 0, 0 ), m_cCursorHotSpot( 0, 0 )
{
	m_bIsInitiated = false;
	m_bPaletteEnabled = false;
	m_bUsingHWCursor = false;
	m_bCursorIsOn = false;
	int j;

	bool bFound = false;
	int nNrDevs = sizeof( asChipInfos ) / sizeof( chip_info );

	/* Get Info */
	if ( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}


	uint32 ChipsetID = ( m_cPCIInfo.nVendorID << 16 ) | m_cPCIInfo.nDeviceID;

	for ( j = 0; j < nNrDevs; j++ )
	{
		if ( ChipsetID == asChipInfos[j].nDeviceId )
		{
			bFound = true;
			break;
		}
	}


	if ( !bFound )
	{
		dbprintf( "GeForceFX :: No supported cards found\n" );
		return;
	}

	dbprintf( "GeForceFX :: Found %s\n", asChipInfos[j].pzName );

	m_nFd = nFd;

	int nIoSize = get_pci_memory_size( nFd, &m_cPCIInfo, 0 );

	m_hRegisterArea = create_area( "geforcefx_regs", ( void ** )&m_pRegisterBase, nIoSize, AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( m_hRegisterArea, ( void * )( m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK ) );

	int nMemSize = get_pci_memory_size( nFd, &m_cPCIInfo, 1 );

	m_hFrameBufferArea = create_area( "geforcefx_framebuffer", ( void ** )&m_pFrameBufferBase, nMemSize, AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( m_hFrameBufferArea, ( void * )( m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK ) );

	memset( &m_sHW, 0, sizeof( m_sHW ) );

	m_sHW.Chipset = asChipInfos[j].nDeviceId;
	m_sHW.alphaCursor = ( ( m_sHW.Chipset & 0x0ff0 ) >= 0x0110 );
	m_sHW.Architecture = ( m_sHW.Chipset & 0x0f00 ) >> 4;
	m_sHW.FbAddress = m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
	m_sHW.IOAddress = m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	m_sHW.FbBase = ( uint8 * )m_pFrameBufferBase;
	m_sHW.FbStart = m_sHW.FbBase;

	CommonSetup();

	m_sHW.FbMapSize = m_sHW.RamAmountKBytes * 1024;
	m_sHW.FbUsableSize = m_sHW.FbMapSize - ( 128 * 1024 );
	m_sHW.ScratchBufferSize = 16384;
	m_sHW.ScratchBufferStart = m_sHW.FbUsableSize - m_sHW.ScratchBufferSize;

	os::color_space colspace[] =
	{
	CS_RGB16, CS_RGB32};
	int bpp[] = { 2, 4 };
	float rf[] = { 60.0f, 75.0f, 85.0f, 100.0f };
	for ( int i = 0; i < int ( sizeof( bpp ) / sizeof( bpp[0] ) ); i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			m_cScreenModeList.push_back( os::screen_mode( 640, 480, 640 * bpp[i], colspace[i], rf[j] ) );
			m_cScreenModeList.push_back( os::screen_mode( 800, 600, 800 * bpp[i], colspace[i], rf[j] ) );
			m_cScreenModeList.push_back( os::screen_mode( 1024, 768, 1024 * bpp[i], colspace[i], rf[j] ) );
			m_cScreenModeList.push_back( os::screen_mode( 1280, 1024, 1280 * bpp[i], colspace[i], rf[j] ) );
			m_cScreenModeList.push_back( os::screen_mode( 1600, 1200, 1600 * bpp[i], colspace[i], rf[j] ) );
		}
	}

	memset( m_anCursorShape, 0, sizeof( m_anCursorShape ) );

	CRTCout( 0x11, CRTCin( 0x11 ) | 0x80 );
	NVLockUnlock( &m_sHW, 0 );

	m_hSwapThread = -1;
	m_bSwap = false;
	m_bVideoOverlayUsed = false;
	m_bIsInitiated = true;
	
	
}

FX::~FX()
{
	if ( m_hRegisterArea != -1 )
	{
		delete_area( m_hRegisterArea );
	}
	if ( m_hFrameBufferArea != -1 )
	{
		delete_area( m_hFrameBufferArea );
	}
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

area_id FX::Open()
{
	return m_hFrameBufferArea;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void FX::Close()
{
}

int FX::GetScreenModeCount()
{
	return m_cScreenModeList.size();
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::GetScreenModeDesc( int nIndex, os::screen_mode * psMode )
{
	if ( nIndex < 0 || uint ( nIndex ) >= m_cScreenModeList.size() )
	{
		return false;
	}
	*psMode = m_cScreenModeList[nIndex];
	return true;
}

int fx_swap_entry( void* data )
{
	FX* pcFX = (FX*)data;
	pcFX->SwapThread();
	return( 0 );
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

int FX::SetScreenMode( os::screen_mode sMode )
{
	vga_regs newmode;
	FXRegPtr nvReg = &m_sHW.ModeReg;
	memset( &newmode, 0, sizeof( struct vga_regs ) );
	
	/* Stop swap thread */
	if( m_hSwapThread > -1 )
	{
		m_bSwap = false;
		wait_for_thread( m_hSwapThread );
		m_hSwapThread = -1;
	}

	int nBpp = ( sMode.m_eColorSpace == CS_RGB32 ) ? 4 : 2;
	int nHTotal = int ( sMode.m_nWidth * 1.3 );
	int nVTotal = int ( sMode.m_nHeight * 1.1 );
	int nHFreq = int ( ( sMode.m_vRefreshRate + 0.2 ) * nVTotal );
	int nPixClock = nHFreq * nHTotal / 1000;
	int nHSyncLength = ( nHTotal - sMode.m_nWidth ) / 2;
	int nRightBorder = ( nHTotal - sMode.m_nWidth - nHSyncLength ) / 3;
	int nVSyncLength = 2;
	int nBottomBorder = ( nVTotal - sMode.m_nHeight - nVSyncLength ) / 3;

	int nHSyncStart = sMode.m_nWidth + nRightBorder;
	int nVSyncStart = sMode.m_nHeight + nBottomBorder;
	int nHSyncEnd = nHSyncStart + nHSyncLength;
	int nVSyncEnd = nVSyncStart + nVSyncLength;

	int horizTotal = ( nHTotal / 8 - 1 ) - 4;
	int horizDisplay = sMode.m_nWidth / 8 - 1;
	int horizStart = nHSyncStart / 8 - 1;
	int horizEnd = nHSyncEnd / 8 - 1;
	int horizBlankStart = sMode.m_nWidth / 8 - 1;
	int horizBlankEnd = nHTotal / 8 - 1;
	int vertTotal = nVTotal - 2;
	int vertDisplay = sMode.m_nHeight - 1;
	int vertStart = nVSyncStart - 1;
	int vertEnd = nVSyncEnd - 1;
	int vertBlankStart = sMode.m_nHeight - 1;
	int vertBlankEnd = ( nVTotal - 2 ) + 1;

	if ( m_sHW.FlatPanel == 1 )
	{
		vertStart = vertTotal - 3;
		vertEnd = vertTotal - 2;
		vertBlankStart = vertStart;
		horizStart = horizTotal - 3;
		horizEnd = horizTotal - 2;
		horizBlankEnd = horizTotal + 4;
	}


	newmode.crtc[0x00] = Set8Bits( horizTotal );
	newmode.crtc[0x01] = Set8Bits( horizDisplay );
	newmode.crtc[0x02] = Set8Bits( horizBlankStart );
	newmode.crtc[0x03] = SetBitField( horizBlankEnd, 4: 0, 4:0 ) | SetBit( 7 );
	newmode.crtc[0x04] = Set8Bits( horizStart );
	newmode.crtc[0x05] = SetBitField( horizBlankEnd, 5: 5, 7: 7 ) | SetBitField( horizEnd, 4: 0, 4:0 );
	newmode.crtc[0x06] = SetBitField( vertTotal, 7: 0, 7:0 );
	newmode.crtc[0x07] = SetBitField( vertTotal, 8: 8, 0: 0 ) | SetBitField( vertDisplay, 8: 8, 1: 1 ) | SetBitField( vertStart, 8: 8, 2: 2 ) | SetBitField( vertBlankStart, 8: 8, 3: 3 ) | SetBit( 4 ) | SetBitField( vertTotal, 9: 9, 5: 5 ) | SetBitField( vertDisplay, 9: 9, 6: 6 ) | SetBitField( vertStart, 9: 9, 7:7 );
	newmode.crtc[0x09] = SetBitField( vertBlankStart, 9: 9, 5:5 ) | SetBit( 6 );
	newmode.crtc[0x10] = Set8Bits( vertStart );
	newmode.crtc[0x11] = SetBitField( vertEnd, 3: 0, 3:0 ) | SetBit( 5 );
	newmode.crtc[0x12] = Set8Bits( vertDisplay );
	newmode.crtc[0x13] = Set8Bits( ( sMode.m_nWidth / 8 ) * nBpp );
	newmode.crtc[0x15] = Set8Bits( vertBlankStart );
	newmode.crtc[0x16] = Set8Bits( vertBlankEnd );
	newmode.crtc[0x17] = 0xc3;
	newmode.crtc[0x18] = 0xff;

	newmode.gra[0x05] = 0x40;
	newmode.gra[0x06] = 0x05;
	newmode.gra[0x07] = 0x0f;
	newmode.gra[0x08] = 0xff;

	for ( int i = 0; i < 16; i++ )
	{
		newmode.attr[i] = ( uint8 )i;
	}
	newmode.attr[0x10] = 0x41;
	newmode.attr[0x11] = 0xff;
	newmode.attr[0x12] = 0x0f;
	newmode.attr[0x13] = 0x00;
	newmode.attr[0x14] = 0x00;

	if ( m_sHW.Television )
		newmode.attr[0x11] = 0x00;

	newmode.seq[0x01] = 0x01;
	newmode.seq[0x02] = 0x0f;
	newmode.seq[0x04] = 0x0e;

	nvReg->bpp = nBpp * 8;
	nvReg->width = sMode.m_nWidth;
	nvReg->height = sMode.m_nHeight;

     
	nvReg->screen = SetBitField( horizBlankEnd, 6: 6, 4: 4 ) | SetBitField( vertBlankStart, 10: 10, 3: 3 ) | SetBitField( vertStart, 10: 10, 2: 2 ) | SetBitField( vertDisplay, 10: 10, 1: 1 ) | SetBitField( vertTotal, 10: 10, 0:0 );

	nvReg->horiz = SetBitField( horizTotal, 8: 8, 0: 0 ) | SetBitField( horizDisplay, 8: 8, 1: 1 ) | SetBitField( horizBlankStart, 8: 8, 2: 2 ) | SetBitField( horizStart, 8: 8, 3:3 );

	nvReg->extra = SetBitField( vertTotal, 11: 11, 0: 0 ) | SetBitField( vertDisplay, 11: 11, 2: 2 ) | SetBitField( vertStart, 11: 11, 4: 4 ) | SetBitField( vertBlankStart, 11: 11, 6:6 );

	nvReg->interlace = 0xff;	/* interlace off */

	newmode.misc_output = 0x2f;

	m_sHW.CURSOR = ( U032 * )( m_sHW.FbStart + m_sHW.CursorStart );

	NVCalcStateExt( &m_sHW, nvReg, nBpp * 8, sMode.m_nWidth, sMode.m_nWidth, sMode.m_nHeight, nPixClock, 0 );

	nvReg->scale = m_sHW.PRAMDAC[0x00000848 / 4] & 0xfff000ff;
	if ( m_sHW.FlatPanel == 1 )
	{
		nvReg->pixel |= ( 1 << 7 );
		nvReg->scale |= ( 1 << 8 );
	}
	if ( m_sHW.CRTCnumber )
	{
		nvReg->head = m_sHW.PCRTC0[0x00000860 / 4] & ~0x00001000;
		nvReg->head2 = m_sHW.PCRTC0[0x00002860 / 4] | 0x00001000;
		nvReg->crtcOwner = 3;
		nvReg->pllsel |= 0x20000800;
		nvReg->vpll2 = nvReg->vpll;
	}
	else
	if ( m_sHW.twoHeads )
	{
		nvReg->head = m_sHW.PCRTC0[0x00000860 / 4] | 0x00001000;
		nvReg->head2 = m_sHW.PCRTC0[0x00002860 / 4] & ~0x00001000;
		nvReg->crtcOwner = 0;
		nvReg->vpll2 = m_sHW.PRAMDAC0[0x00000520 / 4];
	}
	nvReg->cursorConfig = 0x00000100;
	if ( 0 /*m_sHW.alphaCursor */  )
	{
		nvReg->cursorConfig |= 0x04011000;
		nvReg->general |= ( 1 << 29 );
		nvReg->cursorConfig |= ( 1 << 28 );
	}
	else
		nvReg->cursorConfig |= 0x02000000;

	nvReg->vpllB = 0;
	nvReg->vpll2B = 0;


	NVLockUnlock( &m_sHW, 0 );
	if ( m_sHW.twoHeads )
	{
		VGA_WR08( &m_sHW.PCIO, 0x03D4, 0x44 );
		VGA_WR08( &m_sHW.PCIO, 0x03D5, nvReg->crtcOwner );
		NVLockUnlock( &m_sHW, 0 );
	}

	/* Write registers */
	NVLoadStateExt( &m_sHW, nvReg );
	NVLockUnlock( &m_sHW, 0 );
	LoadVGAState( &newmode );

	/* Load color registers */
	for ( int i = 0; i < 256; i++ )
	{
		VGA_WR08( m_sHW.PDIO, 0x3c8, i );
		VGA_WR08( m_sHW.PDIO, 0x3c9, i );
		VGA_WR08( m_sHW.PDIO, 0x3c9, i );
		VGA_WR08( m_sHW.PDIO, 0x3c9, i );
	}

#ifndef DISABLE_HW_CURSOR
	// restore the hardware cursor
	if ( m_bUsingHWCursor )
	{
		uint32 *pnSrc = ( uint32 * )m_anCursorShape;
		volatile uint32 *pnDst = ( uint32 * )m_sHW.CURSOR;

		for ( int i = 0; i < MAX_CURS * MAX_CURS / 2; i++ )
		{
			*pnDst++ = *pnSrc++;
		}

		SetMousePos( m_cCursorPos );
		if ( m_bCursorIsOn )
		{
			MouseOn();
		}
	}
#endif

	m_sCurrentMode = sMode;
	m_sCurrentMode.m_nBytesPerLine = sMode.m_nWidth * nBpp;
	
	NVSetStartAddress( &m_sHW, m_sCurrentMode.m_nBytesPerLine * m_sCurrentMode.m_nHeight );
	
	/* Init acceleration */
	SetupAccel();
	
	m_bSwap = true;
	m_hSwapThread = spawn_thread( "geforcefx_swap_thread", (void*)fx_swap_entry, 0, 0, this );
	resume_thread( m_hSwapThread );

	return 0;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

os::screen_mode FX::GetCurrentScreenMode()
{

	return m_sCurrentMode;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::IntersectWithMouse( const IRect & cRect )
{
	return false;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void FX::SetCursorBitmap( os::mouse_ptr_mode eMode, const os::IPoint & cHotSpot, const void *pRaster, int nWidth, int nHeight )
{
#ifndef DISABLE_HW_CURSOR
	m_cCursorHotSpot = cHotSpot;
	if ( eMode != MPTR_MONO || nWidth > MAX_CURS || nHeight > MAX_CURS )
	{
#endif
		m_bUsingHWCursor = false;
		return DisplayDriver::SetCursorBitmap( eMode, cHotSpot, pRaster, nWidth, nHeight );
#ifndef DISABLE_HW_CURSOR
	}

	const uint8 *pnSrc = ( const uint8 * )pRaster;
	volatile uint32 *pnDst = ( uint32 * )m_sHW.CURSOR;
	uint16 *pnSaved = m_anCursorShape;
	uint32 *pnSaved32 = ( uint32 * )m_anCursorShape;
	static uint16 anPalette[] = { CURS_TRANSPARENT, CURS_BLACK, CURS_BLACK, CURS_WHITE };

	for ( int y = 0; y < MAX_CURS; y++ )
	{
		for ( int x = 0; x < MAX_CURS; x++, pnSaved++ )
		{
			if ( y >= nHeight || x >= nWidth )
			{
				*pnSaved = CURS_TRANSPARENT;
			}
			else
			{
				*pnSaved = anPalette[*pnSrc++];
			}
		}
	}

	for ( int i = 0; i < MAX_CURS * MAX_CURS / 2; i++ )
	{
		*pnDst++ = *pnSaved32++;
	}

	m_bUsingHWCursor = true;
#endif
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void FX::SetMousePos( os::IPoint cNewPos )
{
#ifndef DISABLE_HW_CURSOR
	m_cCursorPos = cNewPos;
	if ( !m_bUsingHWCursor )
	{
#endif
		return DisplayDriver::SetMousePos( cNewPos );
#ifndef DISABLE_HW_CURSOR
	}
	int x = cNewPos.x - m_cCursorHotSpot.x;
	int y = cNewPos.y - m_cCursorHotSpot.y;

	m_sHW.PRAMDAC[0x0000300/4] = ( y << 16 ) | ( x & 0xffff );
#endif
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void FX::MouseOn()
{
#ifndef DISABLE_HW_CURSOR
	m_bCursorIsOn = true;
	if ( !m_bUsingHWCursor )
	{
#endif
		return DisplayDriver::MouseOn();
#ifndef DISABLE_HW_CURSOR
	}
	NVShowHideCursor( &m_sHW, 1 );
#endif
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void FX::MouseOff()
{
#ifndef DISABLE_HW_CURSOR
	m_bCursorIsOn = false;
	if ( !m_bUsingHWCursor )
	{
#endif
		return DisplayDriver::MouseOff();
#ifndef DISABLE_HW_CURSOR
	}
	NVShowHideCursor( &m_sHW, 0 );
#endif
}



//-----------------------------------------------------------------------------
//                          DMA Functions
//-----------------------------------------------------------------------------

void  DmaKickoff( NVPtr pNv ) 
{
	if ( pNv->dmaCurrent != pNv->dmaPut )
	{
		pNv->dmaPut = pNv->dmaCurrent;
		WRITE_PUT( pNv, pNv->dmaPut );
	}
}
void  DmaWait(  NVPtr pNv,  uint32 size  )
{
	uint32 dmaGet;

	size++;
	while ( pNv->dmaFree < size )
	{
		dmaGet = READ_GET( pNv );
		if ( pNv->dmaPut >= dmaGet )
		{
			pNv->dmaFree = pNv->dmaMax - pNv->dmaCurrent;
			if ( pNv->dmaFree < size )
			{
				DmaNext( pNv, 0x20000000 );
				if ( !dmaGet )
				{
					if ( !pNv->dmaPut )	/* corner case - idle */
						DmaKickoff( pNv );
					
					do
					{
						dmaGet = READ_GET( pNv );
					}
					while ( !dmaGet );
				}
				WRITE_PUT( pNv, 0 );
				pNv->dmaCurrent = pNv->dmaPut = 0;
				pNv->dmaFree = dmaGet - 1;
			}
		}
		else
			pNv->dmaFree = dmaGet - pNv->dmaCurrent - 1;
	}
}


//-----------------------------------------------------------------------------
//                          Accelerated Functions
//-----------------------------------------------------------------------------

void FX::WaitForIdle()
{
	bigtime_t nTimeOut = get_system_time() + 5000;
	while(READ_GET(&m_sHW) != m_sHW.dmaPut && ( get_system_time() < nTimeOut ) );

    while(m_sHW.PGRAPH[0x0700/4] && ( get_system_time() < nTimeOut ) );
   
    /*if( get_system_time() > nTimeOut ) {
    	dbprintf( "GeForce FX :: Error: Engine timed out\n" );
    	
    }*/
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------
void FX::SwapThread()
{
	/* Enable interrupts on V-Blank */
	m_sHW.PCRTC[0x140/4] = 0x1;
	
	
	while( m_bSwap )
	{
		m_cGELock.Lock();
		
		DmaStart( &m_sHW, SURFACE_OFFSET_SRC, 2 );
		
		DmaNext( &m_sHW, 0 );
		DmaNext( &m_sHW, m_sCurrentMode.m_nBytesPerLine * m_sCurrentMode.m_nHeight );
		DmaKickoff(&m_sHW);
		
		WaitForIdle();

		DmaStart(&m_sHW, BLIT_POINT_SRC, 3);
		DmaNext (&m_sHW, ( ( 0 << 16 ) | 0 ) );
		DmaNext (&m_sHW, ( (0  << 16 ) | 0));
		DmaNext (&m_sHW, (( m_sCurrentMode.m_nHeight << 16 ) | m_sCurrentMode.m_nWidth ));
		DmaKickoff(&m_sHW);

		WaitForIdle();
		
		DmaStart( &m_sHW, SURFACE_OFFSET_SRC, 2 );
		DmaNext( &m_sHW, 0 );
		DmaNext( &m_sHW, 0 );
		DmaKickoff(&m_sHW);
		
		WaitForIdle();
		
		m_cGELock.Unlock();
		
		/* Wait for the next V-Blank event */
		while( !( m_sHW.PCRTC[0x100/4] & 0x1 ) )
		{
			snooze( 1000 );
		}
		/* Clear interrupt */
		m_sHW.PCRTC[0x100/4] = 0x1;
	}
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------
bool FX::DrawLine( SrvBitmap * pcBitMap, const IRect & cClipRect, const IPoint & cPnt1, const IPoint & cPnt2, const Color32_s & sColor, int nMode )
{
	if ( pcBitMap->m_bVideoMem == false || nMode != DM_COPY )
	{
		WaitForIdle();
		return DisplayDriver::DrawLine( pcBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode );
	}

	int x1 = cPnt1.x;
	int y1 = cPnt1.y;
	int x2 = cPnt2.x;
	int y2 = cPnt2.y;

	if ( DisplayDriver::ClipLine( cClipRect, &x1, &y1, &x2, &y2 ) == false )
	{
		return false;
	}

	uint32 nColor;

	if ( pcBitMap->m_eColorSpc == CS_RGB32 )
	{
		nColor = COL_TO_RGB32( sColor );
	}
	else
	{
		// we only support CS_RGB32 and CS_RGB16
		nColor = COL_TO_RGB16( sColor );
	}

	m_cGELock.Lock();
	
	DmaStart(&m_sHW, LINE_COLOR, 1);
	DmaNext (&m_sHW, nColor);
	DmaStart(&m_sHW, LINE_LINES(0), 4);
	DmaNext (&m_sHW, (( y1 << 16 ) | ( x1 & 0xffff )));
	DmaNext (&m_sHW, (( y2 << 16 ) | ( x2 & 0xffff )));
	DmaNext (&m_sHW, (( y2 << 16 ) | ( x2 & 0xffff )));
	DmaNext (&m_sHW, ((( y2 + 1 ) << 16 ) | ( x2 & 0xffff )));
	DmaKickoff(&m_sHW);

	WaitForIdle();
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::FillRect( SrvBitmap * pcBitMap, const IRect & cRect, const Color32_s & sColor )
{
	if ( pcBitMap->m_bVideoMem == false )
	{
		WaitForIdle();
		return DisplayDriver::FillRect( pcBitMap, cRect, sColor );
	}

	int nWidth = cRect.Width() + 1;
	int nHeight = cRect.Height() + 1;
	uint32 nColor;

	if ( pcBitMap->m_eColorSpc == CS_RGB32 )
	{
		nColor = COL_TO_RGB32( sColor );
	}
	else
	{
		// we only support CS_RGB32 and CS_RGB16
		nColor = COL_TO_RGB16( sColor );
	}

	m_cGELock.Lock();
	
	DmaStart(&m_sHW, RECT_SOLID_COLOR, 1);
	DmaNext (&m_sHW, nColor);
	DmaStart(&m_sHW, RECT_SOLID_RECTS(0), 2);
	DmaNext (&m_sHW, (( cRect.left << 16 ) | cRect.top));
	DmaNext (&m_sHW, (( nWidth << 16 ) | nHeight));
	DmaKickoff(&m_sHW);
	
	WaitForIdle();
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::BltBitmap( SrvBitmap * pcDstBitMap, SrvBitmap * pcSrcBitMap, IRect cSrcRect, IPoint cDstPos, int nMode )
{
	if ( pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || nMode != DM_COPY )
	{
		WaitForIdle();
		return DisplayDriver::BltBitmap( pcDstBitMap, pcSrcBitMap, cSrcRect, cDstPos, nMode );
	}

	int nWidth = cSrcRect.Width() + 1;
	int nHeight = cSrcRect.Height() + 1;

	m_cGELock.Lock();

	DmaStart(&m_sHW, BLIT_POINT_SRC, 3);
	DmaNext (&m_sHW, (( cSrcRect.top << 16 ) | cSrcRect.left));
	DmaNext (&m_sHW, (( cDstPos.y << 16 ) | cDstPos.x));
	DmaNext (&m_sHW, (( nHeight << 16 ) | nWidth));
	DmaKickoff(&m_sHW);

	WaitForIdle();
	m_cGELock.Unlock();
	return true;
}


//-----------------------------------------------------------------------------
//                          Video Overlay Functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::CreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer )
{
	if ( eFormat == CS_YUV422 && !m_bVideoOverlayUsed )
	{
		/* Calculate offset */
		uint32 pitch = 0;
		uint32 totalSize = 0;

		pitch = ( ( cSize.x << 1 ) + 3 ) & ~3;
		totalSize = pitch * cSize.y;

		uint32 offset = PAGE_ALIGN( m_sHW.ScratchBufferStart - totalSize - PAGE_SIZE );

		*pBuffer = create_area( "geforcefx_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, ( void * )( ( m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK ) + offset ) );


		/* Start video */

		switch ( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) )
		{
		case 16:
			m_nColorKey = COL_TO_RGB16( sColorKey );
			break;
		case 32:
			m_nColorKey = COL_TO_RGB32( sColorKey );
			break;

		default:
			delete_area( *pBuffer );
			return ( false );
		}

		/* NV30 Overlay */
		m_sHW.PMC[0x00008910 / 4] = 4096;
		m_sHW.PMC[0x00008914 / 4] = 4096;
		m_sHW.PMC[0x00008918 / 4] = 4096;
		m_sHW.PMC[0x0000891C / 4] = 4096;
		m_sHW.PMC[0x00008b00 / 4] = m_nColorKey & 0xffffff;

		m_sHW.PMC[0x8900 / 4] = offset;
		m_sHW.PMC[0x8908 / 4] = offset + totalSize;


		m_sHW.PMC[0x8800 / 4] = offset;
		m_sHW.PMC[0x8808 / 4] = offset + totalSize;

		m_sHW.PMC[0x8920 / 4] = 0;

		m_sHW.PMC[0x8928 / 4] = ( cSize.y << 16 ) | cSize.x;
		m_sHW.PMC[0x8930 / 4] = 0;
		m_sHW.PMC[0x8938 / 4] = ( cSize.x << 20 ) / cDst.Width();
		m_sHW.PMC[0x8940 / 4] = ( cSize.y << 20 ) / cDst.Height();
		m_sHW.PMC[0x8948 / 4] = ( cDst.top << 16 ) | cDst.left;
		m_sHW.PMC[0x8950 / 4] = ( cDst.Height() << 16 ) | cDst.Width(  );

		uint32 dstPitch = ( pitch ) | 1 << 20;

		m_sHW.PMC[0x8958 / 4] = dstPitch;
		m_sHW.PMC[0x8704 / 4] = 0;
		m_sHW.PMC[0x8700 / 4] = 1;

		m_bVideoOverlayUsed = true;
		m_cVideoSize = cSize;
		m_nVideoOffset = offset;
		
		return ( true );
	}
	return ( false );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::RecreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, area_id *pBuffer )
{

	if ( eFormat == CS_YUV422 )
	{
		delete_area( *pBuffer );
		/* Calculate offset */
		uint32 pitch = 0;
		uint32 totalSize = 0;

		pitch = ( ( cSize.x << 1 ) + 3 ) & ~3;
		totalSize = pitch * cSize.y;

		uint32 offset = PAGE_ALIGN( m_sHW.ScratchBufferStart - totalSize - PAGE_SIZE );

		*pBuffer = create_area( "geforcefx_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, ( void * )( ( m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK ) + offset ) );

		m_sHW.PMC[0x00008910 / 4] = 4096;
		m_sHW.PMC[0x00008914 / 4] = 4096;
		m_sHW.PMC[0x00008918 / 4] = 4096;
		m_sHW.PMC[0x0000891C / 4] = 4096;
		m_sHW.PMC[0x00008b00 / 4] = m_nColorKey & 0xffffff;

		m_sHW.PMC[0x8900 / 4] = offset;
		m_sHW.PMC[0x8908 / 4] = offset + totalSize;


		m_sHW.PMC[0x8800 / 4] = offset;
		m_sHW.PMC[0x8808 / 4] = offset + totalSize;

		m_sHW.PMC[0x8920 / 4] = 0;

		m_sHW.PMC[0x8928 / 4] = ( cSize.y << 16 ) | cSize.x;
		m_sHW.PMC[0x8930 / 4] = 0;
		m_sHW.PMC[0x8938 / 4] = ( cSize.x << 20 ) / cDst.Width();
		m_sHW.PMC[0x8940 / 4] = ( cSize.y << 20 ) / cDst.Height();
		m_sHW.PMC[0x8948 / 4] = ( cDst.top << 16 ) | cDst.left;
		m_sHW.PMC[0x8950 / 4] = ( cDst.Height() << 16 ) | cDst.Width(  );

		uint32 dstPitch = ( ( ( cSize.x << 1 ) + 63 ) & ~63 ) | 1 << 20;

		m_sHW.PMC[0x8958 / 4] = dstPitch;
		m_sHW.PMC[0x00008704 / 4] = 0;
		m_sHW.PMC[0x8704 / 4] = 0xfffffffe;
		m_sHW.PMC[0x8700 / 4] = 1;


		m_bVideoOverlayUsed = true;
		m_cVideoSize = cSize;
		m_nVideoOffset = offset;
		return ( true );
	}
	return ( false );
}



typedef volatile struct {
	uint64 timeStamp; /* nanoseconds since Jan. 1, 1970 */
	uint32 returnVal; /* NVX				 */
	uint16 errorCode; /* NVX				 */
	uint8  reserved;  /* zero				 */
	uint8  status;    /* NV_NOTIFICATION_STATUS_*	 */
} NvNotification;

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool FX::UploadVideoData( area_id *phArea, area_id hSrcArea, uint32 nBytesPerRow )
{
	
	/* DOES NOT WORK YET! */
	return( false );
	#if 0
	/* Create notification pointer */
	NvNotification* pNotify = NULL;
	area_id hNotifyArea = create_area( "geforcefx_notify", (void**)&pNotify, PAGE_ALIGN( sizeof( NvNotification ) ), AREA_FULL_ACCESS, AREA_NO_LOCK );
	area_id hArea = hNotifyArea;
	memset( (void*)pNotify, 0, sizeof( NvNotification ) );
		
	
	/*uint8* pMem = NULL;
	area_id hMem = create_area( "geforcefx_notify", (void**)&pMem, PAGE_ALIGN( 100 * 100 * 4 ), AREA_FULL_ACCESS, AREA_NO_LOCK );
	memset( (void*)pMem, 0, 100 * 100 * 4 );
	*/
	area_id hAreaMem = hSrcArea;
	
	/* Get address of the source area */
		
	if( ioctl( m_nFd, FX_GET_DMA_ADDRESS, &hArea ) == 0 &&
		ioctl( m_nFd, FX_GET_DMA_ADDRESS, &hAreaMem ) == 0 )
	{
		//dbprintf( "%x\n", (uint)hArea );
		//dbprintf( "%x\n", (uint)hAreaMem );
		
		
		/* Update notifier object */		
		m_sHW.PRAMIN[0x0828] = 0x0002303e;
		m_sHW.PRAMIN[0x0829] = 0x000000ff;
		m_sHW.PRAMIN[0x082a] = (uint32)hArea | 0x00000002;
    	m_sHW.PRAMIN[0x082b] = 0x00000002;
    
		m_sHW.PRAMIN[0x0010] = 0x80000018;
	    m_sHW.PRAMIN[0x0011] = 0x8000120a;
    
   		/* Update source object */
		m_sHW.PRAMIN[0x0830] = 0x0002303e;
		m_sHW.PRAMIN[0x0831] = ( 720 * 576 * 4 );
		m_sHW.PRAMIN[0x0832] = (uint32)hAreaMem | 0x00000002;
		m_sHW.PRAMIN[0x0833] = 0x00000002;
    
	    m_sHW.PRAMIN[0x0014] = 0x8000001a;
    	m_sHW.PRAMIN[0x0015] = 0x8000120c;
    	
    	/* Update dest object */
		m_sHW.PRAMIN[0x082c] = 0x0000303d;
		m_sHW.PRAMIN[0x082d] = ( 720 * 576 * 4 );
		m_sHW.PRAMIN[0x082e] = m_nVideoOffset | 0x00000002;
		m_sHW.PRAMIN[0x082f] = 0xffffffff;
		
		m_sHW.PRAMIN[0x0012] = 0x80000019;
    m_sHW.PRAMIN[0x0013] = 0x8000120b;
    
		pNotify->status = 0xff;
		
		
	
		/* Enable notifier */
		m_cGELock.Lock();	
		DmaStart(&m_sHW, 0xe104, 1);
		DmaNext (&m_sHW, 0x0);
		DmaKickoff(&m_sHW);
		m_cGELock.Unlock();
		WaitForIdle();
			
		//dbprintf( "Get: %x Put: %x\n", READ_GET(&m_sHW), m_sHW.dmaPut );
		//dbprintf("Starting...\n" );
		
		/* Set transfer parameters */
		m_cGELock.Lock();		
		DmaStart(&m_sHW, 0xe30c, 0x7);
		DmaNext (&m_sHW, 0x0); /* source offset */
		DmaNext (&m_sHW, 0x0 ); /* dest offset */
		DmaNext (&m_sHW, m_cVideoSize.x * 2 ); /* source pitch */
		DmaNext (&m_sHW, m_cVideoSize.x * 2 ); /* dest pitch */
		DmaNext (&m_sHW, m_cVideoSize.x * 2 ); /* linesize */
		DmaNext (&m_sHW, m_cVideoSize.y ); /* rows */
		DmaNext (&m_sHW, 0x101); /* 1 byte format */
		DmaKickoff(&m_sHW);
			
		WaitForIdle();
		
		
		/* Start transfer */	
		DmaStart(&m_sHW, 0xe328, 0x1);
		DmaNext (&m_sHW, 0x0);
		DmaKickoff(&m_sHW);
		
/*		DmaStart(&m_sHW, 0xe100, 0x1);
		DmaNext (&m_sHW, 0x0);
		DmaKickoff(&m_sHW);*/
			
		m_cGELock.Unlock();
		
		int i = 100000;
		while( pNotify->status && i-- ) {};
			
			
		//dbprintf( "Get: %x Put: %x\n", READ_GET(&m_sHW), m_sHW.dmaPut );
			
		dbprintf( "%x\n", pNotify->status );
	
		WaitForIdle();
			
		dbprintf( "%x\n", pNotify->status );
			
		//dbprintf( "Get: %x Put: %x\n", READ_GET(&m_sHW), m_sHW.dmaPut );
			
		delete_area( hNotifyArea );
		return( true );
	} else {
		delete_area( hNotifyArea );
		return( false );
	}
	#endif
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void FX::UpdateVideoOverlay( area_id *pBuffer )
{
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void FX::DeleteVideoOverlay( area_id *pBuffer )
{
	if ( m_bVideoOverlayUsed )
	{
		delete_area( *pBuffer );


		m_sHW.PMC[0x00008704 / 4] = 1;

	}
	m_bVideoOverlayUsed = false;
}


//-----------------------------------------------------------------------------
//                          Private Functions
//-----------------------------------------------------------------------------

void FX::LoadVGAState( struct vga_regs *regs )
{
	MISCout( regs->misc_output );

	for ( int i = 1; i < NUM_SEQ_REGS; i++ )
	{
		SEQout( i, regs->seq[i] );
	}

	CRTCout( 17, regs->crtc[17] & ~0x80 );

	for ( int i = 0; i < 25; i++ )
	{
		CRTCout( i, regs->crtc[i] );
	}

	for ( int i = 0; i < NUM_GRC_REGS; i++ )
	{
		GRAout( i, regs->gra[i] );
	}

//      DisablePalette();       
	for ( int i = 0; i < NUM_ATC_REGS; i++ )
	{
		ATTRout( i, regs->attr[i] );
	}

}


//-----------------------------------------------------------------------------

void FX::SetupAccel()
{
	m_sHW.dmaBase = ( uint32 * )( &m_sHW.FbStart[m_sHW.FbUsableSize] );
	uint32 surfaceFormat, patternFormat, rectFormat, lineFormat;
	int pitch = m_sCurrentMode.m_nWidth * ( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) >> 3 );

	
	/* Initialize engine objects */
	m_sHW.dmaBase[0x0] = 0x00040000;
	m_sHW.dmaBase[0x1] = 0x80000010;
	m_sHW.dmaBase[0x2] = 0x00042000;
	m_sHW.dmaBase[0x3] = 0x80000011;
	m_sHW.dmaBase[0x4] = 0x00044000;
	m_sHW.dmaBase[0x5] = 0x80000012;
	m_sHW.dmaBase[0x6] = 0x00046000;
	m_sHW.dmaBase[0x7] = 0x80000013;
	m_sHW.dmaBase[0x8] = 0x00048000;
	m_sHW.dmaBase[0x9] = 0x80000014;
	m_sHW.dmaBase[0xA] = 0x0004A000;
	m_sHW.dmaBase[0xB] = 0x80000015;
	m_sHW.dmaBase[0xC] = 0x0004C000;
	m_sHW.dmaBase[0xD] = 0x80000016;
	m_sHW.dmaBase[0xE] = 0x0004E000;
	//m_sHW.dmaBase[0xF] = 0x80000017;
	m_sHW.dmaBase[0xF] = 0x8000001b;
	m_sHW.dmaPut = 0;
	m_sHW.dmaCurrent = 16;
	m_sHW.dmaMax = 8191;
	m_sHW.dmaFree = 8175;
	switch ( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) )
	{
	case 32:
	case 24:
		surfaceFormat = SURFACE_FORMAT_DEPTH24;
		patternFormat = PATTERN_FORMAT_DEPTH24;
		rectFormat = RECT_FORMAT_DEPTH24;
		lineFormat = LINE_FORMAT_DEPTH24;
		break;
	case 16:
	case 15:
		surfaceFormat = SURFACE_FORMAT_DEPTH16;
		patternFormat = PATTERN_FORMAT_DEPTH16;
		rectFormat = RECT_FORMAT_DEPTH16;
		lineFormat = LINE_FORMAT_DEPTH16;
		break;
	default:
		surfaceFormat = SURFACE_FORMAT_DEPTH8;
		patternFormat = PATTERN_FORMAT_DEPTH8;
		rectFormat = RECT_FORMAT_DEPTH8;
		lineFormat = LINE_FORMAT_DEPTH8;
		break;
	}
	DmaStart( &m_sHW, SURFACE_FORMAT, 4 );
	DmaNext( &m_sHW, surfaceFormat );
	DmaNext( &m_sHW, pitch | ( pitch << 16 ) );
	DmaNext( &m_sHW, 0 );
	DmaNext( &m_sHW, 0 );
	DmaStart( &m_sHW, PATTERN_FORMAT, 1 );
	DmaNext( &m_sHW, patternFormat );
	DmaStart( &m_sHW, RECT_FORMAT, 1 );
	DmaNext( &m_sHW, rectFormat );
	DmaStart( &m_sHW, LINE_FORMAT, 1 );
	DmaNext( &m_sHW, lineFormat );
	m_sHW.currentRop = ~0;	/* set to something invalid */
	
		/* Set solid color mode */
		DmaStart( &m_sHW, PATTERN_COLOR_0, 4 );
	DmaNext( &m_sHW, ~0 );
	DmaNext( &m_sHW, ~0 );
	DmaNext( &m_sHW, ~0 );
	DmaNext( &m_sHW, ~0 );
	DmaStart( &m_sHW, ROP_SET, 1 );
	DmaNext( &m_sHW, 0xCA );
	DmaKickoff( &m_sHW );
}


//-----------------------------------------------------------------------------

bool FX::IsConnected( int output ) 
{
	volatile U032 *PRAMDAC = m_sHW.PRAMDAC0;
	CARD32 reg52C, reg608;
	bool present;

	dbprintf( "GeForce FX :: Probing for analog device on output %s...\n", output ? "B" : "A" );
	if ( output )
		PRAMDAC += 0x800;
	reg52C = PRAMDAC[0x052C / 4];
	reg608 = PRAMDAC[0x0608 / 4];
	PRAMDAC[0x0608 / 4] = reg608 & ~0x00010000;
	PRAMDAC[0x052C / 4] = reg52C & 0x0000FEEE;
	snooze( 1000 );
	PRAMDAC[0x052C / 4] |= 1;
	m_sHW.PRAMDAC0[0x0610 / 4] = 0x94050140;
	m_sHW.PRAMDAC0[0x0608 / 4] |= 0x00001000;
	snooze( 1000 );
	present = ( PRAMDAC[0x0608 / 4] & ( 1 << 28 ) ) ? true : false;
	if ( present )
		dbprintf( "GeForce FX ::  ...found one\n" );
	
	else
		dbprintf( "GeForce FX ::  ...can't find one\n" );
	m_sHW.PRAMDAC0[0x0608 / 4] &= 0x0000EFFF;
	PRAMDAC[0x052C / 4] = reg52C;
	PRAMDAC[0x0608 / 4] = reg608;
	return present;
}


//-----------------------------------------------------------------------------
void FX::CommonSetup()
{
	uint16 nImplementation = m_sHW.Chipset & 0x0ff0;
	bool bMobile = false;
	int FlatPanel = -1;
	bool Television = false;
	bool tvA = false;
	bool tvB = false;

	/*
	 * Register mappings
	 */

	m_sHW.PRAMIN = ( unsigned * )( m_pRegisterBase + 0x00710000 );
	m_sHW.PCRTC0 = ( unsigned * )( m_pRegisterBase + 0x00600000 );
	m_sHW.PRAMDAC0 = ( unsigned * )( m_pRegisterBase + 0x00680000 );
	m_sHW.PFB = ( unsigned * )( m_pRegisterBase + 0x00100000 );

	m_sHW.PFIFO = ( unsigned * )( m_pRegisterBase + 0x00002000 );
	m_sHW.PGRAPH = ( unsigned * )( m_pRegisterBase + 0x00400000 );
	m_sHW.PEXTDEV = ( unsigned * )( m_pRegisterBase + 0x00101000 );
	m_sHW.PTIMER = ( unsigned * )( m_pRegisterBase + 0x00009000 );
	m_sHW.PMC = ( unsigned * )( m_pRegisterBase + 0x00000000 );
	m_sHW.FIFO = ( unsigned * )( m_pRegisterBase + 0x00800000 );
	m_sHW.PCIO0 = ( U008 * )( m_pRegisterBase + 0x00601000 );
	m_sHW.PDIO0 = ( U008 * )( m_pRegisterBase + 0x00681000 );
	m_sHW.PVIO = ( U008 * )( m_pRegisterBase + 0x000C0000 );

	m_sHW.twoHeads = ( nImplementation >= 0x0110 ) && ( nImplementation != 0x0150 ) && ( nImplementation != 0x01A0 ) && ( nImplementation != 0x0200 );

	switch ( m_sHW.Chipset )
	{
	case 0x0316:
	case 0x0317:
	case 0x031A:
	case 0x031B:
	case 0x031C:
	case 0x031D:
	case 0x031E:
	case 0x031F:
	case 0x0324:
	case 0x0325:
	case 0x0328:
	case 0x0329:
	case 0x032C:
	case 0x032D:
	case 0x0347:
	case 0x0348:
	case 0x0349:
	case 0x034B:
	case 0x034C:
		bMobile = true;
		break;
	default:
		break;
	}

	
		/* Get memory size */
		m_sHW.RamAmountKBytes = ( m_sHW.PFB[0x020C / 4] & 0xFFF00000 )>>10;
	m_sHW.CrystalFreqKHz = ( m_sHW.PEXTDEV[0x0000 / 4] & ( 1 << 6 ) ) ? 14318 : 13500;
	if ( m_sHW.PEXTDEV[0x0000 / 4] & ( 1 << 22 ) )
		m_sHW.CrystalFreqKHz = 27000;
	m_sHW.CursorStart = ( m_sHW.RamAmountKBytes - 96 ) * 1024;
	m_sHW.CURSOR = NULL;	/* can't set this here */
	m_sHW.MinVClockFreqKHz = 12000;
	m_sHW.MaxVClockFreqKHz = 350000;
	
		/* Select first head */
		m_sHW.PCIO = m_sHW.PCIO0;
	m_sHW.PCRTC = m_sHW.PCRTC0;
	m_sHW.PRAMDAC = m_sHW.PRAMDAC0;
	m_sHW.PDIO = m_sHW.PDIO0;

	m_sHW.Television = false;
	m_sHW.FlatPanel = -1;
	m_sHW.CRTCnumber = -1;

	if ( !m_sHW.twoHeads )
	{
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x28 );
		if ( VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x80 )
		{
			VGA_WR08( m_sHW.PCIO, 0x03D4, 0x33 );
			if ( !( VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x01 ) )
				Television = true;
			FlatPanel = 1;
		}
		else
		{
			FlatPanel = 0;
		}
		dbprintf( "GeForce FX :: %s connected\n", FlatPanel ? ( Television ? "TV" : "LCD" ) : "CRT" );
		m_sHW.FlatPanel = FlatPanel;
		m_sHW.Television = Television;
	}
	else
	{
		uint8 outputAfromCRTC, outputBfromCRTC;
		int CRTCnumber = -1;
		uint8 slaved_on_A, slaved_on_B;
		bool analog_on_A, analog_on_B;
		uint32 oldhead;
		uint8 cr44;

		if ( nImplementation != 0x0110 )
		{
			if ( m_sHW.PRAMDAC0[0x0000052C / 4] & 0x100 )
				outputAfromCRTC = 1;
			
			else
				outputAfromCRTC = 0;
			if ( m_sHW.PRAMDAC0[0x0000252C / 4] & 0x100 )
				outputBfromCRTC = 1;
			
			else
				outputBfromCRTC = 0;
			analog_on_A = IsConnected( 0 );
			analog_on_B = IsConnected( 1 );
		}
		else
		{
			outputAfromCRTC = 0;
			outputBfromCRTC = 1;
			analog_on_A = false;
			analog_on_B = false;
		}
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x44 );
		cr44 = VGA_RD08( m_sHW.PCIO, 0x03D5 );
		VGA_WR08( m_sHW.PCIO, 0x03D5, 3 );
		
			/* Select second head */
			m_sHW.PCIO = m_sHW.PCIO0 + 0x2000;
		m_sHW.PCRTC = m_sHW.PCRTC0 + 0x800;
		m_sHW.PRAMDAC = m_sHW.PRAMDAC0 + 0x800;
		m_sHW.PDIO = m_sHW.PDIO0 + 0x2000;
		NVLockUnlock( &m_sHW, 0 );
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x28 );
		slaved_on_B = VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x80;
		if ( slaved_on_B )
		{
			VGA_WR08( m_sHW.PCIO, 0x03D4, 0x33 );
			tvB = !( VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x01 );
		}
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x44 );
		VGA_WR08( m_sHW.PCIO, 0x03D5, 0 );
		
			/* Select first head */
			m_sHW.PCIO = m_sHW.PCIO0;
		m_sHW.PCRTC = m_sHW.PCRTC0;
		m_sHW.PRAMDAC = m_sHW.PRAMDAC0;
		m_sHW.PDIO = m_sHW.PDIO0;
		NVLockUnlock( &m_sHW, 0 );
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x28 );
		slaved_on_A = VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x80;
		if ( slaved_on_A )
		{
			VGA_WR08( m_sHW.PCIO, 0x03D4, 0x33 );
			tvA = !( VGA_RD08( m_sHW.PCIO, 0x03D5 ) & 0x01 );
		}
		oldhead = m_sHW.PCRTC0[0x00000860 / 4];
		m_sHW.PCRTC0[0x00000860 / 4] = oldhead | 0x00000010;
		if ( slaved_on_A )
		{
			CRTCnumber = 0;
			FlatPanel = 1;
			Television = tvA;
			dbprintf( "GeForce FX :: CRTC 0 is currently programmed for %s\n", Television ? "TV" : "DFP" );
		}
		else
		if ( slaved_on_B )
		{
			CRTCnumber = 1;
			FlatPanel = 1;
			Television = tvB;
			dbprintf( "GeForce FX :: CRTC 1 is currently programmed for %s\n", Television ? "TV" : "DFP" );
		}
		else
		if ( analog_on_A )
		{
			CRTCnumber = outputAfromCRTC;
			FlatPanel = 0;
			dbprintf( "GeForce FX :: CRTC %i appears to have a CRT attached\n", CRTCnumber );
		}
		else
		if ( analog_on_B )
		{
			CRTCnumber = outputBfromCRTC;
			FlatPanel = 0;
			dbprintf( "GeForce FX :: CRTC %i appears to have a CRT attached\n", CRTCnumber );
		}
		if ( m_sHW.FlatPanel == -1 )
		{
			if ( FlatPanel != -1 )
			{
				m_sHW.FlatPanel = FlatPanel;
				m_sHW.Television = Television;
			}
			else
			{
				dbprintf( "GeForce FX :: Unable to detect display type...\n" );
				if ( bMobile )
				{
					dbprintf( "GeForce FX :: ...On a laptop, assuming DFP\n" );
					m_sHW.FlatPanel = 1;
				}
				else
				{
					dbprintf( "GeForce FX :: ...Using default of CRT\n" );
					m_sHW.FlatPanel = 0;
				}
			}
		}
		if ( m_sHW.CRTCnumber == -1 )
		{
			if ( CRTCnumber != -1 )
				m_sHW.CRTCnumber = CRTCnumber;
			
			else
			{
				dbprintf( "GeForce FX :: Unable to detect which CRTCNumber...\n" );
				if ( m_sHW.FlatPanel )
					m_sHW.CRTCnumber = 1;
				
				else
					m_sHW.CRTCnumber = 0;
				dbprintf( "GeForce FX ::...Defaulting to CRTCNumber %i\n", m_sHW.CRTCnumber );
			}
		}
		else
		{
			dbprintf( "GeForce FX :: Forcing CRTCNumber %i as specified\n", m_sHW.CRTCnumber );
		}
		if ( nImplementation == 0x0110 )
			cr44 = m_sHW.CRTCnumber * 0x3;
		m_sHW.PCRTC0[0x00000860 / 4] = oldhead;
		VGA_WR08( m_sHW.PCIO, 0x03D4, 0x44 );
		VGA_WR08( m_sHW.PCIO, 0x03D5, cr44 );
		if ( m_sHW.CRTCnumber == 1 )
		{
			/* Select second head */
			m_sHW.PCIO = m_sHW.PCIO0 + 0x2000;
			m_sHW.PCRTC = m_sHW.PCRTC0 + 0x800;
			m_sHW.PRAMDAC = m_sHW.PRAMDAC0 + 0x800;
			m_sHW.PDIO = m_sHW.PDIO0 + 0x2000;
		}
		else
		{
			/* Select first head */
			m_sHW.PCIO = m_sHW.PCIO0;
			m_sHW.PCRTC = m_sHW.PCRTC0;
			m_sHW.PRAMDAC = m_sHW.PRAMDAC0;
			m_sHW.PDIO = m_sHW.PDIO0;
		}

	}

	dbprintf( "Using %s on CRTC %i\n", m_sHW.FlatPanel ? ( m_sHW.Television ? "TV" : "DFP" ) : "CRT", m_sHW.CRTCnumber );
	m_sHW.FPDither = FALSE;
}


//-----------------------------------------------------------------------------
extern "C" DisplayDriver * init_gfx_driver( int nFd )
{
	dbprintf( "geforcefx attempts to initialize\n" );

	try
	{
		FX *pcDriver = new FX( nFd );

		if ( pcDriver->IsInitiated() )
		{
			return pcDriver;
		}
		return NULL;
	}
	catch( std::exception & cExc )
	{
		dbprintf( "Got exception\n" );
		return NULL;
	}
}
