/*************************************************/
/* drawtxt.cpp : Function to draw and plot texts */
/*************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "plot_common.h"

#include "trigo.h"
#include "macros.h"

#ifndef DEFAULT_SIZE_TEXT
#define DEFAULT_SIZE_TEXT 50
#endif

#define EDA_DRAWBASE
#include "grfonte.h"

/* fonctions locales : */


/****************************************************************************************************/
void DrawGraphicText( WinEDA_DrawPanel* aPanel, wxDC* DC,
                      const wxPoint& aPos, EDA_Colors aColor, const wxString& aText,
                      int aOrient, const wxSize& aSize,
                      enum GRTextHorizJustifyType aH_justify,
                      enum GRTextVertJustifyType aV_justify,
                      int aWidth,  bool aItalic,
                      void (* aCallback) (int x0, int y0, int xf, int yf))
/****************************************************************************************************/

/** Function DrawGraphicText
 * Draw a graphic text (like module texts)
 * Draw a graphic text (like module texts)
 *  @param aPanel = the current DrawPanel
 *  @param aPos = text position (according to h_justify, v_justify)
 *  @param aColor (enum EDA_Colors) = text color
 *  @param aText = text to draw
 *  @param aOrient = angle in 0.1 degree
 *  @param aSize = text size (size.x or size.y can be < 0 for mirrored texts)
 *  @param aH_justify = horizontal justification (Left, center, right)
 *  @param aV_justify = vertical justification (bottom, center, top)
 *  @param aWidth = line width (pen width) (default = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *  @param aItalic = true to simulate an italic font
 *  @param aCallback() = function called (if non null) to draw each segment.
 *                  used only to draw 3D texts
 */
{
    int            ii, kk, char_count, AsciiCode, endcar;
    int            x0, y0;
    int            zoom;
    int            size_h, size_v, pitch;
    SH_CODE        f_cod, plume = 'U';
    const SH_CODE* ptcar;
    int            ptr;
    int            ux0, uy0, dx, dy;    // Draw coordinate for segments to draw. also used in some other calculation
    int            cX, cY;              // Texte center
    int            ox, oy;              // Draw coordinates for the current char
    int            coord[100];          // Buffer coordinate used to draw polylines (char shapes)
    bool           sketch_mode = false;

    if ( aPanel )
        zoom = aPanel->GetZoom();
    else
        zoom = 1;

    size_h = aSize.x;
    size_v = aSize.y;

    if( aWidth < 0 )
    {
        aWidth = -aWidth;
        sketch_mode = TRUE;
    }

    kk = 0;
    ptr = 0;   /* ptr = text index */

    char_count = aText.Len();
    if( char_count == 0 )
        return;

    pitch = (10 * size_h ) / 9;    // this is the pitch between chars
    if ( pitch > 0 )
        pitch += ABS(aWidth);
    else
        pitch -= ABS(aWidth);

    ox = cX = aPos.x;
    oy = cY = aPos.y;

    /* Do not draw the text if out of draw area! */
    if( aPanel )
    {
        int xm, ym, ll, xc, yc;
        int textsize = ABS( pitch );
        ll = (textsize * char_count) / zoom;

        xc = GRMapX( cX );
        yc = GRMapY( cY );

        x0 = aPanel->m_ClipBox.GetX() - ll;
        y0 = aPanel->m_ClipBox.GetY() - ll;
        xm = aPanel->m_ClipBox.GetRight() + ll;
        ym = aPanel->m_ClipBox.GetBottom() + ll;

        if( xc < x0 )
            return;
        if( yc < y0 )
            return;
        if( xc > xm )
            return;
        if( yc > ym )
            return;
    }


    /* Compute the position ux0, uy0 of the first letter , next */
    dx = (pitch * char_count) / 2;
    dy = size_v / 2;                        /* dx, dy = draw offset between first letter and text center */

    ux0 = uy0 = 0;                          /* Decalage du centre du texte / coord de ref */

    if( (aOrient == 0) || (aOrient == 1800) ) /* Horizontal Text */
    {
        switch( aH_justify )
        {
        case GR_TEXT_HJUSTIFY_CENTER:
            break;

        case GR_TEXT_HJUSTIFY_RIGHT:
            ux0 = -dx;
            break;

        case GR_TEXT_HJUSTIFY_LEFT:
            ux0 = dx;
            break;
        }

        switch( aV_justify )
        {
        case GR_TEXT_VJUSTIFY_CENTER:
            break;

        case GR_TEXT_VJUSTIFY_TOP:
            uy0 = dy;
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            uy0 = -dy;
            break;
        }
    }
    else    /* Vertical Text */
    {
        switch( aH_justify )
        {
        case GR_TEXT_HJUSTIFY_CENTER:
            break;

        case GR_TEXT_HJUSTIFY_RIGHT:
            ux0 = -dy;
            break;

        case GR_TEXT_HJUSTIFY_LEFT:
            ux0 = dy;
            break;
        }

        switch( aV_justify )
        {
        case GR_TEXT_VJUSTIFY_CENTER:
            break;

        case GR_TEXT_VJUSTIFY_TOP:
            uy0 = dx;
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            uy0 = -dx;
            break;
        }
    }

    cX += ux0;
    cY += uy0;

    ox = cX - dx;
    oy = cY + dy;

    if( (aSize.x / zoom) == 0 )
        return;

    if( ABS( (aSize.x / zoom) ) < 3 )    /* chars trop petits pour etre dessines */
    {                                   /* le texte est symbolise par une barre */
        dx = (pitch * char_count) / 2;
        dy = size_v / 2;                /* Decalage du debut du texte / centre */

        ux0 = cX - dx;
        uy0 = cY;

        dx += cX;
        dy = cY;

        RotatePoint( &ux0, &uy0, cX, cY, aOrient );
        RotatePoint( &dx, &dy, cX, cY, aOrient );

        if ( aCallback )
            aCallback( ux0, uy0, dx, dy );
        else
            GRLine( &aPanel->m_ClipBox, DC, ux0, uy0, dx, dy, aWidth, aColor );

        return;
    }

    while( kk++ < char_count )
    {
        x0 = 0; y0 = 0;
#if defined(wxUSE_UNICODE) && defined(KICAD_CYRILLIC)
	AsciiCode = aText.GetChar(ptr) & 0x7FF;
	if ( AsciiCode > 0x40F && AsciiCode < 0x450 ) // big small Cyr
	    AsciiCode = utf8_to_ascii[AsciiCode - 0x410] & 0xFF;
	else
	    AsciiCode = AsciiCode & 0xFF;
#else
        AsciiCode = aText.GetChar( ptr ) & 0xFF;
#endif
        ptcar = graphic_fonte_shape[AsciiCode];  /* ptcar pointe la description
                                                  *  du caractere a dessiner */

        for( ii = 0, endcar = FALSE; !endcar; ptcar++ )
        {
            f_cod = *ptcar;

            /* get code n de la forme selectionnee */
            switch( f_cod )
            {
            case 'X':
                endcar = TRUE;    /* fin du caractere */
                break;

            case 'U':
                if( ii && (plume == 'D' ) )
                {
                    if( aWidth <= 1 )
                       aWidth = 0;
                    if ( aCallback )
                    {
                        int ik, * coordptr;
                        coordptr = coord;
                        for( ik = 0; ik < (ii - 2); ik += 2, coordptr += 2 )
                            aCallback( *coordptr, *(coordptr + 1),
                                     *(coordptr + 2), *(coordptr + 3) );
                    }

                    else if( sketch_mode )
                    {
                        int ik, * coordptr;
                        coordptr = coord;
                        for( ik = 0; ik < (ii - 2); ik += 2, coordptr += 2 )
                            GRCSegm( &aPanel->m_ClipBox, DC, *coordptr, *(coordptr + 1),
                                     *(coordptr + 2), *(coordptr + 3), aWidth, aColor );
                    }
                    else
                        GRPoly( &aPanel->m_ClipBox, DC, ii / 2, coord, 0,
                                aWidth, aColor, aColor );
                }
                plume = f_cod; ii = 0;
                break;

            case 'D':
                plume = f_cod;
                break;

            default:
                {
                    int y, k1, k2;
                    y = k1 = f_cod;     /* trace sur axe V */
                    k1 = -( (k1 * size_v) / 9 );

                    ptcar++;
                    f_cod = *ptcar;

                    k2    = f_cod;  /* trace sur axe H */
                    k2    = (k2 * size_h) / 9;
                    // To simulate an italic font, add a x offset depending on the y offset
                    if ( aItalic )
                        k2 -= k1/8;
                    dx    = k2 + ox; dy = k1 + oy;

                    RotatePoint( &dx, &dy, cX, cY, aOrient );
                    coord[ii++] = dx;
                    coord[ii++] = dy;
                    break;
                }
            }

            /* end switch */
        }

        /* end draw 1 char */

        ptr++; ox += pitch;
    }
}



/* functions used to plot texts, using DrawGraphicText() with a call back function */
static void  (*MovePenFct)( wxPoint pos, int state ); // a pointer to actual plot function (HPGL, PS, ..)
static bool s_Plotbegin;        // Flag to init plot
/* The call back function */
static void s_Callback_plot(int x0, int y0, int xf, int yf)
{
    static wxPoint PenLastPos;
    wxPoint pstart;
    pstart.x = x0;
    pstart.y = y0;
    wxPoint pend;
    pend.x = xf;
    pend.y = yf;
    if ( s_Plotbegin )      // First segment to plot
    {
        MovePenFct( pstart, 'U' );
        MovePenFct( pend, 'D' );
        s_Plotbegin = false;
    }
    
    else
    {
        if ( PenLastPos == pstart )     // this is a next segment in a polyline
        {
            MovePenFct( pend, 'D' );
        }
        else                            // New segment to plot
        {
            MovePenFct( pstart, 'U' );
            MovePenFct( pend, 'D' );
        }
    }
    
    PenLastPos = pend;
}
/******************************************************************************************/
void PlotGraphicText( int aFormat_plot, const wxPoint& aPos, enum EDA_Colors aColor,
                      const wxString& aText,
                      int aOrient, const wxSize& aSize,
                      enum GRTextHorizJustifyType aH_justify,
                      enum GRTextVertJustifyType aV_justify,
                      int aWidth, bool aItalic )
/******************************************************************************************/

/** Function PlotGraphicText
 *  same as DrawGraphicText, but plot graphic text insteed of draw it
 *  @param aFormat_plot = plot format (PLOT_FORMAT_POST, PLOT_FORMAT_HPGL, PLOT_FORMAT_GERBER)
 *  @param aPos = text position (according to aH_justify, aV_justify)
 *  @param aColor (enum EDA_Colors) = text color
 *  @param aText = text to draw
 *  @param aOrient = angle in 0.1 degree
 *  @param aSize = text size (size.x or size.y can be < 0 for mirrored texts)
 *  @param aH_justify = horizontal justification (Left, center, right)
 *  @param aV_justify = vertical justification (bottom, center, top)
 *  @param aWidth = line width (pen width) (default = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *  @param aItalic = true to simulate an italic font
 */
{

    // Initialise the actual function used to plot lines:
    switch( aFormat_plot )
    {
    case PLOT_FORMAT_POST:
        MovePenFct = LineTo_PS;
        break;

    case PLOT_FORMAT_HPGL:
        MovePenFct = Move_Plume_HPGL;
        break;

    case PLOT_FORMAT_GERBER:
    default:
        return;
    }

    if( aColor >= 0 && IsPostScript( aFormat_plot ) )
        SetColorMapPS( aColor );

    s_Plotbegin = true;
    DrawGraphicText( NULL, NULL, aPos, aColor, aText,
                      aOrient, aSize,
                      aH_justify, aV_justify,
                      aWidth,  aItalic,
                      s_Callback_plot);

    /* end text : pen UP ,no move */
    MovePenFct( wxPoint( 0, 0 ), 'Z' );
}
