#include "appwindow.h"
#include "messages.h"
#include "settingswindow.h"

AppWindow::AppWindow(ImageApp* App,const Rect& cFrame, const String& sFile) : Window(cFrame, "main_window", "AView 0.4")
{
    pcApp = App;
    sFileRequester = sFile;
    m_pcLoadRequester=NULL;
    g_pcHScrollBar = NULL;
    g_pcVScrollBar = NULL;

    AddItems();

	SetIcon(LoadImageFromResource("icon24x24.png")->LockBitmap());
}

void AppWindow::AddItems()
{
    float win_width, win_height;
    SetupMenus();


    Rect pcWinBounds=GetBounds();
    win_width=( pcWinBounds.right - pcWinBounds.left );
    win_height=( ( pcWinBounds.bottom - pcWinBounds.top ) - MENU_OFFSET );

    main_bitmap=new Bitmap(1600,1200, CS_RGB32, 0x0000);
    main_bitmap_view = new BitmapView ( main_bitmap,this );

    main_bitmap_view->MoveBy(0,MENU_OFFSET+1);  // Move the view/bitmap down so it does not obscure the menu

    Rect pcBitmapRect=main_bitmap_view->GetBounds();
    float winWidth=pcBitmapRect.left+pcBitmapRect.right;
    float winHeight=pcBitmapRect.top+pcBitmapRect.bottom;

    ResizeTo(winWidth,winHeight);
    AddChild( main_bitmap_view );

    main_bitmap_view->MakeFocus();
    SetupStatusBar();



    // We need to reset a few variables etc

    current_image=0;


}

void AppWindow::SetupMenus()
{
    cMenuBounds = GetBounds();
    cMenuBounds.bottom = MENU_OFFSET;

    pcMenuBar = new Menu(cMenuBounds, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE);

    // Create the menus within the bar
    Menu* pcAppMenu = new Menu(Rect(0,0,0,0),"_Application",ITEMS_IN_COLUMN);
    pcAppMenu->AddItem(new MenuItem("Settings...",new Message(ID_SETTINGS),"CTRL+S"));
    pcAppMenu->AddItem(new MenuSeparator());
    pcAppMenu->AddItem(new MenuItem("About...", new Message(ID_ABOUT), "CTRL+A"));
    pcAppMenu->AddItem(new MenuSeparator());
    pcAppMenu->AddItem(new MenuItem("Quit", new Message (ID_EXIT), "CTRL+Q"));

    Menu* pcFileMenu = new Menu(Rect(0,0,1,1),"_File", ITEMS_IN_COLUMN);
    MenuItem* pcItem = new MenuItem("Open...",new Message(ID_FILE_LOAD), "CTRL+O");
    pcFileMenu->AddItem(pcItem);

    Menu* pcViewMenu = new Menu(Rect(0,0,0,0),"_View",ITEMS_IN_COLUMN);
    pcSizeFitAll = new CheckMenu("Fit to screen",NULL,true);
    pcSizeFitWindow = new CheckMenu("Fit to window",NULL,false);
    pcViewMenu->AddItem(pcSizeFitAll);
    pcViewMenu->AddItem(pcSizeFitWindow);

    Menu * pcHelpMenu =  new Menu(Rect(0,0,0,0),"_Help",ITEMS_IN_COLUMN);
    pcHelpMenu->AddItem("Email...", new Message(ID_HELP));

    pcMenuBar->AddItem(pcAppMenu);
    pcMenuBar->AddItem(pcFileMenu);
    pcMenuBar->AddItem(pcViewMenu);
    pcMenuBar->AddItem(pcHelpMenu);

    pcMenuBar->SetTargetForItems(this);
    pcMenuBar->GetPreferredSize(false);
    // Add the menubar to the window

    AddChild(pcMenuBar);
}

void AppWindow::SetupStatusBar()
{
    pcStatusBar = new StatusBar(Rect(0,GetBounds().Height()-24,GetBounds().Width(), GetBounds().Height()-(-1)),"status_bar",3 );
    pcStatusBar->ConfigurePanel(0, StatusBar::CONSTANT,330);
    pcStatusBar->ConfigurePanel(1, StatusBar::FILL, 120);

    pcStatusBar->SetText("No files open!", 0,100);

    AddChild(pcStatusBar);
}

void AppWindow::DispatchMessage( Message* pcMessage, Handler* pcHandler)
{
	String cRawString;
	int32 nQual, nRawKey;

	if( pcMessage->GetCode() == os::M_KEY_DOWN )	//a key must have been pressed
	{
		if( pcMessage->FindString( "_raw_string", &cRawString ) != 0 || pcMessage->FindInt32( "_qualifiers", &nQual ) != 0 || pcMessage->FindInt32( "_raw_key", &nRawKey ) != 0 )
			Window::DispatchMessage( pcMessage, pcHandler );
		else
		{
			if( nQual & os::QUAL_CTRL )	//ctrl key was caught
			{
				pcMenuBar->MakeFocus();	//simple hack in order to get menus to work 
			}

			else if( nQual & os::QUAL_ALT )
			{
				if( nRawKey == 60 )	//ALT+A
				{
					Point cPoint = pcMenuBar->GetItemAt( 0 )->GetContentLocation();

					pcMenuBar->MouseDown( cPoint, 1 );
				}

				if( nRawKey == 63 )	//ALT+F
				{
					Point cPoint = pcMenuBar->GetItemAt( 1 )->GetContentLocation();

					pcMenuBar->MouseDown( cPoint, 1 );
				}

				if( nRawKey == 79 )	//ALT+V
				{
					Point cPoint = pcMenuBar->GetItemAt( 2 )->GetContentLocation();

					pcMenuBar->MouseDown( cPoint, 1 );
				}

				if( nRawKey == 65 )	//ALT+H
				{
					Point cPoint = pcMenuBar->GetItemAt( 6 )->GetContentLocation();

					pcMenuBar->MouseDown( cPoint, 1 );
				}
				else;
			}
		}
	}
    Window::DispatchMessage(pcMessage, pcHandler);
}

void AppWindow::HandleMessage(Message* pcMessage)
{

    switch(pcMessage->GetCode()) //Get the message code from the message
    {
    case ID_FILE_LOAD:
        {
            if (m_pcLoadRequester==NULL)
                m_pcLoadRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this),sFileRequester.c_str());
            m_pcLoadRequester->CenterInWindow(this);
            m_pcLoadRequester->Show();
            m_pcLoadRequester->MakeFocus();
        }

    case M_LOAD_REQUESTED:
        {
            if(pcMessage->FindString( "file/path", &pzFPath) == 0)
            {

                if( !stat ( pzFPath, &FPath_buf ) )
                {
                    if( FPath_buf.st_mode & S_IFDIR )
                    {
                        BuildDirList(pzFPath);
                        break;
                    }
                    else
                    {
                        Load( pzFPath );
                        break;
                    }
                }
            }

            break;
        }

    case M_KEY_DOWN:
        // This is the message passed back from BitmapView::Keydown.  We *know*
        // that the message has the data we need, so we'll get it & handle it from here.
        {
            pcMessage->FindInt( "_raw_key", &nKeyCode);
            HandleKeyEvent(nKeyCode);
            break;
        }

    case ID_EXIT:
        OkToQuit();
        break;

    case ID_SCROLL_VERTICAL:
        {
            main_bitmap_view->ScrollBack(g_pcHScrollBar->GetValue(), g_pcVScrollBar->GetValue()  );
        }
        break;

    case ID_SCROLL_HORIZONTAL:
        {
            main_bitmap_view->ScrollBack( g_pcHScrollBar->GetValue(), g_pcVScrollBar->GetValue() );
        }
        break;

    case ID_ABOUT:
        {
            Alert* pcAbout = new Alert("About AView","AView 0.4\n\nImage Viewer for Syllable    \nCopyright 2002 - 2003 Syllable Desktop Team\n\nAView is released under the GNU General\nPublic License. Please go to www.gnu.org\nmore information.\n",  Alert::ALERT_INFO, 0x00, "OK", NULL);
            pcAbout->CenterInWindow(this);
            pcAbout->Go(new Invoker());
            break;
        }

    case ID_HELP:
        {
            Alert* pcHelp = new Alert("Comments/Suggestions/Bugs/Suggestions","If you have any questions, comments, suggestions, or bugs,\nplease direct them towards the Syllable Developer List.\n\n ",  Alert::ALERT_INFO, 0x00, "OK", NULL);
            pcHelp->CenterInWindow(this);
            pcHelp->Go(new Invoker());
        }
        break;

    case ID_SETTINGS:
        {
            SettingsWindow* pcSettingsWindow = new SettingsWindow(pcApp,this);
            pcSettingsWindow->CenterInWindow(this);
            pcSettingsWindow->Show();
            pcSettingsWindow->MakeFocus();
        }
        break;

    case M_MESSAGE_PASSED:
        pcMessage->FindString("dirname",&sFileRequester);
        m_pcLoadRequester = NULL;
        break;

    default:
        Window::HandleMessage( pcMessage);
        break;
    }
}

bool AppWindow::OkToQuit( void )
{
    Application::GetInstance()->PostMessage( M_QUIT);
    return( false );
}


void AppWindow::Load(const char *cFileName)
// Load requested file
{
    if (g_pcVScrollBar)
    {
        RemoveChild(g_pcVScrollBar);
        g_pcVScrollBar = NULL;
    }

    if (g_pcHScrollBar)
    {
        RemoveChild(g_pcHScrollBar);
        g_pcHScrollBar = NULL;
    }

    if(strcmp(cFileName,""))  // This is a safeguard aginst use being passed an empty String.
        // Easier than finding the actual bug at the moment! ;)
    {

        if ( strstr(cFileName, ".jpg") || (strstr(cFileName,".png")) || (strstr(cFileName, ".gif"))  || (strstr(cFileName, ".icon")))
        {

            bSetTitle=true;
            Desktop d;
            // Get rid of the old bitmap
            RemoveChild(main_bitmap_view);
            Sync();
            // delete the bitmap to free up precious memory!
            delete main_bitmap;

            // load the new jpeg & create the BitmapView for it
            char img_fname[128];           //
            strcpy(img_fname,cFileName);   // This needs fixing! :)

            main_bitmap=LoadImageFromFile(img_fname)->LockBitmap();

            if(main_bitmap==NULL)
            {         // We wern't able to load the given file
                float win_width=300;
                float win_height=275-MENU_OFFSET;

                // We're going to make an empty bitmap to handle not being able to load the
                // given bitmap

                main_bitmap=new Bitmap( win_width, win_height, CS_RGB32, 0x0000);
                bSetTitle=false;  // We don't want this filename in the title now
            }

            main_bitmap_view = new BitmapView ( main_bitmap,this );

            main_bitmap_view->MoveBy(0,MENU_OFFSET+1);  // Move the view/bitmap down so it does not obscure the menu

            // We need to know the size of the new bitmap so we can match the window to it
            pcBitmapRect= main_bitmap_view->GetBounds();
            pcBitmapRect.bottom += (pcStatusBar->GetBounds().top+pcStatusBar->GetBounds().bottom+15);

            winWidth=pcBitmapRect.left+pcBitmapRect.right+15;
            winHeight=pcBitmapRect.top+pcBitmapRect.bottom+MENU_OFFSET+24;
            Rect cScrollRect = main_bitmap_view->GetBounds();
            // Now we can resize the window to match the size of the bitmap



            if (main_bitmap->GetBounds().Width() <= GetBounds().Width() && main_bitmap->GetBounds().Height() <= GetBounds().Height())
            {
                ResizeTo(400,400);
                main_bitmap_view->SetFrame(Rect(0,22,GetBounds().right, GetBounds().bottom-24));
                AddChild(main_bitmap_view);
            }

            else if (winWidth > GetBounds().Width()  || winHeight > GetBounds().Height())
            {
                ResizeTo(winWidth, winHeight);
                main_bitmap_view->SetFrame(Rect(0,22,GetBounds().right-15, GetBounds().bottom-39));
                AddChild( main_bitmap_view );

                g_pcVScrollBar = new  ScrollBar(Rect(cScrollRect.Width()+1,22, GetBounds().Width(),GetBounds().Height()-(pcStatusBar->GetBounds().top+pcStatusBar->GetBounds().bottom+13)),"sc",new Message(ID_SCROLL_VERTICAL),0,FLT_MAX,VERTICAL,CF_FOLLOW_RIGHT|CF_FOLLOW_TOP|CF_FOLLOW_BOTTOM);
                g_pcVScrollBar->SetScrollTarget(main_bitmap_view);

                AddChild(g_pcVScrollBar);

                g_pcHScrollBar = new ScrollBar(Rect(0,GetBounds().Height()-39,GetBounds().Width(),GetBounds().Height()-24), "sc2",new Message(ID_SCROLL_HORIZONTAL),0,FLT_MAX,HORIZONTAL, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT |CF_FOLLOW_BOTTOM);
                g_pcHScrollBar->SetScrollTarget(main_bitmap_view);

                AddChild(g_pcHScrollBar);

                g_pcHScrollBar->SetMinMax(0.0f,main_bitmap->GetBounds().Width()-main_bitmap_view->Width());
                g_pcVScrollBar->SetMinMax(0,main_bitmap->GetBounds().Height() - main_bitmap_view->Height() );

            }


            if(bSetTitle==true)
            {
                String sTitle = (String)"AView 0.4- " + (String)cFileName;
                char sRes[1024];
                sprintf((char*)sRes,"%.0fx%.0f",main_bitmap->GetBounds().Width()+1, main_bitmap->GetBounds().Height()+1);

                SetTitle(sTitle.c_str());
                pcStatusBar->SetText(cFileName,0,0);
                pcStatusBar->SetText(sRes,1,0);
            }
            else
            {
                SetTitle("AView 0.4 Beta");
            }

        }

        else
        {
            Alert* pcError = new Alert("Not Supported","This filetype is not supported by AtheOS\n", Alert::ALERT_INFO,0x00, "OK",NULL);
            pcError->Go(new Invoker());
        }
    }
}

void AppWindow::HandleKeyEvent( int nKeyCode )
{
    switch(nKeyCode)
    {

    case 30:  // Backspace - Go to previous image
        {
            if(current_image)      // If we arn't already at the last image
            {
                current_image--;        // We decrement the counter that points to the image currently loaded

                next_image=file_list[current_image].c_str();  // We need the c style String of the first image
                Load( next_image );                           // so that we can load it!

                main_bitmap_view->MakeFocus();
            }

            break;
        }

    case 38:   // Tab
    case 94:   // Space - Go to next image
        {
            if(current_image!=file_list.size())  // If we arn't at the end of the list
            {
                current_image++;

                next_image=file_list[current_image].c_str();  // We need the c style String of the first image
                Load( next_image );                           // so that we can load it!

                main_bitmap_view->MakeFocus();
            }

            break;
        }

    default:   // This isn't a recognised key code that we need to deal with
        break;

    }

}

void AppWindow::BuildDirList(const char* pzFPath)
{
    FDir_handle=opendir( pzFPath);

    char file_path[256];

    while((FDir_entry = readdir(FDir_handle)))
    {
        if(strcmp(FDir_entry->d_name,".") && strcmp(FDir_entry->d_name,".."))
        {

            strcpy( file_path, (char*) pzFPath);
            strcat( file_path, "/");
            strcat( file_path, FDir_entry->d_name);
            strcat( file_path, "\0");

            AppWindow::file_list.push_back( file_path);  // We store every file in a vector

        }

    }

    current_image=0;
    const char* first_image;

    first_image=file_list[current_image].c_str();  // We need the c style String of the first image
    Load( first_image );                           // so that we can load it!


}















