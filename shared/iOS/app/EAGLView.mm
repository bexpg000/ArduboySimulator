//
//  EAGLView.m
//
//  Created by Seth Robinson on 3/6/09.
//  For license info, check the license.txt file that should have come with this.
//

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import "EAGLView.h"
#import "MyAppDelegate.h"

#define USE_DEPTH_BUFFER 1

// A class extension to declare private methods
@interface EAGLView ()

@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) NSTimer *animationTimer;

- (BOOL) createFramebuffer;
- (void) destroyFramebuffer;

@end


@implementation EAGLView

@synthesize context;
@synthesize animationTimer;
@synthesize animationInterval;
@synthesize animationIntervalSave;


// You must implement this
+ (Class)layerClass {
	return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder
{

	if ((self = [super initWithCoder:coder]))
	{
		
		InitDeviceScreenInfo();
		// Get the layer
		CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
		
		eaglLayer.opaque = YES;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
		   [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
		
		if (!context || ![EAGLContext setCurrentContext:context])
		{
			[self release];
			return nil;
		}
		
		animationIntervalSave = 1.0/60.0;
		animationInterval = animationIntervalSave;
	}
	
	
	if (!GetBaseApp()->Init())
	{
		
		NSLog(@"Couldn't init app");
		[self release];
		return nil;
	}
	return self;
}

- (void)onKill
{
	GetBaseApp()->Kill();
}

- (void)drawView 
{
		
	if (!viewRenderbuffer) return;
		
	[EAGLContext setCurrentContext:context];

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	if (GetBaseApp()->GetManualRotationMode())
	{
		glViewport(0, 0, GetPrimaryGLX(), GetPrimaryGLY()); //a trick for extra speed

	} else
	{
		glViewport(0, 0, GetScreenSizeX(), GetScreenSizeY());
		
	}
		
	GetBaseApp()->Update();
	GetBaseApp()->Draw();

	while (!GetBaseApp()->GetOSMessages()->empty())
	{
		OSMessage m = GetBaseApp()->GetOSMessages()->front();
		GetBaseApp()->GetOSMessages()->pop_front();
		
		LogMsg("Got OS message %d, %s", m.m_type, m.m_string.c_str());
		MyAppDelegate *appDelegate = (MyAppDelegate *)[[UIApplication sharedApplication] delegate];
		
		[appDelegate onOSMessage: &m];
	}
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	// if(main_throttled_update()) 
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
}


- (void)layoutSubviews
{
	[EAGLContext setCurrentContext:context];
	[self destroyFramebuffer];
	[self createFramebuffer];
	[self drawView];
}

- (BOOL)createFramebuffer 
{
	
	if (IsIPAD())
	{
		
		NSLog(@"iPad detected");	
	}
	
	if (IsIphone4())
	{
		NSLog(@"iPhone4 detected");
		self.contentScaleFactor = 2.0;	
	}
	
	glGenFramebuffersOES(1, &viewFramebuffer);
	glGenRenderbuffersOES(1, &viewRenderbuffer);
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
	
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
	
	if (USE_DEPTH_BUFFER) 
	{
		glGenRenderbuffersOES(1, &depthRenderbuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
		glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
	}

	if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		return NO;
	}
	
	return YES;
}


- (void)destroyFramebuffer
{

	
	glDeleteFramebuffersOES(1, &viewFramebuffer);
	viewFramebuffer = 0;
	glDeleteRenderbuffersOES(1, &viewRenderbuffer);
	viewRenderbuffer = 0;
	
	if(depthRenderbuffer)
	{
		glDeleteRenderbuffersOES(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}
}


- (void)startAnimation 
{
	self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(drawView) userInfo:nil repeats:YES];
}


- (void)stopAnimation 
{
	self.animationTimer = nil;
}


- (void)setAnimationTimer:(NSTimer *)newTimer
{
	[animationTimer invalidate];
	animationTimer = newTimer;
}


- (void)setAnimationInterval:(NSTimeInterval)interval 
{
	
	animationInterval = interval;
	
	if (animationTimer)
	{
		[self stopAnimation];
		[self startAnimation];
	}
}

- (void)setAnimationIntervalSave:(NSTimeInterval)interval 
{
	
	animationIntervalSave = interval;

	[self setAnimationInterval:animationIntervalSave];
}

- (void)dealloc 
{

	[self stopAnimation];
	
	if ([EAGLContext currentContext] == context) 
	{
		[EAGLContext setCurrentContext:nil];
	}
	
	[context release];	
	[super dealloc];
}

// Handles the start of a touch
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  	// Enumerate through all the touch objects.
	NSUInteger touchCount = 0;
	for (UITouch *touch in touches)
	{
		CGPoint pt =[touch locationInView:self];
		ConvertCoordinatesIfRequired(pt.x, pt.y);
		GetMessageManager()->SendGUI(MESSAGE_TYPE_GUI_CLICK_START,pt.x, pt.y);			
		touchCount++;  
	}	
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  	// Enumerate through all the touch objects.
	NSUInteger touchCount = 0;
	for (UITouch *touch in touches)
	{
		CGPoint pt =[touch locationInView:self];
		ConvertCoordinatesIfRequired(pt.x, pt.y);
		GetMessageManager()->SendGUI(MESSAGE_TYPE_GUI_CLICK_END,pt.x, pt.y);
		touchCount++;  
	}	
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
   // Enumerate through all the touch objects.
	NSUInteger touchCount = 0;
	for (UITouch *touch in touches)
	{
		CGPoint pt =[touch locationInView:self];
		ConvertCoordinatesIfRequired(pt.x, pt.y);
		GetMessageManager()->SendGUI(MESSAGE_TYPE_GUI_CLICK_MOVE,pt.x, pt.y);
		touchCount++;  
	}	
}


@end
