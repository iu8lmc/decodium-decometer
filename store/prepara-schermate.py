"""Porta le catture del telefono al formato che il Play Store accetta.

Stessa soluzione gia' collaudata su decodium-mobile, con i colori di questa
app.

Il problema: l'OPPO cattura 720x1604 in piedi (2,23 a 1) e 1604x720 coricato.
Il negozio vuole schermate non piu' allungate di 9:16, cioe' 1,78 a 1, e una
cattura fuori formato viene rifiutata al caricamento — dopo che si e' compilata
tutta la scheda.

La soluzione NON e' ritagliare: taglierebbe via proprio il quadrante o la
riga delle letture, cioe' quello che si vuole mostrare. Si aggiungono invece
due bande dello stesso fondo dell'app, e l'immagine resta intera dentro una
cornice che sembra parte del disegno.

Uso:
    python store/prepara-schermate.py cattura1.png cattura2.png ...

Le immagini pronte finiscono in store/schermate/.
"""
import os
import sys

from PIL import Image

# Il fondo del frontalino (Decometer.qml: colPanel e la finestra di Main.qml).
BG_ALTO = (20, 24, 29)      # #14181D
BG_BASSO = (11, 14, 18)     # #0B0E12

RAPPORTO_MAX = 16 / 9        # lato lungo / lato corto consentito al massimo
USCITA = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'schermate')


def sfondo(w, h):
    im = Image.new('RGB', (w, h))
    px = im.load()
    for y in range(h):
        t = y / max(1, h - 1)
        riga = tuple(int(BG_ALTO[i] + (BG_BASSO[i] - BG_ALTO[i]) * t) for i in range(3))
        for x in range(w):
            px[x, y] = riga
    return im


def incornicia(percorso):
    im = Image.open(percorso).convert('RGB')
    w, h = im.size
    orizzontale = w > h

    if orizzontale:
        # coricata: si allarga in altezza se e' troppo bassa
        largo, alto = w, max(h, int(round(w / RAPPORTO_MAX)))
    else:
        # in piedi: si allarga in larghezza se e' troppo stretta
        largo, alto = max(w, int(round(h / RAPPORTO_MAX))), h

    if (largo, alto) == (w, h):
        fuori = im
    else:
        fuori = sfondo(largo, alto)
        fuori.paste(im, ((largo - w) // 2, (alto - h) // 2))

    if not os.path.isdir(USCITA):
        os.makedirs(USCITA)
    nome = os.path.splitext(os.path.basename(percorso))[0] + '-play.png'
    destinazione = os.path.join(USCITA, nome)
    fuori.save(destinazione)
    rapporto = max(fuori.size) / min(fuori.size)
    print('  %-28s %s -> %s   (%.2f:1)%s'
          % (os.path.basename(percorso), (w, h), fuori.size, rapporto,
             '' if rapporto <= RAPPORTO_MAX + 0.01 else '   ANCORA FUORI FORMATO'))
    return destinazione


if len(sys.argv) < 2:
    print(__doc__)
    sys.exit(1)

print('schermate preparate:')
for p in sys.argv[1:]:
    incornicia(p)
print()
print('cartella:', USCITA)
