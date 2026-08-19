"""Grafica per la scheda del Play Store: icona 512 e immagine in evidenza.

Non e' un lavoro di grafica a parte: riusa lo STESSO disegno dell'icona
dell'app (make_icon.py), perche' chi vede la scheda sul negozio e chi vede
l'icona sul telefono devono riconoscere lo stesso oggetto. I colori vengono
di la', che a loro volta vengono da Decometer.qml.

    python tools/make_store_art.py

Produce in store/:
    icona-512.png                  l'icona alta risoluzione della scheda
    in-evidenza-1024x500-it.png    l'immagine in cima alla scheda, italiano
    in-evidenza-1024x500-en.png    la stessa, per la scheda in inglese
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_icon import COL_AMBRA, COL_CIANO, COL_FONDO_1, COL_FONDO_2, COL_INK, disegna

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
USCITA = os.path.join(RADICE, "store")

# Il grigio delle diciture minori del frontalino (#8A939C).
COL_LABEL = (138, 147, 156)


def font(dim, grassetto=False):
    # I font di sistema di Windows: se non ci sono si ripiega su quello
    # incorporato, che e' brutto ma non blocca la generazione.
    for nome in (["segoeuib.ttf", "arialbd.ttf"] if grassetto
                 else ["segoeui.ttf", "arial.ttf"]):
        percorso = os.path.join(os.environ.get("WINDIR", r"C:\Windows"), "Fonts", nome)
        if os.path.exists(percorso):
            return ImageFont.truetype(percorso, dim)
    return ImageFont.load_default()


def sfondo(larghezza, altezza):
    # La stessa sfumatura del pannello del frontalino, in verticale.
    img = Image.new("RGB", (larghezza, altezza), COL_FONDO_1)
    d = ImageDraw.Draw(img)
    for y in range(altezza):
        t = y / max(1, altezza - 1)
        d.line(
            [(0, y), (larghezza, y)],
            fill=tuple(int(COL_FONDO_2[i] + (COL_FONDO_1[i] - COL_FONDO_2[i]) * t)
                       for i in range(3)),
        )
    return img


def feature(righe):
    L, A = 1024, 500
    img = sfondo(L, A)
    d = ImageDraw.Draw(img)

    # L'icona a sinistra, la scritta a destra: il ritaglio che il negozio fa
    # su alcuni formati taglia i bordi, quindi niente di essenziale ai margini.
    lato = 300
    icona = disegna(lato).convert("RGBA")
    img.paste(icona, (72, (A - lato) // 2), icona)

    x = 72 + lato + 68
    # DEC(O barrata)METER, con la barra ciano come nel frontalino.
    f_nome = font(74, grassetto=True)
    y = 150
    for pezzo, colore in (("DEC", COL_INK), ("\u00d8", COL_CIANO), ("METER", COL_INK)):
        d.text((x, y), pezzo, font=f_nome, fill=colore)
        x += int(d.textlength(pezzo, font=f_nome))

    d.text((72 + lato + 68, y + 96), righe[0], font=font(30), fill=COL_LABEL)
    d.text((72 + lato + 68, y + 138), righe[1], font=font(30), fill=COL_LABEL)

    # Una riga ambra corta sotto il testo: il colore dell'allarme del
    # quadrante, usato qui solo come firma visiva.
    d.rectangle([72 + lato + 68, y + 196, 72 + lato + 68 + 96, y + 200],
                fill=COL_AMBRA)
    return img


def main():
    os.makedirs(USCITA, exist_ok=True)
    disegna(512).save(os.path.join(USCITA, "icona-512.png"))
    # Due lingue, come per l'app sorella: la scheda del negozio si compila in
    # italiano e in inglese, e un'immagine in evidenza in una lingua sola
    # stona nell'altra scheda.
    feature(["Potenza, ROS e ALC della radio",
             "dal PC al telefono, in rete locale"]).save(
        os.path.join(USCITA, "in-evidenza-1024x500-it.png"))
    feature(["Forward power, SWR and ALC",
             "from your PC to your phone, over WiFi"]).save(
        os.path.join(USCITA, "in-evidenza-1024x500-en.png"))
    print("scritti:")
    for nome in ("icona-512.png", "in-evidenza-1024x500-it.png",
                 "in-evidenza-1024x500-en.png"):
        p = os.path.join(USCITA, nome)
        print(f"  {p}  {os.path.getsize(p)} byte")


if __name__ == "__main__":
    main()
