// PokeMacGochi: the game's round window on a Mac desktop. Only the window; the game
// and everything else live in host/ (see host/host.h).
#import <AppKit/AppKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <QuartzCore/QuartzCore.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include "host.h"
#include "shim/Preferences.h"  // the save folder's storage, as the game uses it

#define APP_NAME "PokeMacGochi"
// About box
#define AUTHOR_EMAIL "fedor@3dprintsos.com"
#define GITHUB_URL "https://github.com/FedorSosnin/PokeKnomiGochi"
#define YOUTUBE_URL "https://youtube.com/3dprintsos"
#define TICK_S (1.0 / 60.0)
// The Pokeball frame around the screen, in points. Only the frame drags the window: the screen is all
// touch. With the mouse over the window it grows outward to RING_HOVER, easier to grab; the window keeps
// a clear margin of RING_PAD for that, which clicks pass through.
#define RING 6.0
#define RING_HOVER 12.0
#define RING_PAD (RING_HOVER - RING)
#define RING_ANIM_S 0.15  // the grow and shrink
#define RING_OUTLINE 1.0
#define RING_LINE 20.0  // the black band in the middle
static const CGFloat kSizes[] = {190, 240, 300};  // side of the window at rest, frame included
#define DEFAULT_SIZE 240.0

// New versions: the newest "mac-v*" release on GitHub. Checked at launch and once a day; the
// player is told once per version and can always check from the menu. The save lives outside
// the app, so replacing it with the new DMG keeps the pet.
#define RELEASES_API "https://api.github.com/repos/FedorSosnin/PokeKnomiGochi/releases?per_page=30"
#define UPDATE_EVERY_S (24 * 60 * 60)
static NSString *const kPrefToldVersion = @"updateToldVersion";
static NSString *gNewVersion = nil;  // newer than this app, once a check found one
static NSURL *gNewURL = nil;         // its DMG (or the release page)

// ---- the buddy relay (relay/): Knomis and Macs meet on the Wi-Fi by themselves; the relay adds
// the browsers, which can't. One WebSocket, retried with a growing pause when it drops. ----
static NSURLSessionWebSocketTask *gRelay = nil;
static int gRelayTries = 0;
static void relaySend(const uint8_t *data, int len) {
  NSURLSessionWebSocketMessage *m = [[NSURLSessionWebSocketMessage alloc] initWithData:[NSData dataWithBytes:data length:len]];
  [gRelay sendMessage:m completionHandler:^(NSError *e) {}];
}
static void relayConnect(void);
static void relayDropped(NSURLSessionWebSocketTask *task) {
  if (task != gRelay) return;
  gRelay = nil;
  hostRelayConnected(NULL);
  double wait = MIN(60.0, 2.0 * (1 << MIN(gRelayTries++, 5)));
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(wait * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ relayConnect(); });
}
static void relayReceive(NSURLSessionWebSocketTask *task) {
  [task receiveMessageWithCompletionHandler:^(NSURLSessionWebSocketMessage *m, NSError *err) {
    dispatch_async(dispatch_get_main_queue(), ^{
      if (task != gRelay) return;
      if (err || !m) { relayDropped(task); return; }
      if (m.type == NSURLSessionWebSocketMessageTypeString) {  // {"n": others}: it also says we are in
        NSDictionary *j = [NSJSONSerialization JSONObjectWithData:[m.string dataUsingEncoding:NSUTF8StringEncoding] options:0 error:nil];
        gRelayTries = 0;
        hostRelayConnected(relaySend);
        if ([j isKindOfClass:NSDictionary.class]) hostRelayPeers([j[@"n"] intValue]);
      } else {
        hostRelayReceived((const uint8_t *)m.data.bytes, (int)m.data.length);
      }
      relayReceive(task);
    });
  }];
}
static void relayConnect(void) {
  static bool pinging = false;
  if (!pinging) {  // keeps a quiet connection open; the relay answers "pong" without waking up
    pinging = true;
    NSTimer *t = [NSTimer timerWithTimeInterval:45 repeats:YES block:^(NSTimer *timer) {
      [gRelay sendMessage:[[NSURLSessionWebSocketMessage alloc] initWithString:@"ping"] completionHandler:^(NSError *e) {}];
    }];
    [NSRunLoop.mainRunLoop addTimer:t forMode:NSRunLoopCommonModes];
  }
  if (gRelay) return;
  // the home's shared IPv4: a Mac connecting over IPv6 would otherwise not meet the Knomi on its Wi-Fi
  NSURLRequest *v4req = [NSURLRequest requestWithURL:[NSURL URLWithString:@HOST_RELAY_V4_URL] cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:8];
  [[NSURLSession.sharedSession dataTaskWithRequest:v4req completionHandler:^(NSData *d, NSURLResponse *r, NSError *e) {
    NSString *v4 = d ? [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding] : @"";
    v4 = [v4 stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    bool ok = [v4 rangeOfString:@"^[0-9]{1,3}(\\.[0-9]{1,3}){3}$" options:NSRegularExpressionSearch].location != NSNotFound;
    NSString *url = ok ? [NSString stringWithFormat:@"%s?v4=%@", HOST_RELAY_URL, v4] : @HOST_RELAY_URL;
    dispatch_async(dispatch_get_main_queue(), ^{
      if (gRelay) return;
      gRelay = [NSURLSession.sharedSession webSocketTaskWithURL:[NSURL URLWithString:url]];
      [gRelay resume];
      relayReceive(gRelay);
    });
  }] resume];
}

static bool gConsole = false;  // --console: launched for testing
static bool gOwnSave = true;   // false with POKEMACGOCHI_SAVE_DIR: another save than the user's
static char **gArgv;

// The window's size and place live in the save folder (namespace "mac"), next to the game they
// belong to: a copy launched on another save (tests, POKEMACGOCHI_SAVE_DIR) can't move this one.
#define PREFS_NS "mac"
static NSString *const kPrefSize = @"windowSize";  // where 1.0 kept them, in the app's user defaults
static NSString *const kPrefOriginX = @"windowX";
static NSString *const kPrefOriginY = @"windowY";

// ---- sound: requests PCM from the host layer on the audio thread ----
#define AUDIO_BUFFERS 3
#define AUDIO_FRAMES 512  // ~32 ms per buffer at 16 kHz

static void audioCallback(void *, AudioQueueRef q, AudioQueueBufferRef buf) {
  hostAudioRender((int16_t *)buf->mAudioData, AUDIO_FRAMES);
  buf->mAudioDataByteSize = AUDIO_FRAMES * sizeof(int16_t);
  AudioQueueEnqueueBuffer(q, buf, 0, NULL);
}

static void startAudio(void) {
  AudioStreamBasicDescription fmt = {};
  fmt.mSampleRate = HOST_AUDIO_RATE;
  fmt.mFormatID = kAudioFormatLinearPCM;
  fmt.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  fmt.mChannelsPerFrame = 1;
  fmt.mFramesPerPacket = 1;
  fmt.mBitsPerChannel = 16;
  fmt.mBytesPerFrame = fmt.mBytesPerPacket = sizeof(int16_t);
  AudioQueueRef q;
  if (AudioQueueNewOutput(&fmt, audioCallback, NULL, NULL, NULL, 0, &q) != noErr) {
    NSLog(@"no audio output: the game continues without sound");
    return;
  }
  for (int i = 0; i < AUDIO_BUFFERS; i++) {
    AudioQueueBufferRef buf;
    if (AudioQueueAllocateBuffer(q, AUDIO_FRAMES * sizeof(int16_t), &buf) == noErr) audioCallback(NULL, q, buf);
  }
  AudioQueueStart(q, NULL);
}

// ---- window ----
@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, NSMenuDelegate>
- (void)checkForUpdates:(id)sender;
@end

@interface RoundWindow : NSWindow
@end
@implementation RoundWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

@interface GameView : NSView
- (void)showFrame;
- (void)saveWindowPlace;
- (void)fillMenu:(NSMenu *)menu;
@end

// the window at rest (what the size menu and the saved place mean) and the window with its margin
static NSRect restFrame(NSRect windowFrame) { return NSInsetRect(windowFrame, RING_PAD, RING_PAD); }
static NSRect windowFrameFor(NSRect rest) { return NSInsetRect(rest, -RING_PAD, -RING_PAD); }

// Over the screen the cursor is a finger, as on the Knomi; while the button is down the finger
// presses: the same hand a little lower and to the right, on the same hotspot
static NSCursor *fingerCursor(bool pressed) {
  static NSCursor *up, *down;
  if (!up) {
    NSCursor *hand = NSCursor.pointingHandCursor;
    NSSize sz = hand.image.size;
    NSSize big = NSMakeSize(sz.width + 1, sz.height + 2);
    NSImage *(^make)(CGFloat, CGFloat) = ^(CGFloat dx, CGFloat dy) {
      return [NSImage imageWithSize:big flipped:NO drawingHandler:^BOOL(NSRect r) {
        [hand.image drawInRect:NSMakeRect(dx, dy, sz.width, sz.height)];
        return YES;
      }];
    };
    up = [[NSCursor alloc] initWithImage:make(0, 2) hotSpot:hand.hotSpot];
    down = [[NSCursor alloc] initWithImage:make(1, 0) hotSpot:hand.hotSpot];
  }
  return pressed ? down : up;
}

// The frame: a Pokeball with the game screen inside. Grab it to move the window.
@interface RingView : NSView
- (void)setHover:(bool)hover;
@end

@implementation RingView {
  CGFloat _ring;  // the frame's width right now: RING at rest, RING_HOVER with the mouse over it
  bool _hover;
  NSTimer *_anim;
}

- (instancetype)initWithFrame:(NSRect)frame {
  if ((self = [super initWithFrame:frame])) _ring = RING;
  return self;
}

// the ball's circle right now, in this view
- (CGFloat)radius { return self.bounds.size.width / 2 - (RING_HOVER - _ring); }
- (CGFloat)distance:(NSEvent *)event {
  NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
  return hypot(p.x - NSMidX(self.bounds), p.y - NSMidY(self.bounds));
}

- (void)setHover:(bool)hover {
  if (hover == _hover) return;
  _hover = hover;
  if (_anim) return;
  _anim = [NSTimer scheduledTimerWithTimeInterval:1.0 / 60 repeats:YES block:^(NSTimer *t) {
    CGFloat target = self->_hover ? RING_HOVER : RING, step = (RING_HOVER - RING) / (RING_ANIM_S * 60);
    self->_ring = self->_ring < target ? fmin(target, self->_ring + step) : fmax(target, self->_ring - step);
    self.needsDisplay = YES;
    [self.window invalidateShadow];
    if (self->_ring == target) {
      [t invalidate];
      self->_anim = nil;
    }
  }];
  [NSRunLoop.currentRunLoop addTimer:_anim forMode:NSRunLoopCommonModes];  // also while dragging
}

- (void)drawRect:(NSRect)dirty {
  NSRect b = NSInsetRect(self.bounds, RING_HOVER - _ring, RING_HOVER - _ring);
  CGFloat mid = NSMidY(b);
  [NSGraphicsContext saveGraphicsState];
  [[NSBezierPath bezierPathWithOvalInRect:b] addClip];
  [NSColor.whiteColor setFill];
  NSRectFill(NSMakeRect(NSMinX(b), NSMinY(b), b.size.width, mid - NSMinY(b)));
  [[NSColor colorWithSRGBRed:0.87 green:0.13 blue:0.13 alpha:1] setFill];
  NSRectFill(NSMakeRect(NSMinX(b), mid, b.size.width, NSMaxY(b) - mid));
  [NSColor.blackColor setFill];
  NSRectFill(NSMakeRect(NSMinX(b), mid - RING_LINE / 2, b.size.width, RING_LINE));
  [NSGraphicsContext restoreGraphicsState];
  // the ball's outline: without it, the white half gets lost against a light background
  NSBezierPath *edge = [NSBezierPath bezierPathWithOvalInRect:NSInsetRect(b, RING_OUTLINE / 2, RING_OUTLINE / 2)];
  edge.lineWidth = RING_OUTLINE;
  [NSColor.blackColor setStroke];
  [edge stroke];
}

// outside the circle the window does not exist: the click passes through to whatever is underneath
- (NSView *)hitTest:(NSPoint)point {
  NSPoint p = [self.superview convertPoint:point toView:self];
  if (hypot(p.x - NSMidX(self.bounds), p.y - NSMidY(self.bounds)) > [self radius]) return nil;
  return [super hitTest:point] ?: self;
}

- (void)mouseDown:(NSEvent *)event {
  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];
  [self.window makeKeyWindow];
  [NSCursor.closedHandCursor set];
  [self.window performWindowDragWithEvent:event];
  [(GameView *)self.subviews.firstObject saveWindowPlace];
}

- (void)rightMouseDown:(NSEvent *)event { [self.subviews.firstObject rightMouseDown:event]; }
// over the ball it grows; the hand only on the frame, the screen is for touching
- (void)mouseMoved:(NSEvent *)event {
  CGFloat d = [self distance:event];
  [self setHover:d <= [self radius]];
  CGFloat screen = self.bounds.size.width / 2 - RING_HOVER;
  if (d <= screen) [fingerCursor(NSEvent.pressedMouseButtons & 1) set];
  else if (d <= [self radius]) [NSCursor.openHandCursor set];
  else [NSCursor.arrowCursor set];
}
- (void)mouseEntered:(NSEvent *)event { [self mouseMoved:event]; }
- (void)mouseExited:(NSEvent *)event {
  [self setHover:false];
  [NSCursor.arrowCursor set];
}
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }
@end

@implementation GameView {
  uint8_t *_rgba;
  bool _left, _right, _up, _down, _touching;
}

- (instancetype)initWithFrame:(NSRect)frame {
  if ((self = [super initWithFrame:frame])) {
    _rgba = (uint8_t *)malloc(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4);
    self.wantsLayer = YES;
    self.layer.masksToBounds = YES;
    self.layer.contentsGravity = kCAGravityResize;
    self.layer.minificationFilter = kCAFilterTrilinear;
    self.layer.magnificationFilter = kCAFilterNearest;
    self.layer.backgroundColor = NSColor.blackColor.CGColor;
    [self addTrackingArea:[[NSTrackingArea alloc]
                              initWithRect:NSZeroRect
                                   options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                                           NSTrackingActiveAlways | NSTrackingInVisibleRect
                                     owner:self
                                  userInfo:nil]];
  }
  return self;
}

- (void)layout {
  [super layout];
  self.layer.cornerRadius = self.bounds.size.width / 2;
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }  // the first click is already a touch

- (void)showFrame {
  hostFrameRGBA(_rgba);
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef prov = CGDataProviderCreateWithData(NULL, _rgba, HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4, NULL);
  CGImageRef img = CGImageCreate(HOST_CANVAS_SIZE, HOST_CANVAS_SIZE, 8, 32, HOST_CANVAS_SIZE * 4, cs,
                                 kCGBitmapByteOrderDefault | kCGImageAlphaNoneSkipLast, prov, NULL, false,
                                 kCGRenderingIntentDefault);
  // a copy so the layer does not depend on the buffer that the next frame overwrites
  CGImageRef owned = CGImageCreateCopy(img);
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  self.layer.contents = (__bridge id)owned;
  [CATransaction commit];
  CGImageRelease(owned);
  CGImageRelease(img);
  CGDataProviderRelease(prov);
  CGColorSpaceRelease(cs);
}

// event point in canvas coordinates
- (NSPoint)canvasPoint:(NSEvent *)event {
  NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
  CGFloat k = HOST_CANVAS_SIZE / self.bounds.size.width;
  return NSMakePoint(p.x * k, (self.bounds.size.height - p.y) * k);
}


// inside the frame, whatever falls outside the circle belongs to the frame
- (NSView *)hitTest:(NSPoint)point {
  NSPoint p = [self.superview convertPoint:point toView:self];
  CGFloat r = self.bounds.size.width / 2;
  return hypot(p.x - r, p.y - r) <= r ? self : nil;
}

- (void)mouseDown:(NSEvent *)event {
  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];
  [self.window makeKeyWindow];
  if (event.modifierFlags & NSEventModifierFlagControl) {
    [self rightMouseDown:event];
    return;
  }
  NSPoint p = [self canvasPoint:event];
  _touching = true;
  [fingerCursor(true) set];
  hostPointer((int)p.x, (int)p.y, true);
}

- (void)mouseDragged:(NSEvent *)event {
  if (!_touching) return;
  NSPoint p = [self canvasPoint:event];
  hostPointer((int)p.x, (int)p.y, true);
}

- (void)mouseUp:(NSEvent *)event {
  if (!_touching) return;
  _touching = false;
  [fingerCursor(false) set];
  NSPoint p = [self canvasPoint:event];
  hostPointer((int)p.x, (int)p.y, false);
}

- (void)mouseEntered:(NSEvent *)event { hostPointerHover(); }
- (void)mouseMoved:(NSEvent *)event { hostPointerHover(); }  // the frame's tracking sets the cursor

// The same menu on right-click and in the menu bar
- (void)fillMenu:(NSMenu *)menu {
  [menu removeAllItems];
  menu.autoenablesItems = NO;
  NSMenuItem *about = [menu addItemWithTitle:@"About " APP_NAME action:@selector(showAbout:) keyEquivalent:@""];
  about.target = self;
  [menu addItem:NSMenuItem.separatorItem];
  NSMenuItem *show = [menu addItemWithTitle:self.window.isVisible ? @"Hide " APP_NAME : @"Show " APP_NAME
                                     action:@selector(toggleWindow:)
                              keyEquivalent:@""];
  show.target = self;
  NSMenuItem *sizes = [menu addItemWithTitle:@"Size" action:nil keyEquivalent:@""];
  NSMenu *sub = [[NSMenu alloc] init];
  NSArray *names = @[ @"Small", @"Medium", @"Large" ];
  for (int i = 0; i < 3; i++) {
    NSMenuItem *it = [sub addItemWithTitle:names[i] action:@selector(pickSize:) keyEquivalent:@""];
    it.target = self;
    it.tag = (NSInteger)kSizes[i];
    it.state = restFrame(self.window.frame).size.width == kSizes[i] ? NSControlStateValueOn : NSControlStateValueOff;
  }
  sizes.submenu = sub;
  // the same setting as the game's SND button on the clock screen, saved with the game
  NSMenuItem *mute = [menu addItemWithTitle:@"Mute Sound" action:@selector(toggleMute:) keyEquivalent:@""];
  mute.target = self;
  mute.state = hostSoundEnabled() ? NSControlStateValueOff : NSControlStateValueOn;
  NSString *upd = gNewVersion ? [NSString stringWithFormat:@"Download Version %@…", gNewVersion] : @"Check for Updates…";
  NSMenuItem *update = [menu addItemWithTitle:upd action:@selector(checkForUpdates:) keyEquivalent:@""];
  update.target = NSApp.delegate;
  [menu addItem:NSMenuItem.separatorItem];
  [menu addItemWithTitle:@"Drag the ring to move" action:nil keyEquivalent:@""].enabled = NO;
  [menu addItemWithTitle:@"\u2190 \u2192 tilt  \u00b7  Space: wild battle" action:nil keyEquivalent:@""].enabled = NO;
  [menu addItem:NSMenuItem.separatorItem];
  NSMenuItem *quit = [menu addItemWithTitle:@"Quit " APP_NAME action:@selector(terminate:) keyEquivalent:@"q"];
  quit.target = NSApp;
}

- (void)rightMouseDown:(NSEvent *)event {
  NSMenu *menu = [[NSMenu alloc] init];
  [self fillMenu:menu];
  [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)toggleMute:(id)sender { hostSetSoundEnabled(!hostSoundEnabled()); }

static NSAttributedString *aboutLine(NSString *label, NSString *text, NSString *url) {
  NSMutableAttributedString *s = [[NSMutableAttributedString alloc] initWithString:label];
  [s appendAttributedString:[[NSAttributedString alloc] initWithString:text attributes:@{NSLinkAttributeName : [NSURL URLWithString:url]}]];
  [s appendAttributedString:[[NSAttributedString alloc] initWithString:@"\n"]];
  return s;
}

- (void)showAbout:(id)sender {
  NSMutableAttributedString *credits = [[NSMutableAttributedString alloc] init];
  [credits appendAttributedString:aboutLine(@"By ", @AUTHOR_EMAIL, @"mailto:" AUTHOR_EMAIL)];
  [credits appendAttributedString:aboutLine(@"", @"PokeKnomiGochi on GitHub", @GITHUB_URL)];
  if (strlen(YOUTUBE_URL)) [credits appendAttributedString:aboutLine(@"", @"YouTube channel", @YOUTUBE_URL)];
  [credits appendAttributedString:[[NSAttributedString alloc]
                                      initWithString:@"\nBased on TamaPoke by Quique Tortosa. Sprites from PMDCollab (CC BY-NC). "
                                                     @"Pok\u00e9mon \u00a9 Nintendo / Game Freak / The Pok\u00e9mon Company."]];
  NSMutableParagraphStyle *centered = [[NSMutableParagraphStyle alloc] init];
  centered.alignment = NSTextAlignmentCenter;
  [credits addAttributes:@{NSParagraphStyleAttributeName : centered, NSFontAttributeName : [NSFont systemFontOfSize:11]}
                   range:NSMakeRange(0, credits.length)];
  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];
  [NSApp orderFrontStandardAboutPanelWithOptions:@{NSAboutPanelOptionCredits : credits}];
  // the round window floats: the panel has to as well, or it opens behind it. And on the pet's screen.
  NSRect screen = (self.window.screen ?: NSScreen.mainScreen).visibleFrame;
  for (NSWindow *w in NSApp.windows)
    if (![w isKindOfClass:RoundWindow.class] && w.isVisible) {
      w.level = NSFloatingWindowLevel + 1;
      [w setFrameOrigin:NSMakePoint(NSMidX(screen) - w.frame.size.width / 2, NSMidY(screen) - w.frame.size.height / 2)];
    }
}

// while hidden it stays alive: the game keeps running, like a Knomi with the screen off
- (void)toggleWindow:(id)sender {
  if (self.window.isVisible) {
    [self.window orderOut:nil];
  } else {
    [self.window makeKeyAndOrderFront:nil];
    if (!gConsole) [NSApp activateIgnoringOtherApps:YES];
  }
}

- (void)pickSize:(NSMenuItem *)item {
  NSRect f = self.window.frame;
  CGFloat s = item.tag;
  NSPoint c = NSMakePoint(NSMidX(f), NSMidY(f));
  [self.window setFrame:windowFrameFor(NSMakeRect(c.x - s / 2, c.y - s / 2, s, s)) display:YES];
  [self.window invalidateShadow];
  [self saveWindowPlace];
}

- (void)saveWindowPlace {
  NSRect f = restFrame(self.window.frame);
  Preferences p;
  p.begin(PREFS_NS);
  p.putInt("size", (int32_t)lround(f.size.width));
  p.putInt("x", (int32_t)lround(f.origin.x));
  p.putInt("y", (int32_t)lround(f.origin.y));
  p.end();
}

- (void)applyKeys {
  hostTiltKeys(_left, _right);
  hostArrowKeys(_up, _down);
}

- (bool)handleKey:(NSEvent *)event down:(bool)down {
  switch (event.keyCode) {
    case 123: _left = down; [self applyKeys]; return true;   // left arrow
    case 124: _right = down; [self applyKeys]; return true;  // right arrow
    case 126: _up = down; [self applyKeys]; return true;     // up arrow (PokeDungeon)
    case 125: _down = down; [self applyKeys]; return true;   // down arrow
    case 49: hostButton(down); return true;                  // space
  }
  return false;
}

- (void)keyDown:(NSEvent *)event {
  if (event.isARepeat) return;
  if (![self handleKey:event down:true]) [super keyDown:event];
}

- (void)keyUp:(NSEvent *)event {
  if (![self handleKey:event down:false]) [super keyUp:event];
}

// without focus keyUp never arrives: the key would stay pressed forever
- (void)releaseKeys {
  _left = _right = _up = _down = false;
  [self applyKeys];
  hostButton(false);
}

- (void)dealloc { free(_rgba); }
@end

// ---- application ----

@implementation AppDelegate {
  RoundWindow *_window;
  GameView *_view;
  id _activity;
  NSStatusItem *_statusItem;
}

// Menu bar icon: a template Pokeball, so that it follows the system theme
static NSImage *statusIcon(void) {
  NSImage *img = [NSImage imageWithSize:NSMakeSize(18, 18) flipped:NO drawingHandler:^BOOL(NSRect r) {
    NSRect ball = NSInsetRect(r, 1.5, 1.5);
    NSBezierPath *edge = [NSBezierPath bezierPathWithOvalInRect:ball];
    edge.lineWidth = 1.5;
    [edge stroke];
    [[NSBezierPath bezierPathWithRect:NSMakeRect(NSMinX(ball), NSMidY(r) - 0.75, ball.size.width, 1.5)] fill];
    // the top half filled, and the button in the center
    NSBezierPath *top = [NSBezierPath bezierPath];
    [top appendBezierPathWithArcWithCenter:NSMakePoint(NSMidX(r), NSMidY(r)) radius:ball.size.width / 2 startAngle:0 endAngle:180];
    [top closePath];
    [top fill];
    [NSGraphicsContext.currentContext setCompositingOperation:NSCompositingOperationClear];
    [[NSBezierPath bezierPathWithOvalInRect:NSInsetRect(r, 6, 6)] fill];
    [NSGraphicsContext.currentContext setCompositingOperation:NSCompositingOperationSourceOver];
    [[NSBezierPath bezierPathWithOvalInRect:NSInsetRect(r, 7.5, 7.5)] fill];
    return YES;
  }];
  [img setTemplate:YES];
  return img;
}

- (void)menuNeedsUpdate:(NSMenu *)menu { [_view fillMenu:menu]; }

static NSRect initialFrame(bool ownSave) {
  Preferences p;
  p.begin(PREFS_NS, true);
  bool known = p.isKey("x");
  CGFloat size = p.getInt("size", 0), x = p.getInt("x", 0), y = p.getInt("y", 0);
  p.end();
  // first launch after 1.0: take the place it kept in the user defaults (only for the real save)
  NSUserDefaults *d = NSUserDefaults.standardUserDefaults;
  if (!known && ownSave && [d objectForKey:kPrefOriginX]) {
    known = true;
    size = [d doubleForKey:kPrefSize];
    x = [d doubleForKey:kPrefOriginX];
    y = [d doubleForKey:kPrefOriginY];
  }
  if (size < kSizes[0] || size > kSizes[2]) size = DEFAULT_SIZE;
  if (known) {
    NSRect f = NSMakeRect(x, y, size, size);
    for (NSScreen *s in NSScreen.screens)  // a monitor that is no longer there does not take the window with it
      if (NSIntersectsRect(s.visibleFrame, f)) return f;
  }
  NSRect vis = NSScreen.mainScreen.visibleFrame;
  return NSMakeRect(NSMaxX(vis) - size - 24, NSMinY(vis) + 24, size, size);
}

- (void)applicationDidFinishLaunching:(NSNotification *)note {
  NSMenu *bar = [[NSMenu alloc] init];  // without this there is no Cmd-Q in an app without a Dock icon
  NSMenu *appMenu = [[NSMenu alloc] init];
  [appMenu addItemWithTitle:@"About " APP_NAME action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
  [appMenu addItemWithTitle:@"Quit " APP_NAME action:@selector(terminate:) keyEquivalent:@"q"];
  [bar addItemWithTitle:@"" action:nil keyEquivalent:@""].submenu = appMenu;
  NSApp.mainMenu = bar;

  _window = [[RoundWindow alloc] initWithContentRect:windowFrameFor(initialFrame(gOwnSave))
                                           styleMask:NSWindowStyleMaskBorderless
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
  _window.opaque = NO;
  _window.backgroundColor = NSColor.clearColor;
  _window.hasShadow = YES;
  _window.level = NSFloatingWindowLevel;
  _window.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary;
  _window.delegate = self;
  CGFloat side = _window.frame.size.width;
  RingView *ring = [[RingView alloc] initWithFrame:NSMakeRect(0, 0, side, side)];
  _view = [[GameView alloc] initWithFrame:NSInsetRect(ring.bounds, RING_HOVER, RING_HOVER)];
  _view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  [ring addSubview:_view];
  _window.contentView = ring;
  [ring addTrackingArea:[[NSTrackingArea alloc]
                            initWithRect:NSZeroRect
                                 options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                                         NSTrackingActiveAlways | NSTrackingInVisibleRect
                                   owner:ring
                                userInfo:nil]];
  [_view showFrame];
  [_window makeKeyAndOrderFront:nil];
  [_window makeFirstResponder:_view];
  _statusItem = [NSStatusBar.systemStatusBar statusItemWithLength:NSSquareStatusItemLength];
  _statusItem.button.image = statusIcon();
  _statusItem.button.toolTip = @APP_NAME;
  _statusItem.menu = [[NSMenu alloc] init];
  _statusItem.menu.delegate = self;
  [_view fillMenu:_statusItem.menu];

  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];  // with the console, without stealing focus from whoever is testing

  // Outside the Dock and covered up, macOS would put it to sleep (App Nap) and the game would fall behind
  _activity = [NSProcessInfo.processInfo beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep
                                                           reason:@"the game runs in real time"];
  [NSWorkspace.sharedWorkspace.notificationCenter addObserver:self
                                                     selector:@selector(willSleep:)
                                                         name:NSWorkspaceWillSleepNotification
                                                       object:nil];
  NSTimer *timer = [NSTimer timerWithTimeInterval:TICK_S target:self selector:@selector(tick:) userInfo:nil repeats:YES];
  timer.tolerance = TICK_S / 4;
  // also while the window is being dragged or the menu is open
  [NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];
  // starting the audio device can take seconds: don't stall the game
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{ startAudio(); });
  // the relay: the real app always; test copies only when asked (POKEMACGOCHI_RELAY=1), so they don't
  // turn up as buddies on everyone's devices at home
  const char *relayEnv = getenv("POKEMACGOCHI_RELAY");
  if (relayEnv ? atoi(relayEnv) != 0 : gOwnSave) relayConnect();
  if (gOwnSave && !gConsole) {  // test copies never nag
    [self performSelector:@selector(checkForUpdates:) withObject:nil afterDelay:10];
    NSTimer *daily = [NSTimer timerWithTimeInterval:UPDATE_EVERY_S target:self selector:@selector(dailyUpdateCheck:) userInfo:nil repeats:YES];
    daily.tolerance = 600;
    [NSRunLoop.mainRunLoop addTimer:daily forMode:NSRunLoopCommonModes];
  }
}

// Asks GitHub for the newest Mac release. From the menu (sender != nil) it always answers;
// the automatic check only speaks up the first time it finds each new version.
- (void)checkForUpdates:(id)sender {
  bool asked = sender != nil;
  if (asked && gNewVersion) {  // already known: straight to the download
    [NSWorkspace.sharedWorkspace openURL:gNewURL];
    return;
  }
  NSMutableURLRequest *req = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:@RELEASES_API]];
  [req setValue:@APP_NAME forHTTPHeaderField:@"User-Agent"];
  [req setValue:@"application/vnd.github+json" forHTTPHeaderField:@"Accept"];
  req.timeoutInterval = 15;
  [[NSURLSession.sharedSession dataTaskWithRequest:req completionHandler:^(NSData *data, NSURLResponse *resp, NSError *err) {
    NSString *found = nil;
    NSURL *url = nil;
    NSArray *list = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    if ([list isKindOfClass:NSArray.class])
      for (NSDictionary *r in list) {
        if (![r isKindOfClass:NSDictionary.class] || [r[@"draft"] boolValue] || [r[@"prerelease"] boolValue]) continue;
        NSString *tag = r[@"tag_name"];
        if (![tag isKindOfClass:NSString.class] || ![tag hasPrefix:@"mac-v"]) continue;
        NSString *v = [tag substringFromIndex:5];
        if (found && [v compare:found options:NSNumericSearch] != NSOrderedDescending) continue;
        found = v;
        url = [NSURL URLWithString:r[@"html_url"] ?: @GITHUB_URL];
        for (NSDictionary *a in r[@"assets"])
          if ([a[@"name"] isEqual:@"PokeMacGochi.dmg"]) url = [NSURL URLWithString:a[@"browser_download_url"]];
      }
    dispatch_async(dispatch_get_main_queue(), ^{ [self updateCheckFound:found url:url asked:asked]; });
  }] resume];
}

- (void)dailyUpdateCheck:(NSTimer *)t { [self checkForUpdates:nil]; }

- (void)updateCheckFound:(NSString *)found url:(NSURL *)url asked:(bool)asked {
  NSString *mine = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"] ?: @"0";
  bool newer = found && url && [found compare:mine options:NSNumericSearch] == NSOrderedDescending;
  if (newer) {
    gNewVersion = found;
    gNewURL = url;
  }
  NSUserDefaults *d = NSUserDefaults.standardUserDefaults;
  if (!asked && (!newer || [[d stringForKey:kPrefToldVersion] isEqual:found])) return;
  NSAlert *alert = [[NSAlert alloc] init];
  if (newer) {
    [d setObject:found forKey:kPrefToldVersion];
    alert.messageText = [NSString stringWithFormat:@APP_NAME " %@ is available", found];
    alert.informativeText = [NSString stringWithFormat:@"You have version %@. Your pet is saved separately from the app, so "
                                                       @"it stays when you replace the app with the new one: quit "
                                                       @APP_NAME ", open the new DMG and drag it into Applications.",
                                                       mine];
    [alert addButtonWithTitle:@"Download"];
    [alert addButtonWithTitle:@"Later"];
  } else {
    alert.messageText = found ? @"You're up to date" : @"Couldn't check for updates";
    alert.informativeText = found ? [NSString stringWithFormat:@APP_NAME " %@ is the newest version.", mine]
                                  : @"Check your internet connection and try again.";
  }
  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];
  if ([alert runModal] == NSAlertFirstButtonReturn && newer) [NSWorkspace.sharedWorkspace openURL:url];
}

- (void)tick:(NSTimer *)t {
  hostTick();
  if (hostFrameDirty()) [_view showFrame];
  if (hostRestartRequested()) {
    hostFlushSave();
    execv(NSBundle.mainBundle.executablePath.fileSystemRepresentation, gArgv);
    [NSApp terminate:nil];  // should not get here
  }
}

- (void)willSleep:(NSNotification *)note { hostFlushSave(); }
- (void)windowDidResignKey:(NSNotification *)note { [_view releaseKeys]; }
- (void)windowDidMove:(NSNotification *)note { [_view saveWindowPlace]; }
- (void)applicationWillTerminate:(NSNotification *)note { hostFlushSave(); }
@end

// Testing: "MOUSE down|drag|up|right x y", "KEY down|up code" and "HOVER on|off", in canvas coordinates, enter through the
// AppKit event queue like real ones, without touching the real mouse. "MENU n [m]" triggers menu entry n (or m in its submenu).
static void postTestEvent(const char *line) {
  char what[8] = "";
  int a = 0, b = 0;
  NSWindow *w = nil;
  for (NSWindow *candidate in NSApp.windows)
    if ([candidate isKindOfClass:RoundWindow.class]) w = candidate;
  int n = sscanf(line, "MENU %d %d", &a, &b);
  if (n >= 1) {
    NSMenu *menu = [[NSMenu alloc] init];
    [(GameView *)w.contentView.subviews.firstObject fillMenu:menu];
    if (n == 2) [[menu itemAtIndex:a].submenu performActionForItemAtIndex:b];  // "MENU n m": entry m of its submenu
    else [menu performActionForItemAtIndex:a];
    return;
  }
  if (sscanf(line, "HOVER %7s", what) == 1) {  // "HOVER on|off": the mouse over the window, for the frame
    [(RingView *)w.contentView setHover:!strcmp(what, "on")];
    return;
  }
  if (sscanf(line, "MOUSE %7s %d %d", what, &a, &b) == 3) {
    CGFloat k = (w.frame.size.width - 2 * RING_HOVER) / HOST_CANVAS_SIZE;
    NSEventType t = !strcmp(what, "down") ? NSEventTypeLeftMouseDown
                    : !strcmp(what, "drag") ? NSEventTypeLeftMouseDragged
                    : !strcmp(what, "right") ? NSEventTypeRightMouseDown : NSEventTypeLeftMouseUp;
    NSPoint p = NSMakePoint(RING_HOVER + a * k, w.frame.size.height - RING_HOVER - b * k);
    [NSApp postEvent:[NSEvent mouseEventWithType:t location:p modifierFlags:0 timestamp:0 windowNumber:w.windowNumber
                                         context:nil eventNumber:0 clickCount:1 pressure:1]
             atStart:NO];
  } else if (sscanf(line, "KEY %7s %d", what, &a) == 2) {
    bool down = !strcmp(what, "down");
    [NSApp postEvent:[NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0
                                     timestamp:0 windowNumber:w.windowNumber context:nil characters:@" "
                   charactersIgnoringModifiers:@" " isARepeat:NO keyCode:(unsigned short)a]
             atStart:NO];
  }
}

// --console: reads serial console commands from stdin, like with the Knomi over USB
static void startConsole(void) {
  [NSThread detachNewThreadWithBlock:^{
    char line[256];
    while (fgets(line, sizeof line, stdin)) {
      if (!strncmp(line, "MOUSE ", 6) || !strncmp(line, "KEY ", 4) || !strncmp(line, "MENU ", 5) || !strncmp(line, "HOVER ", 6)) {
        NSString *copy = @(line);
        dispatch_async(dispatch_get_main_queue(), ^{ postTestEvent(copy.UTF8String); });
      } else {
        hostSerialPush(line);
      }
    }
  }];
}

// Two processes on the same save would overwrite each other's saves
static bool lockSave(NSString *saveDir) {
  [NSFileManager.defaultManager createDirectoryAtPath:saveDir withIntermediateDirectories:YES attributes:nil error:nil];
  int fd = open([saveDir stringByAppendingPathComponent:@".lock"].fileSystemRepresentation, O_CREAT | O_RDWR | O_CLOEXEC, 0644);
  return fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) == 0;
}

int main(int argc, char **argv) {
  gArgv = argv;
  @autoreleasepool {
    NSString *saveDir = NSProcessInfo.processInfo.environment[@"POKEMACGOCHI_SAVE_DIR"];
    gOwnSave = saveDir == nil;
    if (!saveDir) {
      NSString *support = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES)[0];
      saveDir = [support stringByAppendingPathComponent:@APP_NAME];
    }
    if (!lockSave(saveDir)) {
      fprintf(stderr, APP_NAME " is already open\n");
      if (gOwnSave) {  // opened again by hand: say where the running one is instead of doing nothing
        NSApplication.sharedApplication.activationPolicy = NSApplicationActivationPolicyAccessory;
        [NSApp activateIgnoringOtherApps:YES];
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @APP_NAME " is already open";
        alert.informativeText = @"It's the round window on your desktop. If it is hidden, use the Pok\u00e9ball in the menu bar to show it.";
        [alert runModal];
      }
      return 0;
    }
    hostInit(NSBundle.mainBundle.resourcePath.fileSystemRepresentation, saveDir.fileSystemRepresentation);
    for (int i = 1; i < argc; i++)
      if (strcmp(argv[i], "--console") == 0) { gConsole = true; startConsole(); }

    NSApplication *app = NSApplication.sharedApplication;
    app.activationPolicy = NSApplicationActivationPolicyAccessory;
    AppDelegate *delegate = [[AppDelegate alloc] init];
    app.delegate = delegate;

    // a normal kill saves too
    signal(SIGTERM, SIG_IGN);
    dispatch_source_t term = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGTERM, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(term, ^{ [NSApp terminate:nil]; });
    dispatch_resume(term);

    [app run];
  }
  return 0;
}
