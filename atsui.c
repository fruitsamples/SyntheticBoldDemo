/*

File: atsui.c

Abstract: Main drawing code for SyntheticBoldDemo project. Text is 
drawn twice, once with and once without the synthetic bold. See
comments below for more detail. Also note this technique will work
on any CG-based text drawing code. ATSUI is not a requirement. In
this example, the API TXNDrawUnicodeTextBox is used, instead of
ATSUDrawText.

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

Copyright © 2004-2007 Apple Inc., All Rights Reserved

*/ 

#include "globals.h"
#include "atsui.h"

// Globals for just this source module
//
static ATSUStyle			gStyle = NULL;
static CFStringRef			gString = NULL;
static UniChar				*gText = NULL;
static UniCharCount			gLength = 0;
static Fixed				gPointSize;
static ATSUFontID			gFont = 0;


// Sets the font
//
void SetATSUIStuffFont(ATSUFontID inFont)
{
    gFont = inFont;
}


// Sets the font size
//
void SetATSUIStuffFontSize(Fixed inSize)
{
    gPointSize = inSize;
}


// Updates the ATSUI style to the current font and size
//
void UpdateATSUIStyle(void)
{
    ATSUAttributeTag		tags[2];
    ByteCount				sizes[2];
    ATSUAttributeValuePtr	values[2];

    tags[0] = kATSUFontTag;
    sizes[0] = sizeof(ATSUFontID);
    values[0] = &gFont;
    
    tags[1] = kATSUSizeTag;
    sizes[1] = sizeof(Fixed);
    values[1] = &gPointSize;
    
    verify_noerr( ATSUSetAttributes(gStyle, 2, tags, sizes, values) );
}


// Sets up the text based on the specified CFString
//
void UpdateATSUIStuffString(CFStringRef string)
{
    free(gText);
    gLength = CFStringGetLength(string);
    gText = (UniChar *)malloc(gLength * sizeof(UniChar));
    CFStringGetCharacters(string, CFRangeMake(0, gLength), gText);
}


// Creates the ATSUI data
//
void SetUpATSUIStuff(void)
{    
    CFStringRef	string = CFSTR("Hello World!");
	UpdateATSUIStuffString(string);
	
    verify_noerr( ATSUCreateStyle(&gStyle) );
    UpdateATSUIStyle();
    
    gLength = CFStringGetLength(string);
    gText = (UniChar *)malloc(gLength * sizeof(UniChar));
    CFStringGetCharacters(string, CFRangeMake(0, gLength), gText);
}


// Sets kATSUQDBoldfaceTag for the given style
//
void MySetBoldfaceTag(ATSUStyle iStyle)
{
    Boolean					setToTrue = true;
    ATSUAttributeTag		tag = kATSUQDBoldfaceTag;
    ByteCount				size = sizeof(Boolean);
    ATSUAttributeValuePtr	value = (ATSUAttributeValuePtr) &setToTrue;
    
    verify_noerr( ATSUSetAttributes(iStyle, 1, &tag, &size, &value) );
}


// Sets kATSUQDBoldfaceTag for the given style
//
void MyClearBoldfaceTag(ATSUStyle iStyle)
{
    ATSUAttributeTag		tag = kATSUQDBoldfaceTag;

    verify_noerr( ATSUClearAttributes(iStyle, 1, &tag) );
}


// Checks the global preference to see if text at the specified point size
// will be drawn antialiased or not.
//
// Note that iSize is of type Fixed!
//
Boolean IsAntiAliased(Fixed iSize)
{
    Boolean					keyExistsAndHasValidFormat;
    CFIndex					value;

    value = CFPreferencesGetAppIntegerValue(CFSTR("AppleAntiAliasingThreshold"), kCFPreferencesCurrentApplication, &keyExistsAndHasValidFormat);

    if (keyExistsAndHasValidFormat)
        return (Fix2X(iSize) > value); // 'value' is the maximum not-antialiasing size
    else
        return true;
}


void DrawATSUIStuff(CGContextRef inContext, HIRect bounds)
{
    float								windowHeight, windowWidth, quarter;
    Boolean								needToUseCGStrokeMethod;
	HIRect								box1, box2;
	HIThemeTextInfo						textInfo = { 0 };
	ATSUTextLayout						layout;
	ATSUAttributeTag					tags[3];
	ByteCount							sizes[3];
	ATSUAttributeValuePtr				values[3];
	Fixed								flush;
	ATSUTextMeasurement					width;
	
    // Divide the window into vertical quarters, and draw the text in the middle two quarters
    windowHeight = bounds.size.height;
	windowWidth = bounds.size.width;
    quarter = windowHeight / 4.0;	// 1/4 window height
	
	// Set up box 1
	box1 = bounds;
	box1.origin.y += ((windowHeight / 4.0) * 2.0);
	box1.size.height -= (windowHeight / 4.0);
	CGContextStrokeRect(inContext, box1);
	
	// Set up box 2
	box2 = bounds;
	box2.origin.y += (windowHeight / 4.0);
	box2.size.height -= ((windowHeight / 4.0) * 2.0);
	CGContextStrokeRect(inContext, box2);

	// Create an ATSUI Layout object
	verify_noerr( ATSUCreateTextLayout(&layout) );
	
	// Attatch text to layout
	verify_noerr( ATSUSetTextPointerLocation(layout, gText, kATSUFromTextBeginning, kATSUToTextEnd, gLength) );
	
	// Combine the ATSU Style and Layout together
	verify_noerr( ATSUSetRunStyle(layout, gStyle, kATSUFromTextBeginning, kATSUToTextEnd) );
	
	// Add CGContext to ATSU object
	tags[0] = kATSUCGContextTag;
	sizes[0] = sizeof(CGContextRef);
	values[0] = &inContext;
	
	// Make the text centered...
	flush = kATSUCenterAlignment;
	tags[1] = kATSULineFlushFactorTag;
	sizes[1] = sizeof(flush);
	values[1] = &flush;
	
	// Within the width of the box
	width = X2Fix(bounds.size.width);
	tags[2] = kATSULineWidthTag;
	sizes[2] = sizeof(ATSUTextMeasurement);
	values[2] = &width;
	
	verify_noerr( ATSUSetLayoutControls(layout, 3, tags, sizes, values) );
	
    // Draw the text once without the extra bold	
	verify_noerr(ATSUDrawText(layout, kATSUFromTextBeginning, kATSUToTextEnd, X2Fix(box1.origin.x), X2Fix((box1.origin.y + box1.size.height) / 2.0)));

    needToUseCGStrokeMethod = gCurrentlyPrinting || IsAntiAliased(gPointSize);
    if ( needToUseCGStrokeMethod )
	{
        CGContextSaveGState(inContext);
        CGContextSetTextDrawingMode(inContext, kCGTextFillStroke);
        CGContextSetLineWidth(inContext, gStrokeThicknessFactor * Fix2X(gPointSize));
        // You might want to call CGContextSetStrokeColor() here,
        // just to make certain it is the same as the text/fill color.
    }
    else
        MySetBoldfaceTag(gStyle); // This will look very strong on-screen when CG anti-aliasing is off

    // Draw the text again with the extra bold for comparison
	verify_noerr( ATSUDrawText(layout, kATSUFromTextBeginning, kATSUToTextEnd, X2Fix(box2.origin.x), X2Fix((box2.origin.y + box2.size.height) / 2.0)) );

    // Undo the previous CG text mode setting
    if ( needToUseCGStrokeMethod )
        CGContextRestoreGState(inContext);
    else
        MyClearBoldfaceTag(gStyle);

    // Tear down the CGContext since we are done with it
	CGContextFlush(inContext);
}


// Disposes of the ATSUI data
//
void DisposeATSUIStuff(void)
{
    verify_noerr( ATSUDisposeStyle(gStyle) );
    free(gText);
}
