"""Disegna l'icona di Decometer: un misuratore ad arco con l'ago.

Non e' un disegno "artistico": e' lo stesso strumento che l'app mostra, ridotto
a icona. Arco graduato, tacche piu' fitte verso il fondo scala, ago fermo poco
oltre meta' corsa (dove sta un misuratore che sta leggendo qualcosa, non uno
spento a zero e nemmeno uno in allarme), e i colori del frontalino: fondo
scuro, arco ciano, settore alto ambra come sul quadrante vero.

Rigenera tutte le misure che servono ad Android e iOS:
    python tools/make_icon.py
"""
import math
import os

from PIL import Image, ImageDraw

# Colori presi da Decometer.qml, non scelti a occhio: cosi' l'icona e lo
# strumento sono lo stesso oggetto.
COL_FONDO_1 = (11, 14, 18)      # #0B0E12
COL_FONDO_2 = (20, 24, 29)      # #14181D
COL_CIANO = (39, 196, 212)      # #27C4D4
COL_AMBRA = (255, 180, 84)      # #FFB454
COL_INK = (232, 236, 239)       # #E8ECEF
COL_TACCA = (91, 102, 112)      # #5B6670

LATO = 1024


def disegna(lato=LATO):
    # Si disegna 4 volte piu' grandi e si rimpicciolisce: e' il modo piu'
    # semplice per avere bordi puliti senza antialiasing manuale.
    s = lato * 4
    img = Image.new("RGBA", (s, s), COL_FONDO_1)
    d = ImageDraw.Draw(img)

    # Fondo: un quadrato con angoli arrotondati leggermente piu' chiaro al
    # centro, come il pannello dello strumento.
    d.rounded_rectangle([0, 0, s - 1, s - 1], radius=int(s * 0.22), fill=COL_FONDO_2)

    # Il quadrante sta dentro un margine abbondante: sui lanciatori Android
    # l'icona adattiva viene ritagliata a cerchio o a goccia, e un arco che
    # tocca i bordi ci rimette le punte. Meglio piccolo e intero che grande e
    # mozzato.
    cx, cy = s / 2, s * 0.70          # centro dell'arco, in basso
    raggio = s * 0.40
    a0, a1 = 208.0, 332.0             # apertura del quadrante, in gradi

    def punto(ang_deg, r):
        a = math.radians(ang_deg)
        return (cx + r * math.cos(a), cy + r * math.sin(a))

    # Arco principale, spesso. Il tratto finale e' ambra: e' la zona che su un
    # misuratore vuol dire "attenzione", e si riconosce al volo anche a 48 px.
    largh = s * 0.045
    d.arc([cx - raggio, cy - raggio, cx + raggio, cy + raggio],
          a0, a0 + (a1 - a0) * 0.72, fill=COL_CIANO, width=int(largh))
    d.arc([cx - raggio, cy - raggio, cx + raggio, cy + raggio],
          a0 + (a1 - a0) * 0.72, a1, fill=COL_AMBRA, width=int(largh))

    # Tacche: piu' fitte verso il fondo scala, come su uno strumento vero.
    n = 11
    for i in range(n):
        t = i / (n - 1.0)
        ang = a0 + (a1 - a0) * (t ** 0.85)
        lunga = (i % 5 == 0)
        r1 = raggio - largh * 0.85
        r2 = r1 - (s * 0.075 if lunga else s * 0.042)
        col = COL_INK if lunga else COL_TACCA
        p1, p2 = punto(ang, r1), punto(ang, r2)
        d.line([p1, p2], fill=col, width=int(s * (0.013 if lunga else 0.008)))

    # L'ago: fermo poco oltre meta' corsa. Un ago a zero direbbe "strumento
    # spento", uno a fondo scala "stazione in allarme": ne' l'una ne' l'altra
    # sono l'idea giusta per l'icona di un misuratore che funziona.
    ang_ago = a0 + (a1 - a0) * 0.60
    base = s * 0.055
    pa = punto(ang_ago, raggio - largh * 1.5)
    # triangolo: largo al perno, appuntito in cima
    perp = math.radians(ang_ago + 90)
    bx, by = math.cos(perp) * base / 2, math.sin(perp) * base / 2
    d.polygon([pa, (cx + bx, cy + by), (cx - bx, cy - by)], fill=COL_INK)

    # Perno
    r_perno = s * 0.052
    d.ellipse([cx - r_perno, cy - r_perno, cx + r_perno, cy + r_perno],
              fill=COL_INK)
    r_int = s * 0.024
    d.ellipse([cx - r_int, cy - r_int, cx + r_int, cy + r_int], fill=COL_FONDO_1)

    return img.resize((lato, lato), Image.LANCZOS)


def main():
    qui = os.path.dirname(os.path.abspath(__file__))
    radice = os.path.dirname(qui)
    grande = disegna(LATO)

    # ---- iOS: una sola immagine da 1024, il resto lo ricava Xcode
    ios = os.path.join(radice, "ios", "Assets.xcassets", "AppIcon.appiconset",
                       "icon-1024.png")
    grande.convert("RGB").save(ios)
    print("iOS  :", ios)

    # ---- Android: launcher classico + livello di primo piano per l'icona
    # adattiva. Il primo piano va disegnato piu' piccolo dentro la sua tela:
    # Android ne ritaglia i bordi per dargli la forma del lanciatore (cerchio,
    # goccia, quadrato), e quello che sta fuori dal 66% centrale sparisce.
    misure = {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}
    for nome, px in misure.items():
        cartella = os.path.join(radice, "android", "res", "mipmap-" + nome)
        os.makedirs(cartella, exist_ok=True)
        grande.resize((px, px), Image.LANCZOS).save(
            os.path.join(cartella, "ic_launcher.png"))
        grande.resize((px, px), Image.LANCZOS).save(
            os.path.join(cartella, "ic_launcher_round.png"))

        tela = Image.new("RGBA", (px, px), (0, 0, 0, 0))
        dentro = int(px * 0.62)
        tela.paste(grande.resize((dentro, dentro), Image.LANCZOS),
                   ((px - dentro) // 2, (px - dentro) // 2))
        tela.save(os.path.join(cartella, "ic_launcher_foreground.png"))
        print("Android %-7s %dpx" % (nome, px))

    anteprima = os.path.join(qui, "icona_anteprima.png")
    grande.save(anteprima)
    print("anteprima:", anteprima)


if __name__ == "__main__":
    main()
