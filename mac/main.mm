// PokeMacGochi: la ventana redonda del juego en el escritorio de un Mac. Solo la ventana; el juego
// y todo lo demas viven en host/ (ver host/host.h).
#import <AppKit/AppKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <QuartzCore/QuartzCore.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include "host.h"

#define APP_NAME "PokeMacGochi"
#define TICK_S (1.0 / 60.0)
// El marco de Pokeball que rodea la pantalla, en puntos
#define RING 6.0
#define RING_OUTLINE 1.0
#define RING_LINE 20.0  // la franja negra de en medio
// El marco y el borde del circulo (fuera de esta fraccion del radio) arrastran la ventana. La interfaz
// del juego acaba antes: sus botones mas alejados del centro llegan a ~208 de 233.
#define DRAG_RIM 0.92
static const CGFloat kSizes[] = {190, 240, 300};  // lado de la ventana, marco incluido
#define DEFAULT_SIZE 240.0

static bool gConsole = false;  // --console: lanzada para pruebas
static char **gArgv;

static NSString *const kPrefSize = @"windowSize";
static NSString *const kPrefOriginX = @"windowX";
static NSString *const kPrefOriginY = @"windowY";

// ---- sonido: pide PCM a la capa de host desde el hilo de audio ----
#define AUDIO_BUFFERS 3
#define AUDIO_FRAMES 512  // ~32 ms por bufer a 16 kHz

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
    NSLog(@"sin salida de audio: el juego sigue sin sonido");
    return;
  }
  for (int i = 0; i < AUDIO_BUFFERS; i++) {
    AudioQueueBufferRef buf;
    if (AudioQueueAllocateBuffer(q, AUDIO_FRAMES * sizeof(int16_t), &buf) == noErr) audioCallback(NULL, q, buf);
  }
  AudioQueueStart(q, NULL);
}

// ---- ventana ----
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

// El marco: una Pokeball con la pantalla del juego dentro. Se agarra para mover la ventana.
@interface RingView : NSView
@end

@implementation RingView
- (void)drawRect:(NSRect)dirty {
  NSRect b = self.bounds;
  CGFloat mid = NSMidY(b);
  [NSGraphicsContext saveGraphicsState];
  [[NSBezierPath bezierPathWithOvalInRect:b] addClip];
  [NSColor.whiteColor setFill];
  NSRectFill(NSMakeRect(0, 0, b.size.width, mid));
  [[NSColor colorWithSRGBRed:0.87 green:0.13 blue:0.13 alpha:1] setFill];
  NSRectFill(NSMakeRect(0, mid, b.size.width, mid));
  [NSColor.blackColor setFill];
  NSRectFill(NSMakeRect(0, mid - RING_LINE / 2, b.size.width, RING_LINE));
  [NSGraphicsContext restoreGraphicsState];
  // el contorno de la bola: sin el, la mitad blanca se pierde sobre un fondo claro
  NSBezierPath *edge = [NSBezierPath bezierPathWithOvalInRect:NSInsetRect(b, RING_OUTLINE / 2, RING_OUTLINE / 2)];
  edge.lineWidth = RING_OUTLINE;
  [NSColor.blackColor setStroke];
  [edge stroke];
}

// fuera del circulo la ventana no existe: el clic pasa a lo que haya debajo
- (NSView *)hitTest:(NSPoint)point {
  NSPoint p = [self.superview convertPoint:point toView:self];
  CGFloat r = self.bounds.size.width / 2;
  if (hypot(p.x - r, p.y - r) > r) return nil;
  return [super hitTest:point] ?: self;
}

- (void)mouseDown:(NSEvent *)event {
  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];
  [self.window makeKeyWindow];
  [self.window performWindowDragWithEvent:event];
  [(GameView *)self.subviews.firstObject saveWindowPlace];
}

- (void)rightMouseDown:(NSEvent *)event { [self.subviews.firstObject rightMouseDown:event]; }
- (void)mouseMoved:(NSEvent *)event { [NSCursor.openHandCursor set]; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }
@end

@implementation GameView {
  uint8_t *_rgba;
  bool _left, _right, _touching;
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
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }  // el primer clic ya es un toque

- (void)showFrame {
  hostFrameRGBA(_rgba);
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef prov = CGDataProviderCreateWithData(NULL, _rgba, HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4, NULL);
  CGImageRef img = CGImageCreate(HOST_CANVAS_SIZE, HOST_CANVAS_SIZE, 8, 32, HOST_CANVAS_SIZE * 4, cs,
                                 kCGBitmapByteOrderDefault | kCGImageAlphaNoneSkipLast, prov, NULL, false,
                                 kCGRenderingIntentDefault);
  // copia para que la capa no dependa del bufer que sobreescribe el siguiente frame
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

// punto del evento en coordenadas del lienzo
- (NSPoint)canvasPoint:(NSEvent *)event {
  NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
  CGFloat k = HOST_CANVAS_SIZE / self.bounds.size.width;
  return NSMakePoint(p.x * k, (self.bounds.size.height - p.y) * k);
}

- (double)radialFraction:(NSEvent *)event {
  NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
  CGFloat r = self.bounds.size.width / 2;
  return hypot(p.x - r, p.y - r) / r;
}

// dentro del marco, lo que quede fuera del circulo es del marco
- (NSView *)hitTest:(NSPoint)point {
  NSPoint p = [self.superview convertPoint:point toView:self];
  CGFloat r = self.bounds.size.width / 2;
  return hypot(p.x - r, p.y - r) <= r ? self : nil;
}

- (void)mouseDown:(NSEvent *)event {
  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];
  [self.window makeKeyWindow];
  if ([self radialFraction:event] >= DRAG_RIM || (event.modifierFlags & NSEventModifierFlagOption)) {
    [self.window performWindowDragWithEvent:event];
    [self saveWindowPlace];
    return;
  }
  if (event.modifierFlags & NSEventModifierFlagControl) {
    [self rightMouseDown:event];
    return;
  }
  NSPoint p = [self canvasPoint:event];
  _touching = true;
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
  NSPoint p = [self canvasPoint:event];
  hostPointer((int)p.x, (int)p.y, false);
}

- (void)mouseEntered:(NSEvent *)event { hostPointerHover(); }

- (void)mouseMoved:(NSEvent *)event {
  hostPointerHover();
  if ([self radialFraction:event] >= DRAG_RIM) [NSCursor.openHandCursor set];
  else [NSCursor.arrowCursor set];
}

- (void)mouseExited:(NSEvent *)event { [NSCursor.arrowCursor set]; }

// El mismo menu con clic derecho y en la barra de menus
- (void)fillMenu:(NSMenu *)menu {
  [menu removeAllItems];
  menu.autoenablesItems = NO;
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
    it.state = self.window.frame.size.width == kSizes[i] ? NSControlStateValueOn : NSControlStateValueOff;
  }
  sizes.submenu = sub;
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

// oculta sigue viva: el juego no para, como un Knomi con la pantalla apagada
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
  [self.window setFrame:NSMakeRect(c.x - s / 2, c.y - s / 2, s, s) display:YES];
  [self.window invalidateShadow];
  [self saveWindowPlace];
}

- (void)saveWindowPlace {
  NSRect f = self.window.frame;
  NSUserDefaults *d = NSUserDefaults.standardUserDefaults;
  [d setDouble:f.size.width forKey:kPrefSize];
  [d setDouble:f.origin.x forKey:kPrefOriginX];
  [d setDouble:f.origin.y forKey:kPrefOriginY];
}

- (void)applyKeys { hostTiltKeys(_left, _right); }

- (bool)handleKey:(NSEvent *)event down:(bool)down {
  switch (event.keyCode) {
    case 123: _left = down; [self applyKeys]; return true;   // flecha izquierda
    case 124: _right = down; [self applyKeys]; return true;  // flecha derecha
    case 49: hostButton(down); return true;                  // espacio
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

// sin foco no llega el keyUp: la tecla se quedaria pulsada para siempre
- (void)releaseKeys {
  _left = _right = false;
  [self applyKeys];
  hostButton(false);
}

- (void)dealloc { free(_rgba); }
@end

// ---- aplicacion ----
@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, NSMenuDelegate>
@end

@implementation AppDelegate {
  RoundWindow *_window;
  GameView *_view;
  id _activity;
  NSStatusItem *_statusItem;
}

// Icono de la barra de menus: una Pokeball en plantilla, para que siga el tema del sistema
static NSImage *statusIcon(void) {
  NSImage *img = [NSImage imageWithSize:NSMakeSize(18, 18) flipped:NO drawingHandler:^BOOL(NSRect r) {
    NSRect ball = NSInsetRect(r, 1.5, 1.5);
    NSBezierPath *edge = [NSBezierPath bezierPathWithOvalInRect:ball];
    edge.lineWidth = 1.5;
    [edge stroke];
    [[NSBezierPath bezierPathWithRect:NSMakeRect(NSMinX(ball), NSMidY(r) - 0.75, ball.size.width, 1.5)] fill];
    // la mitad de arriba rellena, y el boton del centro
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

static NSRect initialFrame(void) {
  NSUserDefaults *d = NSUserDefaults.standardUserDefaults;
  CGFloat size = [d doubleForKey:kPrefSize];
  if (size < kSizes[0] || size > kSizes[2]) size = DEFAULT_SIZE;
  if ([d objectForKey:kPrefOriginX]) {
    NSRect f = NSMakeRect([d doubleForKey:kPrefOriginX], [d doubleForKey:kPrefOriginY], size, size);
    for (NSScreen *s in NSScreen.screens)  // un monitor que ya no esta no se lleva la ventana
      if (NSIntersectsRect(s.visibleFrame, f)) return f;
  }
  NSRect vis = NSScreen.mainScreen.visibleFrame;
  return NSMakeRect(NSMaxX(vis) - size - 24, NSMinY(vis) + 24, size, size);
}

- (void)applicationDidFinishLaunching:(NSNotification *)note {
  NSMenu *bar = [[NSMenu alloc] init];  // sin esto no hay Cmd-Q en una app sin Dock
  NSMenu *appMenu = [[NSMenu alloc] init];
  [appMenu addItemWithTitle:@"Quit " APP_NAME action:@selector(terminate:) keyEquivalent:@"q"];
  [bar addItemWithTitle:@"" action:nil keyEquivalent:@""].submenu = appMenu;
  NSApp.mainMenu = bar;

  _window = [[RoundWindow alloc] initWithContentRect:initialFrame()
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
  _view = [[GameView alloc] initWithFrame:NSInsetRect(ring.bounds, RING, RING)];
  _view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  [ring addSubview:_view];
  _window.contentView = ring;
  [ring addTrackingArea:[[NSTrackingArea alloc]
                            initWithRect:NSZeroRect
                                 options:NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect
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

  if (!gConsole) [NSApp activateIgnoringOtherApps:YES];  // con consola, sin robarle el foco a quien prueba

  // Fuera del Dock y tapada, macOS la echaria a dormir (App Nap) y el juego se atrasaria
  _activity = [NSProcessInfo.processInfo beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep
                                                           reason:@"el juego corre en tiempo real"];
  [NSWorkspace.sharedWorkspace.notificationCenter addObserver:self
                                                     selector:@selector(willSleep:)
                                                         name:NSWorkspaceWillSleepNotification
                                                       object:nil];
  NSTimer *timer = [NSTimer timerWithTimeInterval:TICK_S target:self selector:@selector(tick:) userInfo:nil repeats:YES];
  timer.tolerance = TICK_S / 4;
  // tambien mientras se arrastra la ventana o esta abierto el menu
  [NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];
  // arrancar el dispositivo de audio puede tardar segundos: que no pare el juego
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{ startAudio(); });
}

- (void)tick:(NSTimer *)t {
  hostTick();
  if (hostFrameDirty()) [_view showFrame];
  if (hostRestartRequested()) {
    hostFlushSave();
    execv(NSBundle.mainBundle.executablePath.fileSystemRepresentation, gArgv);
    [NSApp terminate:nil];  // no deberia llegar aqui
  }
}

- (void)willSleep:(NSNotification *)note { hostFlushSave(); }
- (void)windowDidResignKey:(NSNotification *)note { [_view releaseKeys]; }
- (void)windowDidMove:(NSNotification *)note { [_view saveWindowPlace]; }
- (void)applicationWillTerminate:(NSNotification *)note { hostFlushSave(); }
@end

// Pruebas: "MOUSE down|drag|up|right x y" y "KEY down|up codigo", en coordenadas del lienzo, entran por la
// cola de eventos de AppKit como los reales, sin tocar el raton de verdad. "MENU n" acciona la entrada n del menu.
static void postTestEvent(const char *line) {
  char what[8] = "";
  int a = 0, b = 0;
  NSWindow *w = nil;
  for (NSWindow *candidate in NSApp.windows)
    if ([candidate isKindOfClass:RoundWindow.class]) w = candidate;
  if (sscanf(line, "MENU %d", &a) == 1) {
    NSMenu *menu = [[NSMenu alloc] init];
    [(GameView *)w.contentView.subviews.firstObject fillMenu:menu];
    [menu performActionForItemAtIndex:a];
    return;
  }
  if (sscanf(line, "MOUSE %7s %d %d", what, &a, &b) == 3) {
    CGFloat k = (w.frame.size.width - 2 * RING) / HOST_CANVAS_SIZE;
    NSEventType t = !strcmp(what, "down") ? NSEventTypeLeftMouseDown
                    : !strcmp(what, "drag") ? NSEventTypeLeftMouseDragged
                    : !strcmp(what, "right") ? NSEventTypeRightMouseDown : NSEventTypeLeftMouseUp;
    NSPoint p = NSMakePoint(RING + a * k, w.frame.size.height - RING - b * k);
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

// --console: lee comandos de la consola serie por stdin, como con el Knomi por USB
static void startConsole(void) {
  [NSThread detachNewThreadWithBlock:^{
    char line[256];
    while (fgets(line, sizeof line, stdin)) {
      if (!strncmp(line, "MOUSE ", 6) || !strncmp(line, "KEY ", 4) || !strncmp(line, "MENU ", 5)) {
        NSString *copy = @(line);
        dispatch_async(dispatch_get_main_queue(), ^{ postTestEvent(copy.UTF8String); });
      } else {
        hostSerialPush(line);
      }
    }
  }];
}

// Dos procesos sobre la misma partida se pisarian los guardados
static bool lockSave(NSString *saveDir) {
  [NSFileManager.defaultManager createDirectoryAtPath:saveDir withIntermediateDirectories:YES attributes:nil error:nil];
  int fd = open([saveDir stringByAppendingPathComponent:@".lock"].fileSystemRepresentation, O_CREAT | O_RDWR | O_CLOEXEC, 0644);
  return fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) == 0;
}

int main(int argc, char **argv) {
  gArgv = argv;
  @autoreleasepool {
    NSString *saveDir = NSProcessInfo.processInfo.environment[@"POKEMACGOCHI_SAVE_DIR"];
    if (!saveDir) {
      NSString *support = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES)[0];
      saveDir = [support stringByAppendingPathComponent:@APP_NAME];
    }
    if (!lockSave(saveDir)) {
      fprintf(stderr, APP_NAME " ya esta abierto\n");
      return 0;
    }
    hostInit(NSBundle.mainBundle.resourcePath.fileSystemRepresentation, saveDir.fileSystemRepresentation);
    for (int i = 1; i < argc; i++)
      if (strcmp(argv[i], "--console") == 0) { gConsole = true; startConsole(); }

    NSApplication *app = NSApplication.sharedApplication;
    app.activationPolicy = NSApplicationActivationPolicyAccessory;
    AppDelegate *delegate = [[AppDelegate alloc] init];
    app.delegate = delegate;

    // un kill normal tambien guarda
    signal(SIGTERM, SIG_IGN);
    dispatch_source_t term = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGTERM, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(term, ^{ [NSApp terminate:nil]; });
    dispatch_resume(term);

    [app run];
  }
  return 0;
}
