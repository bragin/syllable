
/*  Syllable Media Converter
 *  Copyright (C) 2003 Arno Klenke
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

#include "MediaConverter.h"
#include <iostream.h>

/* TODO:
 * - Track selection
 * - Stream selection
 * - Respect possible number of streams in the media output
 * - Do not reencode data if not necessary
 */

/* MCWindow class */
MCWindow::MCWindow( const os::Rect & cFrame, const std::string & cName, const std::string & cTitle, uint32 nFlags ):os::Window( cFrame, cName, cTitle, nFlags )
{
	m_bEncode = false;


	/* Create output selection list */
	m_pcOutputLabel = new os::StringView( os::Rect( 0, 0, 1, 1 ), "output_label", "Output" );
	m_pcOutputLabel->SetFrame( os::Rect( 5, 10, m_pcOutputLabel->GetPreferredSize( false ).x + 5, m_pcOutputLabel->GetPreferredSize( false ).y + 10 ) );
	m_pcOutputList = new os::DropdownMenu( os::Rect( 0, 0, 1, 1 ), "output_list" );
	m_pcOutputList->SetFrame( os::Rect( m_pcOutputLabel->GetPreferredSize( false ).x + 10, 5, 250, m_pcOutputList->GetPreferredSize( false ).y + 5 ) );
	m_pcOutputList->SetSelectionMessage( new os::Message( MC_GUI_OUTPUT_CHANGE ) );
	m_pcOutputList->SetTarget( this );
	/* Create video list */
	m_pcVideoLabel = new os::StringView( os::Rect( 0, 0, 1, 1 ), "video_label", "Video" );
	m_pcVideoLabel->SetFrame( os::Rect( 5, 40, m_pcVideoLabel->GetPreferredSize( false ).x + 5, m_pcVideoLabel->GetPreferredSize( false ).y + 40 ) );
	m_pcVideoList = new os::DropdownMenu( os::Rect( 0, 0, 1, 1 ), "video_list" );
	m_pcVideoList->SetFrame( os::Rect( m_pcOutputLabel->GetPreferredSize( false ).x + 10, 35, 250, m_pcVideoList->GetPreferredSize( false ).y + 5 + 30 ) );
	m_pcVideoList->SetSelectionMessage( new os::Message( MC_GUI_VIDEO_CHANGE ) );
	m_pcVideoList->SetTarget( this );
	m_pcVideoBitRateLabel = new os::StringView( os::Rect( 0, 0, 1, 1 ), "video_bitrate_label", "kbit/s" );
	m_pcVideoBitRateLabel->SetFrame( os::Rect( 310, 40, m_pcVideoBitRateLabel->GetPreferredSize( false ).x + 310, m_pcVideoBitRateLabel->GetPreferredSize( false ).y + 40 ) );
	m_pcVideoBitRate = new os::Spinner( os::Rect( 0, 0, 1, 1 ), "video_bitrate", 32, new os::Message( MC_GUI_VIDEO_CHANGE ) );
	m_pcVideoBitRate->SetFrame( os::Rect( 255, 35, 305, m_pcVideoBitRate->GetPreferredSize( false ).y + 35 ) );
	m_pcVideoBitRate->SetMinMax( 32, 8000 );
	m_pcVideoBitRate->SetFormat( "%.0f" );
	m_pcVideoBitRate->SetStep( 16 );

	/* Create audio list */
	m_pcAudioLabel = new os::StringView( os::Rect( 0, 0, 1, 1 ), "audio_label", "Audio" );
	m_pcAudioLabel->SetFrame( os::Rect( 5, 70, m_pcAudioLabel->GetPreferredSize( false ).x + 5, m_pcAudioLabel->GetPreferredSize( false ).y + 70 ) );
	m_pcAudioList = new os::DropdownMenu( os::Rect( 0, 0, 1, 1 ), "audio_list" );
	m_pcAudioList->SetFrame( os::Rect( m_pcOutputLabel->GetPreferredSize( false ).x + 10, 65, 250, m_pcAudioList->GetPreferredSize( false ).y + 5 + 60 ) );
	m_pcAudioList->SetSelectionMessage( new os::Message( MC_GUI_AUDIO_CHANGE ) );
	m_pcAudioList->SetTarget( this );
	m_pcAudioBitRateLabel = new os::StringView( os::Rect( 0, 0, 1, 1 ), "audio_bitrate_label", "kbit/s" );
	m_pcAudioBitRateLabel->SetFrame( os::Rect( 310, 70, m_pcAudioBitRateLabel->GetPreferredSize( false ).x + 310, m_pcAudioBitRateLabel->GetPreferredSize( false ).y + 70 ) );
	m_pcAudioBitRate = new os::Spinner( os::Rect( 0, 0, 1, 1 ), "audio_bitrate", 32, new os::Message( MC_GUI_AUDIO_CHANGE ) );
	m_pcAudioBitRate->SetFrame( os::Rect( 255, 65, 305, m_pcAudioBitRate->GetPreferredSize( false ).y + 65 ) );
	m_pcAudioBitRate->SetMinMax( 32, 8000 );
	m_pcAudioBitRate->SetFormat( "%.0f" );
	m_pcAudioBitRate->SetStep( 16 );

	/* Create output file text field */
	m_pcFileLabel = new os::StringView( os::Rect( 0, 0, 1, 1 ), "file_label", "File" );
	m_pcFileLabel->SetFrame( os::Rect( 5, 100, m_pcFileLabel->GetPreferredSize( false ).x + 5, m_pcFileLabel->GetPreferredSize( false ).y + 100 ) );

	os::String cPath;
	const char *pzHome = getenv( "HOME" );

	if ( pzHome != NULL )
	{
		cPath = pzHome;
		if ( cPath[cPath.size() - 1] != '/' )
		{
			cPath += os::String ( "/" );
		}
	}
	else
	{
		cPath = "/tmp/";
	}
	cPath += os::String ( "output.avi" );

	m_pcFileInput = new os::TextView( os::Rect( 0, 0, 1, 1 ), "file_field", cPath.c_str() );
	m_pcFileInput->SetFrame( os::Rect( m_pcOutputLabel->GetPreferredSize( false ).x + 10, 95, 305, 95 + m_pcFileInput->GetPreferredSize( false ).y ) );
	m_pcFileInput->SetMultiLine( false );


	/* Create progressbar */
	m_pcProgress = new os::ProgressBar( os::Rect( 0, 0, 1, 1 ), "progress" );
	m_pcProgress->SetFrame( os::Rect( 10, 125, 280, 125 + m_pcProgress->GetPreferredSize( false ).y ) );

	/* Create button */
	m_pcStartButton = new os::Button( os::Rect( 0, 0, 1, 1 ), "start", "Start", new os::Message( MC_GUI_START ) );
	m_pcStartButton->SetFrame( os::Rect( 285, 123, 340, 123 + m_pcStartButton->GetPreferredSize( false ).y ) );

	/* Fill output list */
	os::MediaManager * pcManager = os::MediaManager::GetInstance();
	os::MediaOutput * pcOutput;
	uint32 nIndex = 0;

	while ( ( pcOutput = pcManager->GetOutput( nIndex ) ) != NULL )
	{
		m_pcOutputList->AppendItem( pcOutput->GetIdentifier().c_str(  ) );
		delete( pcOutput );
		nIndex++;
	}
	m_pcOutputList->SetSelection( 0, false );


	AddChild( m_pcOutputLabel );
	AddChild( m_pcOutputList );
	AddChild( m_pcVideoList );
	AddChild( m_pcVideoLabel );
	AddChild( m_pcVideoBitRate );
	AddChild( m_pcVideoBitRateLabel );
	AddChild( m_pcAudioList );
	AddChild( m_pcAudioLabel );
	AddChild( m_pcAudioBitRate );
	AddChild( m_pcAudioBitRateLabel );
	AddChild( m_pcFileInput );
	AddChild( m_pcFileLabel );
	AddChild( m_pcProgress );
	AddChild( m_pcStartButton );
}

MCWindow::~MCWindow()
{
	RemoveChild( m_pcOutputLabel );
	RemoveChild( m_pcOutputList );
	RemoveChild( m_pcVideoLabel );
	RemoveChild( m_pcVideoList );
	RemoveChild( m_pcVideoBitRate );
	RemoveChild( m_pcVideoBitRateLabel );
	RemoveChild( m_pcAudioLabel );
	RemoveChild( m_pcAudioList );
	RemoveChild( m_pcAudioBitRate );
	RemoveChild( m_pcAudioBitRateLabel );
	RemoveChild( m_pcFileInput );
	RemoveChild( m_pcFileLabel );
	RemoveChild( m_pcProgress );
	RemoveChild( m_pcStartButton );
	delete( m_pcOutputLabel );
	delete( m_pcOutputList );
	delete( m_pcVideoLabel );
	delete( m_pcVideoList );
	delete( m_pcVideoBitRate );
	delete( m_pcVideoBitRateLabel );
	delete( m_pcAudioLabel );
	delete( m_pcAudioList );
	delete( m_pcAudioBitRate );
	delete( m_pcAudioBitRateLabel );
	delete( m_pcFileInput );
	delete( m_pcFileLabel );
	delete( m_pcProgress );
	delete( m_pcStartButton );
}


void MCWindow::FillLists()
{
	/* Fill video and audio selection lists */
	m_pcVideoList->Clear();
	m_pcAudioList->Clear();
	os::MediaManager * pcManager = os::MediaManager::GetInstance();
	os::MediaOutput * pcOutput;
	pcOutput = pcManager->GetOutput( m_pcOutputList->GetSelection() );

	m_pcVideoList->AppendItem( "None" );
	m_pcAudioList->AppendItem( "None" );
	
	if( !pcOutput->FileNameRequired() ) {
		delete( pcOutput );
		return;
	}

	if ( pcOutput != NULL )
	{
		bool bVidSelect = false;
		bool bAudSelect = false;

		for ( uint32 i = 0; i < pcOutput->GetOutputFormatCount(); i++ )
		{
			os::MediaFormat_s sFormat = pcOutput->GetOutputFormat( i );
			if ( sFormat.nType == os::MEDIA_TYPE_VIDEO )
			{
				m_pcVideoList->AppendItem( sFormat.zName.c_str() );
				if ( m_bVideo && m_sVideoFormat.zName == sFormat.zName )
				{
					m_pcVideoList->SetSelection( m_pcVideoList->GetItemCount() - 1 );
					bVidSelect = true;
				}
			}
			if ( sFormat.nType == os::MEDIA_TYPE_AUDIO )
			{
				m_pcAudioList->AppendItem( sFormat.zName.c_str() );
				if ( m_bAudio && m_sAudioFormat.zName == sFormat.zName )
				{
					m_pcAudioList->SetSelection( m_pcAudioList->GetItemCount() - 1 );
					bAudSelect = true;
				}
			}
		}
		if ( m_bVideo )
			m_pcVideoBitRate->SetValue( m_sVideoFormat.nBitRate / 1000 );
		if ( m_bAudio )
			m_pcAudioBitRate->SetValue( m_sAudioFormat.nBitRate / 1000 );
		if ( !bVidSelect )
			m_pcVideoList->SetSelection( 0, false );
		if ( !bAudSelect )
			m_pcAudioList->SetSelection( 0, false );
		m_pcVideoList->SetEnable( m_bVideo );
		m_pcVideoBitRate->SetEnable( m_bVideo );
		m_pcAudioList->SetEnable( m_bAudio );
		m_pcAudioBitRate->SetEnable( m_bAudio );
		delete( pcOutput );
	}

}
void MCWindow::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case MC_GUI_OUTPUT_CHANGE:
		FillLists();
		break;
	case MC_GUI_START:
		{
			/* Startbutton has been pressed */
			if ( m_bEncode )
			{
				/* ... but we are already encoding */
				os::Application::GetInstance()->PostMessage( new os::Message( MC_GUI_START ), os::Application::GetInstance(  ) );
				break;
			}
			os::Message * pcNewMessage = new os::Message( MC_GUI_START );
			os::MediaManager * pcManager = os::MediaManager::GetInstance();
			os::MediaOutput * pcOutput;
			pcOutput = pcManager->GetOutput( m_pcOutputList->GetSelection() );
			/* Add number of the output, audio and video object to the message */

			if ( pcOutput != NULL )
			{
				pcNewMessage->AddInt32( "output", m_pcOutputList->GetSelection() );
				int32 nVidCount = 0;
				int32 nAudCount = 0;

				for ( uint32 i = 0; i < pcOutput->GetOutputFormatCount(); i++ )
				{
					os::MediaFormat_s sFormat = pcOutput->GetOutputFormat( i );
					if ( sFormat.nType == os::MEDIA_TYPE_VIDEO )
					{
						if ( m_bVideo && nVidCount == m_pcVideoList->GetSelection() - 1 )
						{
							pcNewMessage->AddInt32( "video", i );
						}
						nVidCount++;
					}
					if ( sFormat.nType == os::MEDIA_TYPE_AUDIO )
					{
						if ( m_bAudio && nAudCount == m_pcAudioList->GetSelection() - 1 )
						{
							pcNewMessage->AddInt32( "audio", i );
						}
						nAudCount++;
					}
				}
				delete( pcOutput );
				if ( !m_bVideo || m_pcVideoList->GetSelection() == 0 )
					pcNewMessage->AddInt32( "video", -1 );
				if ( !m_bAudio || m_pcAudioList->GetSelection() == 0 )
					pcNewMessage->AddInt32( "audio", -1 );
				pcNewMessage->AddInt32( "video_bitrate", m_pcVideoBitRate->GetValue().AsInt32(  ) );
				pcNewMessage->AddInt32( "audio_bitrate", m_pcAudioBitRate->GetValue().AsInt32(  ) );
				os::String zFile = m_pcFileInput->GetBuffer()[0];

				pcNewMessage->AddString( "file", zFile.str() );
				os::Application::GetInstance()->PostMessage( pcNewMessage, os::Application::GetInstance(  ) );
			}
		}
		break;
	case MC_START:
		{
			/* Encoding has been started ( sent by the Encode() method ) */
			m_pcStartButton->SetLabel( "Stop" );
			m_bEncode = true;
		}
		break;
	case MC_STOP:
		{
			/* Encoding has been started ( sent by the Encode() method ) */
			m_pcStartButton->SetLabel( "Start" );
			m_bEncode = false;
		}
		break;
	default:
		os::Window::HandleMessage( pcMessage );
		break;
	}
}


bool MCWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return ( false );
}

/* MCApp class */
MCApp::MCApp( const char *pzMimeType, os::String zFileName, bool bLoad ):os::Application( pzMimeType )
{
	m_bEncode = false;

	/* Create media manager */
	m_pcManager = new os::MediaManager();

	if ( !m_pcManager->IsValid() )
	{
		cout << "Media server is not running" << endl;
		PostMessage( os::M_QUIT );
		return;
	}

	/* Open file if neccessary */
	if ( bLoad )
	{
		os::MediaInput * pcInput = m_pcManager->GetBestInput( zFileName );
		if ( pcInput != NULL )
		{
			Open( zFileName, pcInput->GetIdentifier() );
			delete( pcInput );
		}
		else
		{
			cout << "This file / device is not supported!" << endl;
			PostMessage( os::M_QUIT );
		}
	}
	else
	{

		/* Create media input selector */
		m_pcInputSelector = new os::MediaInputSelector( os::Point( 100, 100 ), "Select input file - Media Converter", new os::Messenger( this ), new os::Message( MC_IS_OPEN ), new os::Message( MC_IS_CANCEL ) );

		m_pcInputSelector->Show();
	}
}

MCApp::~MCApp()
{
	/* Close and delete everything */
	delete( m_pcManager );
}

void MCApp::Open( os::String zFile, os::String zInput )
{
	/* Look for the input */
	uint32 i = 0;

	while ( ( m_pcInput = m_pcManager->GetInput( i ) ) != NULL )
	{
		if ( m_pcInput->GetIdentifier() == zInput )
		{
			break;
		}
		delete( m_pcInput );
		m_pcInput = NULL;
		i++;
	}
	if ( m_pcInput == NULL )
	{
		/* This should not happen! */
		PostMessage( os::M_QUIT );
	}
	/* Open input */
	if ( m_pcInput->Open( zFile ) != 0 )
	{
		/* This should not happen! */
		PostMessage( os::M_QUIT );
	}

	/* Check if we support this */
	if ( m_pcInput->StreamBased() || !m_pcInput->PacketBased(  ) )
	{
		os::Alert * pcAlert = new os::Alert( "Error", "This file / device is not supported by the Media Converter.", os::Alert::ALERT_WARNING, 0, "Ok", NULL );
		pcAlert->Go();
		return;
	}
	/* TODO: */
	if ( m_pcInput->GetTrackCount() > 1 )
	{
		os::Alert * pcAlert = new os::Alert( "Error", "Files / devices with more than one track are not yet supported. Sorry!", os::Alert::ALERT_WARNING, 0, "Ok", NULL );
		pcAlert->Go();
		return;
	}

	/* REMOVE!!! */

	m_pcInput->SelectTrack( 0 );
	cout << "Length: " << m_pcInput->GetLength() << endl;
	for ( uint32 j = 0; j < m_pcInput->GetStreamCount(); j++ )
	{
		if ( m_pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_VIDEO )
		{
			cout << "Stream " << j << " " << m_pcInput->GetStreamFormat( j ).zName.c_str() << endl;
			cout << "BitRate: " << m_pcInput->GetStreamFormat( j ).nBitRate << endl;
			cout << "Size: " << m_pcInput->GetStreamFormat( j ).nWidth << "x" << m_pcInput->GetStreamFormat( j ).nHeight << endl;
			cout << "Framerate: " << ( int )m_pcInput->GetStreamFormat( j ).vFrameRate << endl;
		}
		if ( m_pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
		{
			cout << "Stream " << j << " " << m_pcInput->GetStreamFormat( j ).zName.c_str() << endl;
			cout << "BitRate: " << m_pcInput->GetStreamFormat( j ).nBitRate << endl;
			cout << "Channels: " << m_pcInput->GetStreamFormat( j ).nChannels << endl;
		}
	}
	/* Select right streams */
	m_bVideo = m_bAudio = false;
	for ( i = 0; i < m_pcInput->GetStreamCount(); i++ )
	{
		if ( m_pcInput->GetStreamFormat( i ).nType == os::MEDIA_TYPE_VIDEO && !m_bVideo )
		{
			m_nVideoStream = i;
			m_sVideoFormat = m_pcInput->GetStreamFormat( i );
			m_bVideo = true;
		}
		if ( m_pcInput->GetStreamFormat( i ).nType == os::MEDIA_TYPE_AUDIO && !m_bAudio )
		{
			m_nAudioStream = i;
			m_sAudioFormat = m_pcInput->GetStreamFormat( i );
			m_bAudio = true;
		}
	}
	/* Create main window */
	m_pcWin = new MCWindow( os::Rect( 100, 100, 450, 250 ), "mc_window", "Select destination - Media Converter", os::WND_NO_ZOOM_BUT | os::WND_NO_DEPTH_BUT | os::WND_NOT_RESIZABLE );
	m_pcWin->Show();
	m_pcWin->MakeFocus( true );
	m_pcWin->SetVideoAudio( m_sVideoFormat, m_sAudioFormat, m_bVideo, m_bAudio );
}

void MCApp::Encode()
{
	m_bEncode = true;
	os::MediaPacket_s sPacket;
	os::MediaPacket_s sAIPacket;
	os::MediaPacket_s sAOPacket;
	os::MediaPacket_s sVIPacket;
	os::MediaPacket_s sVOPacket;

	/* Tell window that we are encoding */
	m_pcWin->PostMessage( MC_START, m_pcWin );


	/* We are using this pipeline:
	 * input -> codec -> raw -> codec -> output
	 * TODO: Support for :
	 * input -> codec -> output
	 */

	/* Look for the right codecs */
	os::MediaFormat_s sFormat;

	/* Open output */
	if ( m_pcOutput->Open( m_zFile ) < 0 )
	{
		os::Alert * pcAlert = new os::Alert( "Error", "Could not open output", os::Alert::ALERT_WARNING, 0, "Ok", NULL );
		pcAlert->Go();
		PostMessage( MC_GUI_START );
		while ( m_bEncode )
		{
			snooze( 1000 );
		}
		return;
	}


	if ( m_bVideo )
	{
		/* Open video input codec */
		memset( &sFormat, 0, sizeof( sFormat ) );
		sFormat.zName = "Raw Video";
		sFormat.nType = os::MEDIA_TYPE_VIDEO;
		m_pcInputVideo = m_pcManager->GetBestCodec( m_sVideoFormat, sFormat, false );
		if ( m_pcInputVideo )
		{
			m_pcInputVideo->Open( m_sVideoFormat, sFormat, false );
			sFormat = m_pcInputVideo->GetExternalFormat();
		}
		/* Open video output codec */
		os::MediaFormat_s sOutput;
		memset( &sOutput, 0, sizeof( sOutput ) );
		sOutput.zName = m_pcOutput->GetOutputFormat( m_nVideoOutputFormat ).zName;
		sOutput.nType = os::MEDIA_TYPE_VIDEO;
		sOutput.nBitRate = m_nVideoBitRate * 1000;

		m_pcOutputVideo = m_pcManager->GetBestCodec( sOutput, sFormat, true );
		if ( m_pcOutputVideo )
		{
			m_pcOutputVideo->Open( sOutput, sFormat, true );
		}


		if ( m_pcInputVideo == NULL || m_pcOutputVideo == NULL || m_pcOutput->AddStream( m_zFile, m_pcOutputVideo->GetInternalFormat() ) < 0 )
		{
			os::Alert * pcAlert = new os::Alert( "Error", "Could not find a matching video codec", os::Alert::ALERT_WARNING, 0, "Ok", NULL );
			pcAlert->Go();
			PostMessage( MC_GUI_START );
			while ( m_bEncode )
			{
				snooze( 1000 );
			}
			return;
		}

		cout << "Using Input Video Codec " << m_pcInputVideo->GetIdentifier().c_str(  ) << endl;
		cout << "Using Output Video Codec " << m_pcOutputVideo->GetIdentifier().c_str(  ) << endl;
		m_pcInputVideo->CreateVideoOutputPacket( &sVIPacket );
		m_pcOutputVideo->CreateVideoOutputPacket( &sVOPacket );
	}


	if ( m_bAudio )
	{
		/* Open audio input codec */
		memset( &sFormat, 0, sizeof( sFormat ) );
		sFormat.zName = "Raw Audio";
		sFormat.nType = os::MEDIA_TYPE_AUDIO;
		m_pcInputAudio = m_pcManager->GetBestCodec( m_sAudioFormat, sFormat, false );
		if ( m_pcInputAudio )
		{
			m_pcInputAudio->Open( m_sAudioFormat, sFormat, false );
			sFormat = m_pcInputAudio->GetExternalFormat();
		}

		/* Open audio output codec */
		os::MediaFormat_s sOutput;
		memset( &sOutput, 0, sizeof( sOutput ) );
		sOutput.zName = m_pcOutput->GetOutputFormat( m_nAudioOutputFormat ).zName;
		sOutput.nType = os::MEDIA_TYPE_AUDIO;
		sOutput.nBitRate = m_nAudioBitRate * 1000;

		m_pcOutputAudio = m_pcManager->GetBestCodec( sOutput, sFormat, true );
		if ( m_pcOutputAudio )
			m_pcOutputAudio->Open( sOutput, sFormat, true );

		if ( m_pcInputAudio == NULL || m_pcOutputAudio == NULL || m_pcOutput->AddStream( m_zFile, m_pcOutputAudio->GetInternalFormat() ) < 0 )
		{
			os::Alert * pcAlert = new os::Alert( "Error", "Could not find a matching audio codec", os::Alert::ALERT_WARNING, 0, "Ok", NULL );
			pcAlert->Go();
			PostMessage( MC_GUI_START );
			while ( m_bEncode )
			{
				snooze( 1000 );
			}
			return;
		}
		cout << "Using Input Audio Codec " << m_pcInputAudio->GetIdentifier().c_str(  ) << endl;
		cout << "Using Output Audio Codec " << m_pcOutputAudio->GetIdentifier().c_str(  ) << endl;
		/* Create output packets */
		m_pcInputAudio->CreateAudioOutputPacket( &sAIPacket );
		m_pcOutputAudio->CreateAudioOutputPacket( &sAOPacket );

	}


	uint32 nOldValue = 0;
	uint32 nAOStream = 0;
	uint32 nVOStream = 0;

	if ( m_bVideo )
		nAOStream = 1;
	/* Now decode and then encode everything */
	while ( m_bEncode && m_pcInput->ReadPacket( &sPacket ) == 0 )
	{
		if ( m_bAudio && sPacket.nStream == m_nAudioStream )
		{
			if ( m_pcInputAudio->DecodePacket( &sPacket, &sAIPacket ) == 0 )
			{
				if ( sAIPacket.nSize[0] > 0 )
				{
					if ( m_pcOutputAudio->EncodePacket( &sAIPacket, &sAOPacket ) == 0 )
					{
						m_pcOutput->WritePacket( nAOStream, &sAOPacket );
					}
				}
			}
			else
			{
				cout << "Error decoding" << endl;
			}
		}
		if ( m_bVideo && sPacket.nStream == m_nVideoStream )
		{
			if ( m_pcInputVideo->DecodePacket( &sPacket, &sVIPacket ) == 0 )
			{
				if ( sVIPacket.nSize[0] > 0 )
				{
					if ( m_pcOutputVideo->EncodePacket( &sVIPacket, &sVOPacket ) == 0 )
					{
						m_pcOutput->WritePacket( nVOStream, &sVOPacket );
					}
				}
			}
			else
			{
				cout << "Error decoding" << endl;
			}
		}
		m_pcInput->FreePacket( &sPacket );
		m_pcOutput->Flush();
		/* Update progressbar */
		if ( m_pcInput->GetCurrentPosition() * 100 / m_pcInput->GetLength(  ) != nOldValue )
		{
			nOldValue = m_pcInput->GetCurrentPosition() * 100 / m_pcInput->GetLength(  );
			m_pcWin->GetProgress()->SetProgress( ( float )nOldValue / 100 );
		}

		//snooze( 100 );
	}
	/* Close everything */
	m_pcOutput->Close();
	delete( m_pcOutput );
	if ( m_bAudio )
	{
		m_pcInputAudio->DeleteAudioOutputPacket( &sAIPacket );
		m_pcOutputAudio->DeleteAudioOutputPacket( &sAOPacket );
		m_pcInputAudio->Close();
		m_pcOutputAudio->Close();
		delete( m_pcInputAudio );
		delete( m_pcOutputAudio );
	}
	if ( m_bVideo )
	{
		m_pcInputVideo->DeleteVideoOutputPacket( &sVIPacket );
		m_pcOutputVideo->DeleteVideoOutputPacket( &sVOPacket );
		m_pcInputVideo->Close();
		m_pcOutputVideo->Close();
		delete( m_pcInputVideo );
		delete( m_pcOutputVideo );
	}
	m_pcInput->Seek( 0 );

	m_pcWin->GetProgress()->SetProgress( 0 );
	os::Alert * pcAlert = new os::Alert( "Media Converter", "Encoding finished!", os::Alert::ALERT_INFO, 0, "Ok", NULL );
	pcAlert->Go();
	PostMessage( MC_GUI_START );
	while ( m_bEncode )
	{
		snooze( 1000 );
	}
	cout << "End" << endl;
	return;

}

int encode_thread_entry( void *pData )
{
	/* Helper thread */
	MCApp *pcApp = ( MCApp * ) pData;

	pcApp->Encode();
	return ( 0 );
}

void MCApp::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case MC_IS_CANCEL:
		/* Close */
		PostMessage( os::M_QUIT );
		break;
	case MC_IS_OPEN:
		{
			/* Open */
			os::String zFile;
			os::String zInput;

			if ( pcMessage->FindString( "file/path", &zFile.str() ) == 0 && pcMessage->FindString( "input", &zInput.str(  ) ) == 0 )
			{
				Open( zFile, zInput );
			}
			m_pcInputSelector = NULL;

		}
		break;
	case MC_GUI_START:
		{
			int32 nOutput;

			if ( !m_bEncode && pcMessage->FindInt32( "output", &nOutput ) == 0 && pcMessage->FindInt32( "video", &m_nVideoOutputFormat ) == 0 && pcMessage->FindInt32( "audio", &m_nAudioOutputFormat ) == 0 && pcMessage->FindInt32( "video_bitrate", &m_nVideoBitRate ) == 0 && pcMessage->FindInt32( "audio_bitrate", &m_nAudioBitRate ) == 0 && pcMessage->FindString( "file", &m_zFile.str() ) == 0 )
			{
				/* Start encoding */
				m_pcOutput = m_pcManager->GetOutput( nOutput );
				if ( m_pcOutput != NULL )
				{
					cout << "Output: " << m_pcOutput->GetIdentifier().c_str(  ) << endl;
					m_bVideo = m_bAudio = false;
					if ( m_nVideoOutputFormat > -1 )
					{
						m_bVideo = true;
						cout << "Video: " << m_pcOutput->GetOutputFormat( m_nVideoOutputFormat ).zName.c_str() << endl;
					}
					if ( m_nAudioOutputFormat > -1 )
					{
						m_bAudio = true;
						cout << "Audio: " << m_pcOutput->GetOutputFormat( m_nAudioOutputFormat ).zName.c_str() << endl;
					}
					if ( m_bVideo || m_bAudio )
					{
						/* Start encoding thread */
						m_hEncodeThread = spawn_thread( "mc_encode", encode_thread_entry, 0, 0, this );
						resume_thread( m_hEncodeThread );
					}
				}
			}
			else if ( m_bEncode )
			{
				/* Stop encoding */
				m_pcWin->PostMessage( MC_STOP, m_pcWin );
				m_bEncode = false;
				//wait_for_thread( m_hEncodeThread );
			}
		}
		break;
	default:
		os::Application::HandleMessage( pcMessage );
		break;
	}
}


bool MCApp::OkToQuit()
{
	cout << "Quit" << endl;
	return ( true );
}

int main( int argc, char *argv[] )
{
	MCApp *pcApp = NULL;

	if ( argc > 1 )
	{
		pcApp = new MCApp( "application/x-vnd.MediaConverter", argv[1], true );
	}
	else
	{
		pcApp = new MCApp( "application/x-vnd.MediaConverter", "", false );
	}

	pcApp->Run();
	return ( 0 );
}



