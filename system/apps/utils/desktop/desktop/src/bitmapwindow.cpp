#include "bitmapwindow.h"
#include "iconmenu_messages.h"
#include "drives.h"
#include "drives_settings.h"

/*
** name:       LaunchFiles
** purpose:    Launches files in ~/config/desktop/startup
** parameters:
** returns:  
*/
void LaunchFiles()
{
    std::vector<string> launch_files;
    string zName;
    string zPath;
    Directory* pcDir = new Directory();

    if(pcDir->SetTo("~/Settings/Desktop/Startup")==0)
    {
        pcDir->GetPath(&zName);
        while (pcDir->GetNextEntry(&zName))
            if ( (zName.find( "..",0,1)==string::npos) && (zName.find( "Disabled",0)==string::npos))
            {

                launch_files.push_back(zName);
            }
    }
    for (uint32 n = 0; n < launch_files.size(); n++)
    {


        pid_t nPid = fork();
        if ( nPid == 0 )
        {
            set_thread_priority( -1, 0 );
            execlp(launch_files[n].c_str(), launch_files[n].c_str(), NULL );
            exit( 1 );
        }
    }
    delete pcDir;
}


/*
** name:       SyllableInfo
** purpose:    Returns a formatted string for when "Show version on desktop" is enabled 
** parameters: 
** returns:   String
*/
string SyllableInfo()
{
    string return_version;
    char zTmp[6];

    FILE* fin = popen("/usr/bin/uname -v 2>&1","r");
    fgets(zTmp, sizeof(zTmp),fin);
    pclose(fin);

    return_version = "Syllable " + (string)zTmp + ", " + "Desktop V0.5";
    return (return_version);
}




/*
** name:       ScreenRes
** purpose:    Returns the size of the screen.  Useful for "Show Version on desktop" option.
** parameters: 
** returns:   IPoint with the users screen resolution in it
*/
IPoint ScreenRes()
{
    Desktop d_desk;
    return d_desk.GetResolution();
}


/*
** name:       ReadBitmap
** purpose:    Checks to see if the image the user specifies for the desktop background is an actual file
				If not it returns the default one.  If it is, it returns that image.
** parameters: A pointer to a const char* containing the filename of the file
** returns:    Bitmap with specified image
*/
Bitmap* ReadBitmap(const char* zImageName)
{
    ifstream fs_image;
    Bitmap* pcBitmap = NULL;
    DeskSettings* pcSet = new  DeskSettings();

    if ( strcmp(zImageName,"^") != 0)
    {


        string zImagePath = (string)pcSet->GetImageDir() +  (string)zImageName;


        fs_image.open(zImagePath.c_str());

        if (fs_image == NULL)
        {
            fs_image.close();
            pcBitmap = LoadBitmapFromResource("background.jpg");

        }


        else
        {

            fs_image.close();

            if (pcSet->GetImageSize() == 0)
            {

                if ( (LoadBitmapFromFile(zImagePath.c_str())->GetBounds().Width() > ScreenRes().x )  || (LoadBitmapFromFile(zImagePath.c_str())->GetBounds().Height() > ScreenRes().y )  || (LoadBitmapFromFile(zImagePath.c_str())->GetBounds().Height() < ScreenRes().y ) || (LoadBitmapFromFile(zImagePath.c_str())->GetBounds().Width() > ScreenRes().x ))
                {
                    Bitmap* pcLargeBitmap = new Bitmap(ScreenRes().x, ScreenRes().y, CS_RGB32,Bitmap::SHARE_FRAMEBUFFER);
                    Scale(LoadBitmapFromFile(zImagePath.c_str()), pcLargeBitmap, filter_mitchell, 0);
                    pcBitmap = pcLargeBitmap;

                }


                else
                {
                    pcBitmap = LoadBitmapFromFile(zImagePath.c_str() );
                }



            }


            else
            {
                pcBitmap = LoadBitmapFromFile(zImagePath.c_str() );
            }

        }
    }

    else
    {
        Bitmap* pcLargeBitmap = new Bitmap(ScreenRes().x, ScreenRes().y, CS_RGB32,Bitmap::SHARE_FRAMEBUFFER);
        Scale(LoadBitmapFromResource("background.jpg"), pcLargeBitmap, filter_mitchell, 0);
        pcBitmap = pcLargeBitmap;
    }

    return (pcBitmap);
}



/*
** name:       Icon::Select
** purpose:    Invokes paint after erasing the BitmapView
** parameters: A pointer to the BitmapView and a bool that tells if the icon is selected
** returns:    
*/
void Icon::Select( BitmapView* pcView, bool bSelected )
{
    if ( m_bSelected == bSelected )
    {
        return;
    }
    m_bSelected = bSelected;

    pcView->Erase( GetFrame( pcView->GetFont() ) );

    Paint( pcView, Point(0,0), true, true,pcView->pcSettings->GetTrans(), pcView->zBgColor, pcView->zFgColor );
}


/*
** name:       BitmapView Constructor
** purpose:    Constructor for the BitmapView
** parameters: Rect for the Frame
** returns:   
*/
BitmapView::BitmapView( const Rect& cFrame ) :
        View( cFrame, "_bitmap_view", CF_FOLLOW_ALL)
{
    pcSettings = new DeskSettings();
	ReadPrefs();
    t_Menus mAddMenu = AddMenus();
    Menu* pcItemsMenu = new Menu(Rect(0,0,0,0),"New",ITEMS_IN_COLUMN);
    pcItemsMenu->AddItem(new ImageItem("New Shortcut...",NULL, "",LoadBitmapFromResource("shortcut.png")));
    pcItemsMenu->AddItem(new ImageItem("New Directory...",NULL, "",LoadBitmapFromResource("folder.png")));
    pcItemsMenu->AddItem(new MenuSeparator());
     for (uint i=0; i< mAddMenu.size(); i++)
    	pcItemsMenu->AddItem(mAddMenu[i]);
    pcMainMenu = new Menu(Rect(0,0,0,0),"",ITEMS_IN_COLUMN);
    pcMountMenu = new Menu(Rect(0,0,0,0),"Mount Drives    ",ITEMS_IN_COLUMN);
    
    ImageItem* pcNewItems = new ImageItem(pcItemsMenu, NULL, LoadBitmapFromResource("new.png"), 2.7);
   
    
    
    Drives* pcSyllMenu = new Drives(this);
    pcMainMenu->AddItem(new ImageItem(pcSyllMenu, NULL, LoadBitmapFromResource("mount.png"), 8.8 ) );
    pcMainMenu->AddItem(new MenuSeparator());

    pcMainMenu->AddItem(pcNewItems);
    pcMainMenu->AddItem(new MenuSeparator());
    
    
    pcMainMenu->AddItem(new ImageItem("Properties", new Message(M_PROPERTIES_SHOW),"", LoadBitmapFromResource("kcontrol.png")));
    pcMainMenu->AddItem(new MenuSeparator());
    pcMainMenu->AddItem(new ImageItem("Logout",new Message(M_LOGOUT_ALERT),"", LoadBitmapFromResource("exit.png")));
    pcMainMenu->GetPreferredSize(false);

    pcIconMenu = new IconMenu();
    pcIconMenu->SetTargetForItems(this);
	

    m_bCanDrag = false;
    m_bSelRectActive = false;

    SetIcons();
    m_nHitTime = 0;
    pzSyllableVer = SyllableInfo();

    LaunchFiles();

}


/*
** name:       ~BitmapView
** purpose:    Destructor for the BitmapView
** parameters:
** returns:    
*/
BitmapView::~BitmapView()
{
    delete m_pcBitmap;
    RemoveIcons();

}


t_Menus BitmapView::AddMenus()
{
	string zName;
	string zExecute;
	string zExtName;
	string zShort;
	t_Menus mAddMenus;
    Directory *pcDir = new Directory( );
    Rect cRect;
    //int8 nData;
    //uint8 nDataTwo;
    //Bitmap* pcBitmap;
    //void* pData;
    if( pcDir->SetTo( pcSettings->GetExtDir() ) == 0 )
    {
		
        while( pcDir->GetNextEntry( &zName ) ){
        	string zDir = pcSettings->GetExtDir() +(string)"/"+ zName;
        	if (zDir.find( "..",0,1)==string::npos)
        	{
        		
        		 FSNode *pcNode = new FSNode();

    			if( pcNode->SetTo(zDir.c_str()) == 0 )
    			{
        			File *pcConfig = new File( *pcNode );
        			uint32 nSize = pcConfig->GetSize( );
        			void *pBuffer = malloc( nSize );
        			pcConfig->Read( pBuffer, nSize );
        			Message *pcPrefs = new Message( );
        			pcPrefs->Unflatten( (uint8 *)pBuffer );
        			pcPrefs->FindString("Extension Name",&zExtName);
        			pcPrefs->FindString("Extension Executes", &zExecute);
        			pcPrefs->FindString("Extension Shortcut", &zShort);
        			/*pcPrefs->FindRect("Bitmap Rect",&cRect); 
        			pcPrefs->FindInt8("Bitmap Data",&nData); 
        			pcBitmap = new Bitmap((int)cRect.Width(), (int)cRect.Height(), CS_RGB32);
        			memcpy( pcBitmap->LockRaster(),&nData,sizeof(pcBitmap));
        			*/
        			delete pcConfig;
        			mAddMenus.push_back(new ImageItem(zExtName.c_str(), pcPrefs, zShort.c_str()));
        		}
        		delete pcNode;
        	}
           }
    }

    delete pcDir;
	
	
	return mAddMenus;
}

/*
** name:       BitmapView::IconList
** purpose:    Builds a list of .desktop files in ~/Desktop
** parameters: 
** returns:    t_icon - a vector of strings containing the names of the .desktop(s) for later use
*/
t_Icon BitmapView::IconList()
{
    string zName;
    t_Icon t_icon;
    Directory *pcDir = new Directory( );
	struct stat sStat; 
	string zIconDir =  "/boot/atheos" + (string)pcSettings->GetIconDir();
    if( pcDir->SetTo( zIconDir.c_str() ) == 0 )
    {
		
        while( pcDir->GetNextEntry( &zName ) )
        {
        	string zDir = zIconDir + zName;
			stat(zDir.c_str(), &sStat);
        	FSNode* psNode = new FSNode(zDir);
        	if ( S_ISDIR( sStat.st_mode ) && zName.find( "..",0,1)==string::npos)
        		t_icon.push_back(zName);
        	
        	if (!(zName.find( ".desktop",0)==string::npos) )
            	t_icon.push_back(zName);
        }
    }

    delete pcDir;
    return (t_icon);
}


/*
** name:       BitmapView::SetIcons
** purpose:    Sets the icons up using IconList() as a template for the icon names.
** parameters: A pointer to a LauncherMessage containing config info for the plugin
** returns:   
*/
void BitmapView::SetIcons()
{
    char zIconName[1024];
    char zIconImage[1024];
    char zIconExec[1024];
    char zPos[1024];
    char junk[1024];
    struct stat sStat;
    Point zIconPoint(0,0);
    Point zIconPoint2(0,0);
    uint iconExec = 0;
    Point cPos( 20, 20 );
    t_Icon t_IconList = IconList();
    float fyPoint = 0;
    for (iconExec = 0; iconExec < t_IconList.size(); iconExec++)
    {
        ifstream filRead;
        string pzIconPath = (string)pcSettings->GetIconDir() + t_IconList[iconExec].c_str();
        stat(pzIconPath.c_str(), &sStat);
        
        if ( S_ISDIR( sStat.st_mode ))
        	continue;
        	
        filRead.open(pzIconPath.c_str());
        filRead.getline(junk,1024);
        filRead.getline(zIconName,1024);
        
        for(int i=0; i <2; i++)
            filRead.getline(junk,1024);
        
        filRead.getline(zIconImage,1024);
        
        for(int i=0; i <2; i++)
        	filRead.getline(junk,1024);
        
        filRead.getline(zIconExec,1024);
        
        for(int i=0; i <2; i++)
        	filRead.getline(junk,1024);
        
        filRead.getline(zPos,1024);
        filRead.close();

        sscanf(zPos,"%f %f\n",&zIconPoint.x,&zIconPoint.y);
		
        if (zIconPoint.y > fyPoint)
            fyPoint = zIconPoint.y;
		
		if(zIconPoint2.y < zIconPoint.y || zIconPoint2.x < zIconPoint.x)
			zIconPoint2 = zIconPoint;
        
        m_cIcons.push_back(new Icon(zIconName, zIconImage,zIconExec, zIconPoint,sStat));
    }

	for (iconExec = 0; iconExec < t_IconList.size(); iconExec++)
    {
        string pzIconPath = (string)pcSettings->GetIconDir() + t_IconList[iconExec].c_str();
        stat(pzIconPath.c_str(), &sStat);
        if ( S_ISDIR( sStat.st_mode )){
        	string zDirIconExec = "LBrowser " + pzIconPath;
        	m_cIcons.push_back(new Icon(t_IconList[iconExec].c_str(), "/system/icons/folder.icon", zDirIconExec.c_str(), Point(zIconPoint2.x ,zIconPoint2.y+53), sStat));
        	zIconPoint2.y+=53; 
        	continue;
        	}
    }  	
    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        cPos.y += 50;
        if ( cPos.y > 500 )
        {
            cPos.y = 20;
            cPos.x += 50;

            m_cIcons[i]->Paint( this, Point(0,0), true, true, pcSettings->GetTrans(), zBgColor, zFgColor );

        }
        fIconPoint = fyPoint;
        Paint(GetBounds());

    }

}



/*
** name:       BitmapView::Paint
** purpose:    Paints the BitmapView on to the screen
** parameters: Rect that holds the size of the BitmapView
** returns:    
*/
void BitmapView::Paint( const Rect& cUpdateRect)
{
    Rect cBounds = GetBounds();
    SetDrawingMode( DM_COPY ); //COPY
    Font* pcFont = GetFont();

    Erase( cUpdateRect );

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cUpdateRect ) )
        {
            m_cIcons[i]->Paint( this, Point(0,0), true, true, pcSettings->GetTrans(), zBgColor, zFgColor );
        }
    }

    if (bShow == true)
    {
        SetFgColor(0,0,0);
        SetDrawingMode(DM_BLEND);  // BLEND
        MovePenTo(ScreenRes().x - 15 - GetStringWidth(pzSyllableVer) , ScreenRes().y -10);
        DrawString(pzSyllableVer);
    }

}

/*
** name:       BitmapView::Erase
** purpose:    Erases the BitmapView
** parameters: Rect that holds the size of the BitmapView
** returns:    
*/
void BitmapView::Erase( const Rect& cFrame )
{

    if ( m_pcBitmap != NULL )
    {
        Rect cBmBounds = m_pcBitmap->GetBounds();
        int nWidth  = int(cBmBounds.Width()) + 1;
        int nHeight = int(cBmBounds.Height()) + 1;
        SetDrawingMode( DM_COPY );
        for ( int nDstY = int(cFrame.top) ; nDstY <= cFrame.bottom ; )
        {
            int y = nDstY % nHeight;
            int nCurHeight = min( int(cFrame.bottom) - nDstY + 1, nHeight - y );

            for ( int nDstX = int(cFrame.left) ; nDstX <= cFrame.right ; )
            {
                int x = nDstX % nWidth;
                int nCurWidth = min( int(cFrame.right) - nDstX + 1, nWidth - x );

                Rect cRect( 0, 0, nCurWidth - 1, nCurHeight - 1 );
                DrawBitmap( m_pcBitmap, cRect + Point( x, y ), cRect + Point( nDstX, nDstY ) );
                nDstX += nCurWidth;
            }
            nDstY += nCurHeight;


        }
    }
    else
    {
        FillRect( cFrame, Color32_s( 0x00, 0x00, 0x00, 0 ) );
    }
}

/*
** name:       BitmapView::FindIcon
** purpose:    Find the icon that is selected
** parameters: Point that tell where the mouse is
** returns:    NULL or the Icon that is selected
*/
Icon* BitmapView::FindIcon( const Point& cPos )
{
    Font* pcFont = GetFont();

    for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
    {
        if ( m_cIcons[i]->GetFrame( pcFont ).DoIntersect( cPos ) )
        {
            return( m_cIcons[i] );
        }
    }
    return( NULL );
}

/*
** name:       BitmapView::MouseDown
** purpose:    Paints the BitmapView on to the screen
** parameters: Point where the tip of the cursor is and an int that tells what button is pushed
** returns:    
*/
void BitmapView::MouseDown( const Point& cPosition, uint32 nButtons )
{
    MakeFocus( true );

    Icon* pcIcon = FindIcon( cPosition );
    if(nButtons ==1)
    {
        if ( pcIcon != NULL )
        {
            if (  pcIcon->m_bSelected )
            {
                if ( m_nHitTime + 500000 >= get_system_time() )
                {
                    pid_t nPid = fork();
                    if ( nPid == 0 )
                    {
                        set_thread_priority( -1, 0 );
                        std::string pzExec = pcIcon->GetExecName();
                        int nPos = pzExec.find(' ');

                        if (nPos == string::npos)
                        {
                            execlp(pzExec.c_str(), pzExec.c_str(), NULL );
                            exit( 1 );
                        }
                        else
                        {

                            execlp(pzExec.substr(0,nPos).c_str(), pzExec.substr(0,nPos).c_str(),pzExec.substr(nPos+1).c_str(), NULL);
                            exit(1);
                        }
                    }
                }
                else
                {
                    m_bCanDrag = true;
                }
                m_nHitTime = get_system_time();
                return;
            }
        }
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            m_cIcons[i]->Select( this, false );
        }
        if ( pcIcon != NULL )
        {
            m_bCanDrag = true;
            pcIcon->Select( this, true );
        }
        else
        {
            m_bSelRectActive = true;
            m_cSelRect = Rect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
            SetDrawingMode( DM_INVERT );
            DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        }
        Flush();
        m_cLastPos = cPosition;
        m_nHitTime = get_system_time();
    }

    else if (nButtons == 2)
    {

        if (pcIcon !=NULL)
        {

            m_bSelRectActive = true;
            m_cSelRect = Rect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
            SetDrawingMode( DM_INVERT );
            DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
            pcIcon->Select( this,true );
            Flush();

            if (  pcIcon->m_bSelected )
                cIconName = pcIcon->GetName();
            cIconExec = pcIcon->GetExecName();
            cIconPic  = pcIcon->GetBitmap();

            pcIconMenu->Open(ConvertToScreen(cPosition));
            pcIconMenu->SetTargetForItems(this);
        }

        else if(pcIcon == NULL)
        {
            pcMainMenu->Open(ConvertToScreen(cPosition));
            pcMainMenu->SetTargetForItems(this);
        }

    }
}


void BitmapView::RemoveIcons()
{
    for (uint i=m_cIcons.size(); i>0; i--)
    {
        m_cIcons.pop_back();
    }

    Erase(this->GetBounds());
}

/*
** name:       BitmapView::MouseUp
** purpose:    When the mouse button is depressed, this is executed
** parameters: Point where the cursor is, int that tells what button is pressed, Message
** returns:    
*/
void BitmapView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
    m_bCanDrag = false;

    Font* pcFont = GetFont();
    if ( m_bSelRectActive )
    {
        SetDrawingMode( DM_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        m_bSelRectActive = false;

        if ( m_cSelRect.left > m_cSelRect.right )
        {
            float nTmp = m_cSelRect.left;
            m_cSelRect.left = m_cSelRect.right;
            m_cSelRect.right = nTmp;
        }
        if ( m_cSelRect.top > m_cSelRect.bottom )
        {
            float nTmp = m_cSelRect.top;
            m_cSelRect.top = m_cSelRect.bottom;
            m_cSelRect.bottom = nTmp;
        }

        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            m_cIcons[i]->Select( this, m_cSelRect.DoIntersect( m_cIcons[i]->GetFrame( pcFont ) ) );
        }
        Flush();
    }
    else if ( pcData != NULL && pcData->ReturnAddress() == Messenger( this ) )
    {
        Point cHotSpot;
        pcData->FindPoint( "_hot_spot", &cHotSpot );


        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            Rect cFrame = m_cIcons[i]->GetFrame( pcFont );
            Erase( cFrame );
        }
        Flush();

        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            if ( m_cIcons[i]->m_bSelected )
            {
                m_cIcons[i]->m_cPosition += cPosition - m_cDragStartPos;
            }
            m_cIcons[i]->Paint( this, Point(0,0), true, true, pcSettings->GetTrans(), zBgColor, zFgColor );
        }
        Flush();
    }
}


/*
** name:       BitmapView::MouseMove
** purpose:    Executed when the mouse is moved
** parameters: Point that holds where the cursor is, 
** returns:    
*/
void BitmapView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
    m_cLastPos = cNewPos;

    if ( (nButtons & 0x01) == 0 )
    {
        return;
    }

    if ( m_bSelRectActive )
    {
        SetDrawingMode( DM_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        m_cSelRect.right = cNewPos.x;
        m_cSelRect.bottom = cNewPos.y;

        Rect cSelRect = m_cSelRect;
        if ( cSelRect.left > cSelRect.right )
        {
            float nTmp = cSelRect.left;
            cSelRect.left = cSelRect.right;
            cSelRect.right = nTmp;
        }
        if ( cSelRect.top > cSelRect.bottom )
        {
            float nTmp = cSelRect.top;
            cSelRect.top = cSelRect.bottom;
            cSelRect.bottom = nTmp;
        }
        Font* pcFont = GetFont();
        SetDrawingMode( DM_COPY );
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            m_cIcons[i]->Select( this, cSelRect.DoIntersect( m_cIcons[i]->GetFrame( pcFont ) ) );
        }

        SetDrawingMode( DM_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );

        Flush();
        return;
    }

    if ( m_bCanDrag )
    {
        Flush();

        Icon* pcSelIcon = NULL;

        Rect cSelFrame( 1000000, 1000000, -1000000, -1000000 );

        Font* pcFont = GetFont();
        Message cData(1234);
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
        {
            if ( m_cIcons[i]->m_bSelected )
            {
                cData.AddString( "file/path", m_cIcons[i]->GetName().c_str() );
                cSelFrame |= m_cIcons[i]->GetFrame( pcFont );
                pcSelIcon = m_cIcons[i];
            }
        }
        if ( pcSelIcon != NULL )
        {
            m_cDragStartPos = cNewPos; // + cSelFrame.LeftTop() - cNewPos;

            if ( (cSelFrame.Width()+1.0f) * (cSelFrame.Height()+1.0f) < 12000 )
            {
                Bitmap cDragBitmap( cSelFrame.Width()+1.0f, cSelFrame.Height()+1.0f, CS_RGB32, Bitmap::ACCEPT_VIEWS | Bitmap::SHARE_FRAMEBUFFER );

                View* pcView = new View( cSelFrame.Bounds(), "" );
                cDragBitmap.AddChild( pcView );

                pcView->SetFgColor( 255, 255, 255, 0 );
                pcView->FillRect( cSelFrame.Bounds() );


                for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
                {
                    if ( m_cIcons[i]->m_bSelected )
                    {
                        m_cIcons[i]->Paint( pcView, -cSelFrame.LeftTop(), true, false,pcSettings->GetTrans(), zBgColor, zFgColor );
                    }
                }
                cDragBitmap.Sync();

                uint32* pRaster = (uint32*)cDragBitmap.LockRaster();

                for ( int y = 0 ; y < cSelFrame.Height() + 1.0f ; ++y )
                {
                    for ( int x = 0 ; x < cSelFrame.Width()+1.0f ; ++x )
                    {
                        if ( pRaster[x + y * int(cSelFrame.Width()+1.0f)] != 0x00ffffff &&
                                (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0xff000000) == 0xff000000 )
                        {
                            pRaster[x + y * int(cSelFrame.Width()+1.0f)] = (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0x00ffffff) | 0xb0000000;
                        }
                    }
                }
                BeginDrag( &cData, cNewPos - cSelFrame.LeftTop(), &cDragBitmap );

            }
            else
            {
                BeginDrag( &cData, cNewPos - cSelFrame.LeftTop(), cSelFrame.Bounds() );
            }
        }
        m_bCanDrag = false;
    }
    Flush();
}


/*
** name:       BitmapView::ReadPrefs
** purpose:    Reads the Flattend message from the config file.
** parameters: void
** returns:    
*/
void BitmapView::ReadPrefs(void)
{
    pcSettings = NULL;
    pcSettings = new DeskSettings();
    pcSetPrefs = NULL;
    pcSetPrefs =  pcSettings->GetSettings();
    pcSetPrefs->FindColor32( "Back_Color", &zBgColor );
    pcSetPrefs->FindColor32( "Icon_Color",   &zFgColor );
    pcSetPrefs->FindString ( "DeskImage",  &zDImage  );
    pcSetPrefs->FindBool   ( "ShowVer",    &bShow   );
    pcSetPrefs->FindBool   ( "Alphabet",   &bAlphbt);

    m_pcBitmap = ReadBitmap(zDImage.c_str());
} /*end of ReadPrefs()*/

/*
** name:       BitmapView::HandleMessage
** purpose:    Handles the messages from BitmapView.
** parameters: Message that holds data from the BitmapView
** returns:    
*/
void BitmapView::HandleMessage(Message* pcMessage)
{
    Alert* pcAlert = NULL;
    switch (pcMessage->GetCode())
    {

    case ID_ICON_PROPERTIES:
        {
            Window* pcIconProp = new IconProp(cIconName, cIconExec, cIconPic);
            pcIconProp->Show();
            pcIconProp->MakeFocus();
        }
        break;

    case M_PROPERTIES_SHOW:

        pcProp = new PropWin();
        pcProp->Show();
        pcProp->MakeFocus();

        break;

    case M_LOGOUT_USER:
        {
            int32 nAlertBut;
            pcMessage->FindInt32( "which", &nAlertBut );

            if ( nAlertBut == 0)
            {
                GetWindow()->OkToQuit();

            }
            break;
        }

    case M_LOGOUT_ALERT:
        pcAlert = new Alert("Question:","Do you really want to logout?\n\nRemember that all applications\nwill be closed when you log out.\n",Alert::ALERT_INFO,0, "Yes","No",NULL);
        pcAlert->Go(new Invoker(new Message (M_LOGOUT_USER), this));
        break;

    case M_PROPERTIES_FOCUS:
        break;

    case M_SHOW_DRIVE_INFO:
        {
            string zAlertInfo;
            string zAlertName;
            pcMessage->FindString("Alert_Name", &zAlertName);
            pcMessage->FindString("Alert_Info",  &zAlertInfo);
            (new Alert (zAlertName.c_str(),zAlertInfo.c_str(),Alert::ALERT_INFO,0,"Ok",NULL))->Go(new Invoker());
            break;
        }
    case M_DRIVES_UNMOUNT:

        pcAlert = new Alert("Alert!!!","Unmount doesn't work yet!!!\n", 0, "OK",NULL );
        pcAlert->Go(new Invoker());

        break;


    case M_SHOW_DRIVE_SETTINGS:
        {
            Window* pcDrives = new DriveWindow(GetWindow(), fIconPoint);
            if (getuid() == 0)
            {

                pcDrives->Show();
                pcDrives->MakeFocus();

            }
            else
            {
                Alert* pcAlert = new Alert("Warning", "Only root can open this dialog.\n\nHowever, would you like to place the\ndrive icons on your desktop?\n", Alert::ALERT_INFO,0, "Yes","No",NULL);
                pcAlert->Go(new Invoker(new Message (M_DRIVE_ICON), this));
            }
        }
        break;

    case M_DRIVE_ICON:
        {
            int32 nAlertBut;
            pcMessage->FindInt32( "which", &nAlertBut );

            if ( nAlertBut == 0)
            {

                break;
            }

            break;
        }
    default:
        View::HandleMessage(pcMessage);
        break;
    }
} /*end of HandleMessage()*/


/*
** name:       BitmapWindow
** purpose:    Constructor for the BitmapWindow
** parameters: 
** returns:    
*/
BitmapWindow::BitmapWindow() : Window(Rect( 0, 0, 1599, 1199 ), "_bitmap_window", "Desktop", WND_NO_BORDER | WND_BACKMOST ,ALL_DESKTOPS )//,WND_BACKMOST || WND_NO_BORDER
{

    DeskSettings* pcSet = new DeskSettings();
    pcConfigChange = new NodeMonitor(pcSet->GetSetDir(),NWATCH_DIR,this); /*note must work on NodeMonitor*/
    pcIconChange = new NodeMonitor(pcSet->GetIconDir(),NWATCH_DIR,this);

    pcBitmapView = new BitmapView( GetBounds());
    AddTimer(pcBitmapView,5,5,true);
    AddChild( pcBitmapView );
}/*end of BitmwapWindow Constructor*/

/*
** name:       BitmapWindow::HandleMessage
** purpose:    Handles the message between the BitmapWindow
** parameters: Message passed from the BitmapWindow
** returns:    
*/
void BitmapWindow::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {
    case M_CONFIG_CHANGE:
        pcBitmapView->ReadPrefs();
        pcBitmapView->RemoveIcons();
        pcBitmapView->SetIcons();
        pcBitmapView->Sync();
        break;
    }
} /*end of HandleMessage()*/

bool BitmapWindow::OkToQuit(void)
{
	Application::GetInstance()->PostMessage(M_QUIT );
    return (true);
} /*end of OkToQuit()*/








































