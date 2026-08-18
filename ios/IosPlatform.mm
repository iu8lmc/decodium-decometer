// Pezzo nativo iOS minimo: tiene lo schermo acceso mentre si guarda il
// quadrante. E' un ritaglio di decodium-mobile/androidapp/ios/IosPlatform.mm,
// che ha anche portachiavi e sessione audio — inutili qui, perche' questa app
// non ha ne' login al relay ne' audio da ricevere.
#import <UIKit/UIKit.h>

void iosSetIdleTimerDisabled(bool disabled)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [UIApplication sharedApplication].idleTimerDisabled = disabled ? YES : NO;
    });
}
