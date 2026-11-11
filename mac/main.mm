// PokeMacGochi: the game's round window on a Mac desktop. Only the window; the game
// and everything else live in host/ (see host/host.h).
#import <AppKit/AppKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <QuartzCore/QuartzCore.h>
#import <Security/Security.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include "host.h"
#include "shim/Preferences.h"  // the save folder's storage, as the game uses it
#include "../update_key.h"        // checks the updates' signature

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

// New versions: the newest "mac-v*" release on GitHub. Checked at launch and every UPDATE_EVERY_S by the
// real clock (time the Mac sleeps counts, as it doesn't for a timer), looked at every UPDATE_POLL_S, so a
// release made overnight is in by morning; a check that got no answer is tried again at the next look. The
// player is told once per version and can always check from the menu. The save lives outside
// the app, so a new app keeps the pet.
// Updating in place, as the Knomi does: the release also carries the app as a zip and its
// signature (tools/sign_update.sh). The app downloads both itself, checks the signature against
// UPDATE_PUBLIC_KEY, unpacks the new app next to itself, saves the game, swaps the two and opens the
// new one. Downloaded by the app and not by a browser, it isn't quarantined: no Gatekeeper warning
// after the first install. When it can't (an older release, an app it can't write, running from the
// DMG), the menu offers the download as before.
#define RELEASES_API "https://api.github.com/repos/FedorSosnin/PokeKnomiGochi/releases?per_page=30"
#define UPDATE_EVERY_S (60 * 60)  // GitHub answers 60 an hour from one address
#define UPDATE_POLL_S (10 * 60)
static NSDate *gUpdateAnswered = nil;  // when GitHub last answered an update check
#define UPDATE_ZIP "PokeMacGochi.zip"
static NSString *const kPrefToldVersion = @"updateToldVersion";
static NSString *gNewVersion = nil;  // newer than this app, once a check found one
static NSURL *gNewURL = nil;         // its DMG (or the release page)
static NSURL *gNewZip = nil;         // the new app, zipped, and its signature: to update in place
static NSURL *gNewSig = nil;
static bool gUpdating = false;
// Update Automatically (the game's setting: on unless turned off): the version found waits here until
// nobody is using the game (hostQuietForUpdate), then goes in without asking. A failed one says nothing
// and is tried again with the next day's check.
static bool gAutoPending = false;
static bool gAutoInstall = false;  // the update running now went in by itself

// the signature (ECDSA P-256 over SHA-256, DER) of an update, against the key the app carries
static bool updateSignatureOK(NSData *data, NSData *sig) {
  NSDictionary *attrs = @{(id)kSecAttrKeyType : (id)kSecAttrKeyTypeECSECPrimeRandom,
                          (id)kSecAttrKeyClass : (id)kSecAttrKeyClassPublic, (id)kSecAttrKeySizeInBits : @256};
  SecKeyRef key = SecKeyCreateWithData((__bridge CFDataRef)[NSData dataWithBytes:UPDATE_PUBLIC_KEY length:sizeof(UPDATE_PUBLIC_KEY)],
                                       (__bridge CFDictionaryRef)attrs, NULL);
  if (!key || !data.length || !sig.length) {
    if (key) CFRelease(key);
    return false;
  }
  bool ok = SecKeyVerifySignature(key, kSecKeyAlgorithmECDSASignatureMessageX962SHA256, (__bridge CFDataRef)data,
                                  (__bridge CFDataRef)sig, NULL);
  CFRelease(key);
  return ok;
}

// can this copy replace itself? (it has the files for it, and it may write where it is: not the
// DMG, not a copy macOS runs from a private place because it hasn't been moved yet)
static bool canUpdateInPlace() {
  NSString *app = NSBundle.mainBundle.bundlePath;
  NSFileManager *fm = NSFileManager.defaultManager;
  return gNewZip && gNewSig && ![app containsString:@"/AppTranslocation/"] && [fm isWritableFileAtPath:app] &&
         [fm isWritableFileAtPath:app.stringByDeletingLastPathComponent];
}

static NSInteger httpStatus(NSURLResponse *r) {
  return [r isKindOfClass:NSHTTPURLResponse.class] ? ((NSHTTPURLResponse *)r).statusCode : 0;
}

// ---- network time: the game's clock follows a time server (the Date of an answer from HOST_TIME_URL),
// not the Mac's clock, so setting the Mac's clock can't fake time away. 0 to the game: no answer ----
static void fetchNetTime() {
  NSString *env = NSProcessInfo.processInfo.environment[@"POKEMACGOCHI_TIME_URL"];  // testing
  NSMutableURLRequest *req = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:env.length ? env : @HOST_TIME_URL]
                                                     cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                                 timeoutInterval:15];
  req.HTTPMethod = @"HEAD";
  [[NSURLSession.sharedSession dataTaskWithRequest:req completionHandler:^(NSData *d, NSURLResponse *r, NSError *e) {
    uint32_t utc = 0;
    NSString *date = !e && [r isKindOfClass:NSHTTPURLResponse.class] ? [(NSHTTPURLResponse *)r valueForHTTPHeaderField:@"Date"] : nil;
    if (date) {
      NSDateFormatter *f = [NSDateFormatter new];
      f.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
      f.timeZone = [NSTimeZone timeZoneForSecondsFromGMT:0];
      f.dateFormat = @"EEE, dd MMM yyyy HH:mm:ss zzz";
      NSDate *t = [f dateFromString:date];
      if (t) utc = (uint32_t)t.timeIntervalSince1970;
    }
    dispatch_async(dispatch_get_main_queue(), ^{ hostNetTime(utc); });
  }] resume];
}

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
#if UI_V2
#define AUDIO_FRAMES 1536  // ~32 ms per buffer at 48 kHz
#else
#define AUDIO_FRAMES 512  // ~32 ms per buffer at 16 kHz
#endif

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
- (void)offerUpdate:(NSString *)version;
- (void)installUpdate;
- (void)swapInUpdate:(NSURL *)fresh;
- (void)updateFailed:(NSString *)why;
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
    self.layer.magnificationFilter = kCAFilterLinear;  // the canvas is 466 px: a stretch that is not a whole number, so nearest doubled every ~5th row and column (blocky, uneven text at Large)
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

- (void)mouseEntered:(NSEvent *)event { [self mouseMoved:event]; }
- (void)mouseMoved:(NSEvent *)event {  // the frame's tracking sets the cursor
  hostPointerHover();
  NSPoint p = [self canvasPoint:event];
  bool in = p.x >= 0 && p.y >= 0 && p.x < HOST_CANVAS_SIZE && p.y < HOST_CANVAS_SIZE;
  hostPointerHoverAt(in ? (int)p.x : -1, (int)p.y);  // V2 outlines the control under it
}
- (void)mouseExited:(NSEvent *)event { hostPointerHoverAt(-1, -1); }

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
  NSString *upd = gUpdating     ? @"Updating…"
                  : !gNewVersion ? @"Check for Updates…"
                  : [NSString stringWithFormat:canUpdateInPlace() ? @"Update to Version %@…" : @"Download Version %@…", gNewVersion];
  NSMenuItem *update = [menu addItemWithTitle:upd action:gUpdating ? nil : @selector(checkForUpdates:) keyEquivalent:@""];
  update.target = NSApp.delegate;
  NSMenuItem *autoUpd = [menu addItemWithTitle:@"Update Automatically" action:@selector(toggleAutoUpdate:) keyEquivalent:@""];
  autoUpd.target = self;
  autoUpd.state = hostAutoUpdate() ? NSControlStateValueOn : NSControlStateValueOff;
  [menu addItem:NSMenuItem.separatorItem];
  [menu addItemWithTitle:@"Drag the ring to move" action:nil keyEquivalent:@""].enabled = NO;
  [menu addItemWithTitle:@"\u2190 \u2192 tilt  \u00b7  Space: wild battle  \u00b7  Space or \u2191: throw" action:nil keyEquivalent:@""].enabled = NO;
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
- (void)toggleAutoUpdate:(id)sender {
  hostSetAutoUpdate(!hostAutoUpdate());
  gAutoPending = hostAutoUpdate() && gNewVersion && canUpdateInPlace();  // one already found goes in too
}

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
                                      initWithString:@"\nSprites from PMDCollab (CC BY-NC). "
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
  // test copies never nag, unless given a release list of their own to test updating with
  if ((gOwnSave && !gConsole) || NSProcessInfo.processInfo.environment[@"POKEMACGOCHI_UPDATE_API"]) {
    [self performSelector:@selector(checkForUpdates:) withObject:nil afterDelay:10];
    NSTimer *poll = [NSTimer timerWithTimeInterval:UPDATE_POLL_S target:self selector:@selector(pollUpdateCheck:) userInfo:nil repeats:YES];
    poll.tolerance = 60;
    [NSRunLoop.mainRunLoop addTimer:poll forMode:NSRunLoopCommonModes];
  }
}

// Asks GitHub for the newest Mac release. From the menu (sender != nil) it always answers, and always
// asks again, even with a new version already found: a newer one may be out since (Fedor was offered 2.0
// after 2.0.1 was out). The automatic check only speaks up the first time it finds each new version.
- (void)checkForUpdates:(id)sender {
  bool asked = sender != nil;
  if (gUpdating) return;
  // POKEMACGOCHI_UPDATE_API: another release list, for testing the update on a local server
  NSString *api = NSProcessInfo.processInfo.environment[@"POKEMACGOCHI_UPDATE_API"] ?: @RELEASES_API;
  NSMutableURLRequest *req = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:api]];
  [req setValue:@APP_NAME forHTTPHeaderField:@"User-Agent"];
  [req setValue:@"application/vnd.github+json" forHTTPHeaderField:@"Accept"];
  req.timeoutInterval = 15;
  if (asked) req.cachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
  [[NSURLSession.sharedSession dataTaskWithRequest:req completionHandler:^(NSData *data, NSURLResponse *resp, NSError *err) {
    NSString *found = nil;
    NSURL *url = nil, *zip = nil, *sig = nil;
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
        zip = sig = nil;
        for (NSDictionary *a in r[@"assets"]) {
          if (![a isKindOfClass:NSDictionary.class] || ![a[@"browser_download_url"] isKindOfClass:NSString.class]) continue;
          NSURL *u = [NSURL URLWithString:a[@"browser_download_url"]];
          if ([a[@"name"] isEqual:@"PokeMacGochi.dmg"]) url = u;
          if ([a[@"name"] isEqual:@UPDATE_ZIP]) zip = u;
          if ([a[@"name"] isEqual:@UPDATE_ZIP ".sig"]) sig = u;
        }
      }
    bool answered = [list isKindOfClass:NSArray.class];
    dispatch_async(dispatch_get_main_queue(), ^{
      if (answered) gUpdateAnswered = NSDate.date;
      if (asked && !answered && gNewVersion) {  // no answer, but one was found before: that one
        if (canUpdateInPlace()) [self offerUpdate:gNewVersion];
        else [NSWorkspace.sharedWorkspace openURL:gNewURL];
        return;
      }
      [self updateCheckFound:found url:url zip:zip sig:sig asked:asked];
    });
  }] resume];
}

// the automatic check, when it's due by the clock (or the last one got no answer)
- (void)pollUpdateCheck:(NSTimer *)t {
  if (!gUpdateAnswered || -gUpdateAnswered.timeIntervalSinceNow >= UPDATE_EVERY_S) [self checkForUpdates:nil];
}

// "Update now?", then the update in place
- (void)offerUpdate:(NSString *)version {
  NSString *mine = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"] ?: @"0";
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = [NSString stringWithFormat:@APP_NAME " %@ is available", version];
  alert.informativeText = [NSString stringWithFormat:@"You have version %@. Updating takes a few seconds: " @APP_NAME
                                                     @" closes and opens again on the new version, with your pet just as it was.",
                                                     mine];
  [alert addButtonWithTitle:@"Update"];
  [alert addButtonWithTitle:@"Later"];
  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];
  if (gConsole || [alert runModal] == NSAlertFirstButtonReturn) [self installUpdate];  // a test copy doesn't wait for a click
}

// downloads the zip and its signature, checks it, unpacks it next to the app; then swaps and reopens
- (void)installUpdate {
  if (gUpdating || !canUpdateInPlace()) return;
  gUpdating = true;
  NSURL *zipURL = gNewZip, *sigURL = gNewSig;
  NSString *version = gNewVersion;
  NSURL *appURL = NSBundle.mainBundle.bundleURL;
  fprintf(stderr, "update: downloading %s\n", version.UTF8String);
  NSURLSession *s = NSURLSession.sharedSession;
  [[s dataTaskWithURL:sigURL completionHandler:^(NSData *sig, NSURLResponse *r1, NSError *e1) {
    if (httpStatus(r1) != 200 || !sig.length) {
      dispatch_async(dispatch_get_main_queue(), ^{ [self updateFailed:@"The update couldn't be downloaded."]; });
      return;
    }
    [[s dataTaskWithURL:zipURL completionHandler:^(NSData *zip, NSURLResponse *r2, NSError *e2) {
      NSString *err = nil;
      NSURL *fresh = nil;
      if (httpStatus(r2) != 200 || !zip.length) err = @"The update couldn't be downloaded.";
      else if (!updateSignatureOK(zip, sig)) err = @"The update didn't pass its safety check, so it wasn't installed.";
      else {
        // unpacked on the app's own disk, so the swap is a rename
        NSFileManager *fm = NSFileManager.defaultManager;
        NSURL *tmp = [fm URLForDirectory:NSItemReplacementDirectory inDomain:NSUserDomainMask appropriateForURL:appURL create:YES error:nil];
        NSURL *zipFile = [tmp URLByAppendingPathComponent:@UPDATE_ZIP];
        if (!tmp || ![zip writeToURL:zipFile atomically:NO]) err = @"There was no room to unpack the update.";
        else {
          NSTask *unzip = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/ditto"
                                                   arguments:@[ @"-x", @"-k", zipFile.path, tmp.path ]];
          [unzip waitUntilExit];
          fresh = [tmp URLByAppendingPathComponent:appURL.lastPathComponent];
          NSDictionary *info = [NSDictionary dictionaryWithContentsOfURL:[fresh URLByAppendingPathComponent:@"Contents/Info.plist"]];
          if (unzip.terminationStatus != 0 || ![info[@"CFBundleShortVersionString"] isEqual:version] ||
              ![info[@"CFBundleIdentifier"] isEqual:NSBundle.mainBundle.bundleIdentifier])
            err = @"The update wasn't the app it should be, so it wasn't installed.";
        }
      }
      dispatch_async(dispatch_get_main_queue(), ^{
        if (err) [self updateFailed:err];
        else [self swapInUpdate:fresh];
      });
    }] resume];
  }] resume];
}

- (void)swapInUpdate:(NSURL *)fresh {
  NSURL *appURL = NSBundle.mainBundle.bundleURL;
  hostFlushSave();  // the pet first
  NSError *err = nil;
  NSURL *now = nil;
  if (![NSFileManager.defaultManager replaceItemAtURL:appURL withItemAtURL:fresh backupItemName:nil options:0
                                     resultingItemURL:&now error:&err]) {
    [self updateFailed:@"The new app couldn't be put in place of this one."];
    return;
  }
  fprintf(stderr, "update: installed, reopening\n");
  // reopen once this one has quit (the save is locked while it runs). A test copy (its own save, from
  // POKEMACGOCHI_SAVE_DIR) starts its program directly so it keeps that save; the real app goes through open
  NSString *path = (now ?: appURL).path;
  NSString *wait = [NSString stringWithFormat:@"while kill -0 %d 2>/dev/null; do sleep 0.2; done; ", getpid()];
  NSString *cmd = gOwnSave ? [wait stringByAppendingString:@"exec /usr/bin/open \"$0\""]
                           : [wait stringByAppendingString:@"exec \"$0/Contents/MacOS/" APP_NAME "\" \"$@\""];
  NSMutableArray *args = [NSMutableArray arrayWithObjects:@"-c", cmd, path, nil];
  if (!gOwnSave)
    for (char **a = gArgv + 1; *a; a++) [args addObject:@(*a)];
  NSTask *t = [[NSTask alloc] init];
  t.launchPath = @"/bin/sh";
  t.arguments = args;
  [t launch];
  [NSApp terminate:nil];
}

- (void)updateFailed:(NSString *)why {
  gUpdating = false;
  fprintf(stderr, "update: failed: %s\n", why.UTF8String);
  if (gConsole || gAutoInstall) {  // by itself: no alert for nobody; tomorrow's check tries again
    gAutoInstall = false;
    return;
  }
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = @"Couldn't update";
  alert.informativeText = [why stringByAppendingString:@" You can download the new version instead."];
  [alert addButtonWithTitle:@"Download"];
  [alert addButtonWithTitle:@"Later"];
  [NSApp activateIgnoringOtherApps:YES];
  if ([alert runModal] == NSAlertFirstButtonReturn && gNewURL) [NSWorkspace.sharedWorkspace openURL:gNewURL];
}

- (void)updateCheckFound:(NSString *)found url:(NSURL *)url zip:(NSURL *)zip sig:(NSURL *)sig asked:(bool)asked {
  NSString *mine = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"] ?: @"0";
  bool newer = found && url && [found compare:mine options:NSNumericSearch] == NSOrderedDescending;
  if (newer) {
    gNewVersion = found;
    gNewURL = url;
    gNewZip = zip;
    gNewSig = sig;
  }
  if (!asked && newer && canUpdateInPlace() && hostAutoUpdate()) {  // goes in by itself, once nobody's using it
    if (!gAutoPending) fprintf(stderr, "update: %s found, updating once it's quiet\n", found.UTF8String);
    gAutoPending = true;
    return;
  }
  NSUserDefaults *d = NSUserDefaults.standardUserDefaults;
  if (!asked && (!newer || [[d stringForKey:kPrefToldVersion] isEqual:found])) return;
  if (newer) [d setObject:found forKey:kPrefToldVersion];
  if (newer && canUpdateInPlace()) {
    [self offerUpdate:found];
    return;
  }
  NSAlert *alert = [[NSAlert alloc] init];
  if (newer) {
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
  if (hostNetTimeWanted()) fetchNetTime();
  if (hostFrameDirty()) [_view showFrame];
  if (gAutoPending && !gUpdating && hostAutoUpdate() && hostQuietForUpdate()) {
    gAutoPending = false;
    gAutoInstall = true;
    fprintf(stderr, "update: quiet, updating to %s by itself\n", gNewVersion.UTF8String);
    [self installUpdate];
  }
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

// a link from the game (About's): in the browser, from the main thread
static void macOpenUrl(const char *u) {
  NSString *s = [NSString stringWithUTF8String:u];
  dispatch_async(dispatch_get_main_queue(), ^{
    NSURL *url = [NSURL URLWithString:s];
    if (url) [NSWorkspace.sharedWorkspace openURL:url];
  });
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
    hostSetOpenUrl(macOpenUrl);  // About's links, in the browser
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
