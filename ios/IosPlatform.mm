// Pezzo nativo iOS minimo: tiene lo schermo acceso mentre si guarda il
// quadrante. E' un ritaglio di decodium-mobile/androidapp/ios/IosPlatform.mm,
// che ha anche portachiavi e sessione audio — inutili qui, perche' questa app
// non ha ne' login al relay ne' audio da ricevere. L'unica aggiunta e' la
// vibrazione, per l'allarme di ROS alto.
#import <UIKit/UIKit.h>
// AudioToolbox serve solo per la vibrazione dell'allarme di ROS alto.
#import <AudioToolbox/AudioToolbox.h>

void iosSetIdleTimerDisabled(bool disabled)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [UIApplication sharedApplication].idleTimerDisabled = disabled ? YES : NO;
    });
}

// Vibrazione: su iOS non si sceglie la durata, si chiede al sistema di
// vibrare e basta. E' il corrispettivo del beep di un misuratore da tavolo,
// per un allarme che deve arrivare anche a schermo spento o in tasca.
void iosVibra()
{
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
}

// I margini che il sistema si tiene per se': l'incavo in alto, la barra
// gestuale in basso, e i lati quando il telefono e' coricato. Qui li da' la
// finestra attiva; su Android bisogna invece chiederli alla View, dal suo
// thread (vedi la nota in MeterBridge.cpp).
void iosSafeAreaInsets(double* top, double* bottom, double* left, double* right)
{
    if (top) *top = 0; if (bottom) *bottom = 0;
    if (left) *left = 0; if (right) *right = 0;
    UIWindow* w = nil;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:[UIWindowScene class]]) continue;
        for (UIWindow* candidate in ((UIWindowScene*) scene).windows) {
            if (candidate.isKeyWindow) { w = candidate; break; }
        }
        if (w) break;
    }
    if (!w) w = UIApplication.sharedApplication.windows.firstObject;
    if (!w) return;
    UIEdgeInsets const in = w.safeAreaInsets;
    if (top)    *top    = in.top;
    if (bottom) *bottom = in.bottom;
    if (left)   *left   = in.left;
    if (right)  *right  = in.right;
}
