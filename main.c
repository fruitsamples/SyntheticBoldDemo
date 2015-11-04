/*

File:m main.c

Abstract: Main entry point and control & command event handling
routines for SyntheticBoldDemo project.

Version: <1.1>

Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
Computer, Inc. ("Apple") in consideration of your agreement to the
following terms, and your use, installation, modification or
redistribution of this Apple software constitutes acceptance of these
terms.  If you do not agree with these terms, please do not use,
install, modify or redistribute this Apple software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, Apple grants you a personal, non-exclusive
license, under Apple's copyrights in this original Apple software (the
"Apple Software"), to use, reproduce, modify and redistribute the Apple
Software, with or without modifications, in source and/or binary forms;
provided that if you redistribute the Apple Software in its entirety and
without modifications, you must retain this notice and the following
text and disclaimers in all such redistributions of the Apple Software. 
Neither the name, trademarks, service marks or logos of Apple Computer,
Inc. may be used to endorse or promote products derived from the Apple
Software without specific prior written permission from Apple.  Except
as expressly stated in this notice, no other rights or licenses, express
or implied, are granted by Apple herein, including but not limited to
any patent rights that may be infringed by your derivative works or by
other works in which the Apple Software may be incorporated.

The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.

IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Copyright � 2004-2007 Apple Inc., All Rights Reserved

*/ 

#include "globals.h"
#include "window.h"
#include "print.h"
#include "fontmenu.h"
#include "atsui.h"
#include "main.h"


// Globals
ControlRef gValueStringControl;
ControlRef gSliderControl;
ControlRef gStringInputControl;
ControlRef gUpdateButtonControl;

// Main entry point.  Sets things up, then runs the event loop
//
int main(int argc, char* argv[])
{
    char						startingFontName[] = "Geneva";
    int							startingFontSize = 48;
    ATSUFontID					font;
    OSStatus					err = noErr;

    // Set up the menubar and main window
    err = SetupMenuAndWindows();
    require_noerr( err, CantDoSetup );
    
    // Create the ATSUI data and draw it for the first time
    //
    verify_noerr( ATSUFindFontFromName(startingFontName, strlen(startingFontName), kFontFullName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &font) );
    verify( FindAndSelectFont(font) );
    SetATSUIStuffFont(font);
    SetATSUIStuffFontSize(Long2Fix(startingFontSize));
    SetUpATSUIStuff();
	HIViewSetNeedsDisplay( gView, true );

    // Call the event loop
    RunApplicationEventLoop();

CantDoSetup:
    return err;
}

// Creates the menu bar and window, then installs proper event handlers for them
//
OSStatus SetupMenuAndWindows(void)
{
    IBNibRef					nibRef;
    EventHandlerUPP				handlerUPP;
    EventTypeSpec				myEvents[] =
		{
			{ kEventClassWindow, kEventWindowClose },
			{ kEventClassControl, kEventControlDraw },
			{ kEventClassCommand, kEventCommandProcess },
			{ kEventClassControl, kEventControlHit }
		};
    WindowRef					dialog;
    ControlID					valueStringControID = { 'VALU', 0 };
    ControlID					sliderControID = { 'SLID', 0 };
    ControlID					stringInputControID = { 'TEXT', 0 };
    ControlID					updateButtonControID = { 'UPDT', 0 };
	static const HIViewID		viewID = { 'awin', 0 };
    OSStatus					err = noErr;
	
    // Create a Nib reference passing the name of the nib file (without the .nib extension)
    // CreateNibReference only searches into the application bundle.
    err = CreateNibReference(CFSTR("main"), &nibRef);
    require_noerr( err, CantGetNibRef );
    
    // Once the nib reference is created, set the menu bar. "MainMenu" is the name of the menu bar
    // object. This name is set in InterfaceBuilder when the nib is created.
    err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
    require_noerr( err, CantSetMenuBar );

    // Install the font menu
    err = InstallFontMenu(kFontMenuID);
    require_noerr( err, CantCreateFontMenu );

    // Then create a window. "MainWindow" is the name of the window object. This name is set in 
    // InterfaceBuilder when the nib is created.
    err = CreateWindowFromNib(nibRef, CFSTR("MainWindow"), &gWindow);
    require_noerr( err, CantCreateWindow );

    // Then create the other window. "Dialog" is the name of the window object. This name is set in 
    // InterfaceBuilder when the nib is created.
    err = CreateWindowFromNib(nibRef, CFSTR("Settings"), &dialog);
    require_noerr( err, CantCreateWindow );

    // We don't need the nib reference anymore.
    DisposeNibReference(nibRef);

    // The windows were created hidden, so show them.
    ShowWindow(gWindow);
    ShowWindow(dialog);

    // Get the control for the value string
    verify_noerr( GetControlByID(dialog, &valueStringControID, &gValueStringControl) );
    verify_noerr( GetControlByID(dialog, &sliderControID, &gSliderControl) );
    verify_noerr( GetControlByID(dialog, &stringInputControID, &gStringInputControl) );
    verify_noerr( GetControlByID(dialog, &updateButtonControID, &gUpdateButtonControl) );
	HIViewFindByID(HIViewGetRoot(gWindow), viewID, &gView);

    // Install a handler to quit the application when the window is closed
    handlerUPP = NewEventHandlerUPP(DoWindowClose);			// DoWindowClose() is defined in window.c
    verify_noerr( InstallWindowEventHandler(gWindow, handlerUPP, GetEventTypeCount(myEvents[0]), &myEvents[0], NULL, NULL) );

	// Install a handler to update the window
    handlerUPP = NewEventHandlerUPP(DoWindowBoundsChanged);	// DoWindowBoundsChanged() is defined in window.c
    verify_noerr( HIViewInstallEventHandler(gView, handlerUPP, GetEventTypeCount(myEvents[1]), &myEvents[1], (void *)gView, NULL) );

    // Install a handler for command events
    handlerUPP = NewEventHandlerUPP(DoCommandEvent);		// DoCommandEvent() is defined below
    verify_noerr( InstallApplicationEventHandler(handlerUPP, GetEventTypeCount(myEvents[2]), &myEvents[2], NULL, NULL) );
    
    // Install a handler for control value change event (for slider)
    handlerUPP = NewEventHandlerUPP(DoControlHitEvent);		// DoControlHitEvent() is defined below
    verify_noerr( InstallApplicationEventHandler(handlerUPP, GetEventTypeCount(myEvents[3]), &myEvents[3], NULL, NULL) );

    // Set up the global print session
    verify_noerr( InitializePrinting() );

CantCreateWindow:
CantCreateFontMenu:
CantSetMenuBar:
CantGetNibRef:
        return err;
}

// Handles command and menu events
//
pascal OSStatus DoCommandEvent(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData)
{
    HICommand					theCommand;
    UInt32						theCommandID;
    MenuRef						theMenu;
    MenuItemIndex				theItem;
    FMFont						font;
    OSStatus					status = eventNotHandledErr;
    Boolean						needsRedrawing = false;

    
    // Get the HICommand from the event structure, then get the menu reference and item out of that
    verify_noerr( GetEventParameter(theEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &theCommand) );
    theCommandID = theCommand.commandID;
    theMenu = theCommand.menu.menuRef;
    theItem = theCommand.menu.menuItemIndex;

    if ( GetMenuID(theMenu) >= kFontMenuID )
	{
		// Handle font menu
        font = SelectAndGetFont(theMenu, theItem);
        SetATSUIStuffFont(font);
        UpdateATSUIStyle();

        needsRedrawing = true;
        status = noErr;
    }
    else if ( (theCommandID >> 24) == 'Z' )
	{
		// Handle font size menu
        UInt32				numericalBits;
        char				string[5];
        
        // The last three bytes of the CommandID are an ASCII representation of the font size
        numericalBits = theCommandID & 0x00FFFFFF;
        memcpy(string, &numericalBits, sizeof(UInt32));
        string[0] = '0';
        string[4] = 0x00;
        SetATSUIStuffFontSize(Long2Fix(atoi(string)));
        UpdateATSUIStyle();

        // Update the menu
        verify_noerr( SetMenuCommandMark(NULL, gCurrentFontSizeCommandID, kMenuNoMark) );
        verify_noerr( SetMenuCommandMark(NULL, theCommandID, kMenuCheckMark) );
        gCurrentFontSizeCommandID = theCommandID;

        needsRedrawing = true;
        status = noErr;
    }
    else switch (theCommandID)
	{
		// Handle other menu commands
        case kHICommandPageSetup:
            verify_noerr( DoPageSetupDialog() );
            status = noErr;
            needsRedrawing = true;
            break;
        case kHICommandPrint:
            status = DoPrintDialog();
            if ( status == noErr )
			{
                DoPrintLoop();
            }
            else if ( status != kPMCancel )
			{
                check_string( (status == noErr), "DoPrintDialog returned an error" );
            }
            status = noErr;
            needsRedrawing = true;
            break;
    }

    // Redraw if necessary
    if (needsRedrawing)
		HIViewSetNeedsDisplay( gView, true );
    return status;
}


// Checks for updates to control
//
pascal OSStatus DoControlHitEvent(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData)
{
    ControlRef					thisControl;
    char						buffer[256];
    CFStringRef					valueString;
    CFStringRef					editString;
    
    // Figure out which control this came from
    verify_noerr( GetEventParameter(theEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &thisControl) );
    
    if ( thisControl == gSliderControl )
	{
        // Get the value
        gStrokeThicknessFactor = GetControl32BitValue(thisControl) / 1000.0;
    
        // Give feedback
        snprintf(buffer, 255, "%0.3f\n", gStrokeThicknessFactor);
        valueString = CFStringCreateWithCString(NULL, buffer, kCFStringEncodingASCII);
        verify_noerr( SetControlData(gValueStringControl, 0, kControlStaticTextCFStringTag, sizeof(CFStringRef), &valueString) );
        CFRelease(valueString);
 		HIViewSetNeedsDisplay( gValueStringControl, true );
   
        // Update the display
		HIViewSetNeedsDisplay( gView, true );
        
        return noErr;
    }
    else if ( thisControl == gUpdateButtonControl )
	{
        // Get the string from the user
        verify_noerr( GetControlData(gStringInputControl, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), (void *)&editString, NULL) );
        UpdateATSUIStuffString(editString);
        CFRelease(editString);

        // Update the display
		HIViewSetNeedsDisplay( gView, true );

        return noErr;
    }

    // other
    return eventNotHandledErr;
}
