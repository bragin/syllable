/*
 *  ATA/ATAPI driver for Syllable
 *
 *	Copyright (C) 2004 Arno Klenke
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *	Copyright (C) 2003 Red Hat, Inc.  All rights reserved.
 *	Copyright (C) 2003 Jeff Garzik
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
 */

#include "ata.h"
#include "ata_private.h"


extern bool g_bEnableLBA48bit;
extern bool g_bEnablePIO32bit;
extern bool g_bEnableDMA;

/* Cable types */
char g_zCable[][50] = {
	"none",
	"standard",
	"40pin",
	"80pin",
	"SATA"
};

/* Print a string with the supported drive speeds */

#define DRIVE_SUPPORTS( i ) ( psPort->nSupportedDriveSpeed & ( 1 << ( i ) ) )

void ata_print_drive_speed( ATA_port_s* psPort )
{
	char zString[128];
	memset( zString, 0, 128 );
	
	if( DRIVE_SUPPORTS( ATA_SPEED_PIO ) )
		strcat( zString, "PIO " );
	
	/* PIO */
	if( psPort->nSupportedDriveSpeed & 0xe )
		strcat( zString, "( " );
	if( DRIVE_SUPPORTS( ATA_SPEED_PIO_3 ) )
		strcat( zString, "3 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_PIO_4 ) )
		strcat( zString, "4 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_PIO_5 ) )
		strcat( zString, "5 " );
	if( psPort->nSupportedDriveSpeed & 0xe )
		strcat( zString, ") " );
	
	/* MWDMA */
	if( psPort->nSupportedDriveSpeed & 0xe0 )
		strcat( zString, "MWDMA ( " );
	if( DRIVE_SUPPORTS( ATA_SPEED_MWDMA_0 ) )
		strcat( zString, "0 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_MWDMA_1 ) )
		strcat( zString, "1 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_MWDMA_2 ) )
		strcat( zString, "2 " );
	if( psPort->nSupportedDriveSpeed & 0xe0 )
		strcat( zString, ") " );

	/* UDMA */
	if( psPort->nSupportedDriveSpeed & 0xff00 )
		strcat( zString, "UDMA ( " );
	if( DRIVE_SUPPORTS( ATA_SPEED_UDMA_0 ) )
		strcat( zString, "0 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_UDMA_1 ) )
		strcat( zString, "1 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_UDMA_2 ) )
		strcat( zString, "2 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_UDMA_3 ) )
		strcat( zString, "3 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_UDMA_4 ) )
		strcat( zString, "4 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_UDMA_5 ) )
		strcat( zString, "5 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_UDMA_6 ) )
		strcat( zString, "6 " );
	if( DRIVE_SUPPORTS( ATA_SPEED_UDMA_7 ) )
		strcat( zString, "7 " );
	if( psPort->nSupportedDriveSpeed & 0xff00 )
		strcat( zString, ")" );
		
	kerndbg( KERN_INFO, "Modes: %s\n", zString );
}

/* Reset atapi devices */
void ata_probe_atapi_reset( ATA_port_s* psPort )
{
	ATA_WRITE_REG( psPort, ATA_REG_FEATURE, 0 );
	ATA_WRITE_REG( psPort, ATAPI_REG_IRR, 0 );
	ATA_WRITE_REG( psPort, ATAPI_REG_SAMTAG, 0 );
	ATA_WRITE_REG( psPort, ATAPI_REG_COUNT_LOW, 0 );
	ATA_WRITE_REG( psPort, ATAPI_REG_COUNT_HIGH, 0 );
	
	ATA_WRITE_REG( psPort, ATA_REG_COMMAND, ATAPI_CMD_RESET );
	
	ata_io_wait( psPort, ATA_STATUS_BUSY, 0 );
}

/* Configure one drive */
status_t ata_probe_configure_drive( ATA_port_s* psPort )
{
	int i;
	
	uint8 nSpeed = 0;
	psPort->nCurrentSpeed = ATA_SPEED_PIO;
	
	if( !( psPort->nDevice == ATA_DEV_ATAPI || psPort->nDevice == ATA_DEV_ATA ) )
		return 0;
			
	/* Select drive */
	psPort->sOps.select( psPort, 0 );
	
	if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) < 0 )
		return( -1 );
		
	/* Reset atapi drives */
	if( psPort->nDevice == ATA_DEV_ATAPI )
		 ata_probe_atapi_reset( psPort );
		 
	
	for( i = ATA_SPEED_HIGHEST; i >= 0; i-- )
	{
		if( ( psPort->nSupportedPortSpeed & ( 1 << i ) ) &&
			( psPort->nSupportedDriveSpeed & ( 1 << i ) ) )
		{
			if( i == ATA_SPEED_PIO ) {
				kerndbg( KERN_INFO, "%s: Using standard PIO mode\n", psPort->zDeviceName );
				return( 0 );
			}
			if( i == ATA_SPEED_DMA ) {
				psPort->nCurrentSpeed = ATA_SPEED_DMA;
				kerndbg( KERN_INFO, "%s: Using standard DMA mode\n", psPort->zDeviceName );
				return( 0 );
			}
			if( i >= ATA_SPEED_UDMA_0 ) {
				kerndbg( KERN_INFO, "%s: Using Ultra DMA mode %i\n", psPort->zDeviceName, i - ATA_SPEED_UDMA_0 );
				nSpeed = ATA_XFER_UDMA_0 + ( i - ATA_SPEED_UDMA_0 );
			} else if( i >= ATA_SPEED_MWDMA_0 ) {
				kerndbg( KERN_INFO, "%s: Using Multiword DMA mode %i\n", psPort->zDeviceName, i - ATA_SPEED_MWDMA_0 );
				nSpeed = ATA_XFER_MWDMA_0 + ( i - ATA_SPEED_MWDMA_0 );
			} else if( i >= ATA_SPEED_PIO_3 ) {
				kerndbg( KERN_INFO, "%s: Using PIO mode %i\n", psPort->zDeviceName, i + 2 );
				nSpeed = ATA_XFER_PIO_3 + ( i - 1 );
			}
			
			
			ATA_WRITE_REG( psPort, ATA_REG_COUNT, nSpeed )
			ATA_WRITE_REG( psPort, ATA_REG_LBA_LOW, 0 )
			ATA_WRITE_REG( psPort, ATA_REG_LBA_MID, 0 )
			ATA_WRITE_REG( psPort, ATA_REG_LBA_HIGH, 0 )
			ATA_WRITE_REG( psPort, ATA_REG_FEATURE, 0x03 )
			ATA_WRITE_REG( psPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT | ATA_CONTROL_INTDISABLE )
			ATA_WRITE_REG( psPort, ATA_REG_COMMAND, ATA_CMD_SET_FEATURES )
			if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) < 0 )
				return( -1 );
			
			psPort->nCurrentSpeed = i;
			
			return( 0 );
		}
	}
	kerndbg( KERN_INFO, "%s: Using current speed\n", psPort->zDeviceName );
	return( 0 );
}


/* Probe for drive */
void ata_probe_port( ATA_port_s* psPort )
{
	uint8 nLbaHigh, nLbaMid, nStatus;
	uint8 nID[512];
	ATA_identify_info_s* psID;
	
	if( psPort->bConfigured )
		return;
	
	/* Lock port */
	LOCK( psPort->hPortLock );
	
	
	/* Reset port */
	if( psPort->sOps.reset( psPort ) != 0 )
	{
		kerndbg( KERN_DEBUG_LOW, "No device connected on %i:%i\n", psPort->nChannel, psPort->nPort );
		psPort->nDevice = ATA_DEV_NONE;
		psPort->nCable = ATA_CABLE_NONE;
		UNLOCK( psPort->hPortLock );
		return;
	}
	
	/* Read registers */
	ATA_READ_REG( psPort, ATA_REG_LBA_HIGH, nLbaHigh )
	ATA_READ_REG( psPort, ATA_REG_LBA_MID, nLbaMid )
	ATA_READ_REG( psPort, ATA_REG_STATUS, nStatus )
	
	if( ( ( nLbaMid == 0x00 && nLbaHigh == 0x00 ) ||
		( nLbaMid == 0xc3 && nLbaHigh == 0xc3 ) ) && nStatus != 0 )
	{
		psPort->nDevice = ATA_DEV_ATA;
		kerndbg( KERN_INFO, "ATA device connected on %s cable\n", g_zCable[psPort->nCable] );
	} else 
	{
		if( ( nLbaMid == 0x14 && nLbaHigh == 0xeb ) ||
		( nLbaMid == 0x69 && nLbaHigh == 0x96 ) )
		{
			psPort->nDevice = ATA_DEV_ATAPI;
			kerndbg( KERN_INFO, "ATAPI device connected on %s cable\n", g_zCable[psPort->nCable] );
		} else {
			psPort->nDevice = ATA_DEV_NONE;
			psPort->nCable = ATA_CABLE_NONE;
			kerndbg( KERN_DEBUG_LOW, "No device connected on %i:%i\n", psPort->nChannel, psPort->nPort );
			goto end;
		}
	} 
	
	psPort->bConfigured = true;
	
	if( psPort->nDevice == ATA_DEV_UNKNOWN )
		goto end;
		
	/* Select drive */
	psPort->sOps.select( psPort, 0 );
	
	if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 )
		goto err_dev_data;
		
	/* Put identification command and read response data */
	if( psPort->nDevice == ATA_DEV_ATAPI )
		ATA_WRITE_REG( psPort, ATA_REG_COMMAND, ATAPI_CMD_IDENTIFY )
	else
		ATA_WRITE_REG( psPort, ATA_REG_COMMAND, ATA_CMD_IDENTIFY )
		
	if( ata_io_read( psPort, &nID[0], 512 ) != 512 )
		goto err_dev_data;
	
	psID = (ATA_identify_info_s*)&nID[0];
	
	/* Get drive name */
	extract_model_id( psPort->zDeviceName, psID->model_id );
	
	kerndbg( KERN_INFO, "Name: %s\n", psPort->zDeviceName );
	
	/* Check that the device supports LBA */
	if( psPort->nDevice == ATA_DEV_ATA && !( psID->capabilities & 0x02 ) ) {
		kerndbg( KERN_WARNING, "Error: Device does not support LBA mode\n" );
		goto err_id;
	}
	
	/* Add supported speeds */
	psPort->nSupportedDriveSpeed = 1 << ATA_SPEED_PIO;
	if( psID->valid & 0x02 )
	{
		psPort->nSupportedDriveSpeed |= ( psID->eide_pio_modes & 0x7 ) << ATA_SPEED_PIO_3;
		psPort->nSupportedDriveSpeed |= ( psID->multi_word_dma_info & 0x7 ) << ATA_SPEED_MWDMA_0;
		
		if( psID->multi_word_dma_info & 0x7 )
			psPort->nSupportedDriveSpeed |= 1 << ATA_SPEED_DMA;
	}
	if( psID->valid & 0x04 )
	{
		psPort->nSupportedDriveSpeed |= 1 << ATA_SPEED_DMA;
		psPort->nSupportedDriveSpeed |= ( psID->ultra_dma_modes & 0xff ) << ATA_SPEED_UDMA_0;
	}
	
	ata_print_drive_speed( psPort );
	
	/* Check for 48bit addressing */
	if( psID->command_set_2 & 0x400 )
	{
		kerndbg( KERN_INFO, "%s: Drive uses 48bit addressing", g_bEnableLBA48bit ? "Warning" : "Error" );
		kerndbg( KERN_INFO, "Capacity: %i Mb\n", (int)( psID->lba_capacity_48 / 1000 * 512 / 1000 ) );
		psPort->bLBA48bit = true;
		if( !g_bEnableLBA48bit )
			goto err_id;
	} else
		if( psID->lba_sectors )
			kerndbg( KERN_INFO, "Capacity: %i Mb\n", (int)( psID->lba_sectors / 1000 * 512 / 1000 ) );

	psPort->bPIO32bit = g_bEnablePIO32bit;

	memcpy( &psPort->sID, psID, sizeof( ATA_identify_info_s ) );
	
	/* Check if dma is disabled */
	if( !g_bEnableDMA )
		psPort->nSupportedPortSpeed = ( 1 << ATA_SPEED_PIO );
	
	goto end;
err_dev_data:
	kerndbg( KERN_FATAL, "Could not get drive data\n" );
	
err_id:
	
	psPort->nDevice = ATA_DEV_UNKNOWN;
end:
	/* Unlock port */
	UNLOCK( psPort->hPortLock );
}

















































