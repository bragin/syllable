#ifndef BITMAP_WINDOW_H
#define BITMAP_WINDOW_H

//#define _GNU_SOURCE

#include "include.h"
#include "iconview.h"
#include "loadbitmap.h"
#include "messages.h"
#include "properties.h"
#include "imageitem.h"
#include "bitmapscale.h"
#include "iconmenu.h"
#include "iconmenu_prop.h"
#include "settings.h"

using namespace os;

typedef vector <std::string> t_Icon;



#define M_CONFIG_CHANGE 20000019
#define M_DESKTOP_CHANGE 25005



class BitmapView : public View
{
    public:
        BitmapView( const Rect& cFrame);
        ~BitmapView();

        Icon*		FindIcon( const Point& cPos );
        void 		Erase( const Rect& cFrame );

        virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
        virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
        virtual void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
        virtual void	Paint( const Rect& cUpdateRect);
        void ReadPrefs();
        Color32_s zBgColor, zFgColor;
        void 			 SetIcons();
        void              RemoveIcons();
   
    private:
        Point		     m_cLastPos;
        Point		     m_cDragStartPos;
        Rect		     m_cSelRect;
        bigtime_t	     m_nHitTime;
        Bitmap*	     	 m_pcBitmap;
        std::vector<Icon*> m_cIcons;
        bool		     m_bCanDrag;
        bool		     m_bSelRectActive;
        Menu*	     	 pcMountMenu;
        Menu*            pcLineIcons;
        Menu*            pcMainMenu;
        void             HandleMessage(Message* pcMessage);
       // mounted_drives	 m_drives;
        PropWin* 		 pcProp;
        string 			 pzSyllableVer;
        string 			 zDImage;
        bool 			 bShow;
        IconMenu* 		 pcIconMenu;
        string 			 cIconName;
        string			 cIconExec;
        t_Icon 			 IconList();
        bool             bAlphbt;
        DeskSettings* pcSettings;
        Message* pcSetPrefs;
        

};

class BitmapWindow : public Window
{
    public:
        BitmapWindow();

    private:
        virtual void HandleMessage(Message* pcMessage);
        NodeMonitor*     pcConfigChange;
        NodeMonitor*     pcIconChange;
        BitmapView*		 pcBitmapView;
};

#endif








