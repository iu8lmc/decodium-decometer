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
