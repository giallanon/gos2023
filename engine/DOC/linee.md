### Line3D
Semplicemente una linea tra 2 punti in 3D, con uno spessore fixed in pixel.
I vertici vengono trasformati dalla camera e la linea è un "quad" istanziato.
La linea ha i seguenti parametri:
    spessore (in pixel, fisso, la linea sarà sempre larga tot pixel)
    colore start-end
    texture
    tu-tv offset: per animare la texture. La mesh istanziata ha le solite coordinate 0,0 1,1. tu-tv offset viene addato in realtime

?? zbuffer?     idealmente dovrei poter decidere linea per linea se zbuffer on-off
?? line-list?   se esistesse un modo per