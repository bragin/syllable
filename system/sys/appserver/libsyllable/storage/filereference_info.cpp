/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/fs_attribs.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/dropdownmenu.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/image.h>
#include <gui/imageview.h>
#include <util/message.h>
#include <util/messenger.h>
#include <util/string.h>
#include <storage/registrar.h>
#include <storage/path.h>
#include <storage/filereference.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cassert>
#include <sys/stat.h>
#include <time.h>
#include <string>

using namespace os;

class InfoWin : public os::Window
{
public:
	InfoWin( os::Rect cFrame, os::String zFile, const os::Messenger & cViewTarget, os::Message* pcChangeMsg ) : os::Window( cFrame, "info_window", "Info", os::WND_NOT_RESIZABLE )
	{
		m_zFile = zFile;
		SetTitle( zFile );
		os::Image* pcImage = NULL;
		os::String zType = "Unknown";
		os::String zMimeType = "application/unknown";
		struct stat sStat;
		char zSize[255];
		char zDate[255];
		bool bBlockTypeSelection = false;
		os::RegistrarManager* pcManager = NULL;
		m_cMessenger = cViewTarget;
		m_pcChangeMsg = pcChangeMsg;
		
		
		/* Get stat */
		lstat( zFile.c_str(), &sStat );
		
		/* Calculate size string */
		if( S_ISDIR( sStat.st_mode ) || S_ISLNK( sStat.st_mode ) )
		{
			strcpy( zSize, "None" );
			bBlockTypeSelection = true;
		} else {
			if( sStat.st_size >= 1000000 )
				sprintf( zSize, "%i Mb", ( int )( sStat.st_size / 1000000 + 1 ) );
			else
				sprintf( zSize, "%i Kb", ( int )( sStat.st_size / 1000 + 1 ) );
		}
		
		
		/* Get the filetype and icon first */
		try
		{
			pcManager = os::RegistrarManager::Get();
			os::Message cTypeMessage;
			pcManager->GetTypeAndIcon( zFile, os::Point( 48, 48 ), &zType, &pcImage, &cTypeMessage );
			cTypeMessage.FindString( "mimetype", &zMimeType );
		} catch( ... )
		{
		}
		
		/* Calculate */
		struct tm *psTime = localtime( &sStat.st_mtime );
		strftime( zDate, sizeof( zDate ), "%c", psTime );
		
		
		
		/* Create main view */
		m_pcView = new os::LayoutView( GetBounds(), "main_view" );
	
		os::VLayoutNode* pcVRoot = new os::VLayoutNode( "v_root" );
		pcVRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
		
	
		os::HLayoutNode* pcHNode = new os::HLayoutNode( "h_node", 0.0f );
		
		/* Icon */
		m_pcImageView = new os::ImageView( os::Rect(), "icon", pcImage );
		
		pcHNode->AddChild( m_pcImageView, 0.0f );
		pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		
		os::VLayoutNode* pcVInfo = new os::VLayoutNode( "v_info" );
		
		/* Name */
		os::HLayoutNode* pcHFileName = new os::HLayoutNode( "h_filename" );
		
		m_pcFileNameLabel = new os::StringView( os::Rect(), "filename_label", "Name:" );	
		m_pcFileName = new os::StringView( os::Rect(), "filename", os::Path( zFile ).GetLeaf() );	
	
		pcHFileName->AddChild( m_pcFileNameLabel, 0.0f );
		pcHFileName->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHFileName->AddChild( m_pcFileName );
		
		
		/* Path */
		os::HLayoutNode* pcHPath = new os::HLayoutNode( "h_path" );
		
		m_pcPathLabel = new os::StringView( os::Rect(), "path_label", "Path:" );	
		m_pcPath = new os::StringView( os::Rect(), "path", os::Path( zFile ).GetDir() );	
	
		pcHPath->AddChild( m_pcPathLabel, 0.0f );
		pcHPath->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHPath->AddChild( m_pcPath );
		
		/* Size */
		os::HLayoutNode* pcHSize = new os::HLayoutNode( "h_size" );
		
		m_pcSizeLabel = new os::StringView( os::Rect(), "size_label", "Size:" );	
		m_pcSize = new os::StringView( os::Rect(), "size", zSize );	
	
		pcHSize->AddChild( m_pcSizeLabel, 0.0f );
		pcHSize->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHSize->AddChild( m_pcSize );
		
		/* Data */
		os::HLayoutNode* pcHDate = new os::HLayoutNode( "h_date" );
		
		m_pcDateLabel = new os::StringView( os::Rect(), "date_label", "Modified:" );	
		m_pcDate = new os::StringView( os::Rect(), "date", zDate );	
	
		pcHDate->AddChild( m_pcDateLabel, 0.0f );
		pcHDate->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHDate->AddChild( m_pcDate );
		
		/* Type */
		
		os::HLayoutNode* pcHType = new os::HLayoutNode( "h_type" );
		
		m_pcTypeLabel = new os::StringView( os::Rect(), "type_label", "Type:" );	
		m_pcTypeSelector = new os::DropdownMenu( os::Rect(), "type" );	
		m_pcTypeSelector->SetEnable( !bBlockTypeSelection );
		
		pcHType->AddChild( m_pcTypeLabel, 0.0f );
		pcHType->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		pcHType->AddChild( m_pcTypeSelector );
		
		pcHType->AddChild( new os::HLayoutSpacer( "" ) );
		
		
		
		pcVInfo->AddChild( pcHFileName );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVInfo->AddChild( pcHPath );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVInfo->AddChild( pcHSize );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVInfo->AddChild( pcHDate );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		pcVInfo->AddChild( pcHType );
		pcVInfo->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
		
		pcVInfo->SameWidth( "filename_label", "path_label", "size_label", "date_label", "type_label", NULL );
		
		pcHNode->AddChild( pcVInfo );
		
		
		
		/* create buttons */
		os::HLayoutNode* pcHButtons = new os::HLayoutNode( "h_buttons", 0.0f );
		
		m_pcOk = new os::Button( os::Rect(), "ok", 
						"Ok", new os::Message( 0 ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
		pcHButtons->AddChild( new os::HLayoutSpacer( "" ) );
		pcHButtons->AddChild( m_pcOk, 0.0f );
	
		pcVRoot->AddChild( pcHNode );
		pcVRoot->AddChild( new os::VLayoutSpacer( "" ) );
		pcVRoot->AddChild( pcHButtons );
		
		
		/* Populate dropdown menu */
		m_pcTypeSelector->AppendItem( os::String( "Current: " ) + zType );
		m_pcTypeSelector->AppendItem( "Executable" );
		if( !bBlockTypeSelection && pcManager )
		{
			for( int32 i = 0; i < pcManager->GetTypeCount(); i++ )
			{
				os::Message cMsg = pcManager->GetType( i );
				os::String zRType;
				if( cMsg.FindString( "identifier", &zRType ) == 0 )
					m_pcTypeSelector->AppendItem( zRType );
			}
		}
		
		
		m_pcTypeSelector->SetSelection( 0, false );
		
		
		m_pcView->SetRoot( pcVRoot );
	
		AddChild( m_pcView );
		
		m_pcOk->SetTabOrder( 0 );
		
		SetDefaultButton( m_pcOk );
		
		m_pcOk->MakeFocus();
		
		ResizeTo( m_pcView->GetPreferredSize( false ) );
		
		if( pcManager )
			pcManager->Put(); 
		
	}

	~InfoWin() { 
		os::Image* pcImage = m_pcImageView->GetImage();
		m_pcImageView->SetImage( NULL );
		delete( pcImage );
	}
	
	
	void HandleMessage( os::Message* pcMessage )
	{
		
		switch( pcMessage->GetCode() )
		{
			
			case 0:
			{
				int nSelection = m_pcTypeSelector->GetSelection();
				if( nSelection == 1 )
				{
					/* Executable */
					char zString[255] = "application/x-executable";
					int nFd = open( m_zFile.c_str(), O_RDWR );
					if( nFd >= 0 )
					{
						write_attr( nFd, "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, zString,
							0, strlen( zString ) );
						close( nFd );
					}
				}
				else if( nSelection > 1 )
				{
					/* Get mimetype */
					try
					{
						os::RegistrarManager* pcManager = os::RegistrarManager::Get();
						os::Message cMsg = pcManager->GetType( nSelection - 2 );
						os::String zMimeType;
						if( cMsg.FindString( "mimetype", &zMimeType ) == 0 )
						{
							/* Write it */
							int nFd = open( m_zFile.c_str(), O_RDWR );
							if( nFd >= 0 )
							{
								write_attr( nFd, "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, zMimeType.c_str(),
								0, zMimeType.Length() );
								close( nFd );
							}
						}
						pcManager->Put();
					} catch( ... )
					{
					}
				}
				
				if( nSelection > 0 && m_pcChangeMsg )
					m_cMessenger.SendMessage( m_pcChangeMsg );
				
				PostMessage( os::M_QUIT );
				break;
			}
			default:
				break;
		}
		
		os::Window::HandleMessage( pcMessage );
	}
	
private:
	os::Messenger		m_cMessenger;
	os::Message*		m_pcChangeMsg;
	os::String			m_zFile;
	os::LayoutView*		m_pcView;
	os::ImageView*		m_pcImageView;
	os::StringView*		m_pcFileNameLabel;
	os::StringView*		m_pcFileName;
	os::StringView*		m_pcPathLabel;
	os::StringView*		m_pcPath;
	os::StringView*		m_pcSizeLabel;
	os::StringView*		m_pcSize;
	os::StringView*		m_pcDateLabel;
	os::StringView*		m_pcDate;
	os::StringView*		m_pcTypeLabel;
	os::DropdownMenu*	m_pcTypeSelector;
	os::Button*			m_pcOk;
};


/** Create a dialog which shows information about a file or directory.
 * \par Description:
 *	This dialog will show information about a file like its name, size
 *  and type. It is also possible to change the file type. If any 
 *  changes have been made, then a message will be sent to the supplied 
 *  message target.
 * \par Note:
 *	Don’t forget to place the window at the right position and show it.
 * \param cMsgTarget - The target that will receive the message.
 * \param pcMsg - The message that will be sent.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

Window* FileReference::InfoDialog( const Messenger & cMsgTarget, Message* pcChangeMsg )
{
	os::String cPath;
	GetPath( &cPath );
	InfoWin* pcWin = new InfoWin( os::Rect( 50, 50, 350, 300 ), cPath, cMsgTarget, pcChangeMsg );
	
	return( pcWin );
}



