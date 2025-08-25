### Hystory
2025-08-24  creazione documento




### intro
Nella gestione delle risorse, bisogna distinguere 2 attivita' che sono separate e distinte:

- build di una risorsa e creazione dei relativi file .gosres
- load a runtime di una .gosres


### build step (aka import asset into engine)
Un tool apposito si occupa di prendere dei file in input e produrre i necessari file .gosres
Lo stesso tool eventualmente puo' monitorare le directory per detectare delle modifiche nei file src e rebuildare le risorse associate.

Il motore di build cerca in resource/src tutti i file di tipo 



## struttura delle directory
- <BASE_FOLDER>
  - compiled      -> tutte le risorse buildate finiscono qui, che e' dove il runtime si aspetta di trovarle
  
  - raw           -> tutte le risorse di base, non buildate
    - shaders     -> i src degli shader in formato testo
    - images      -> png, jpg e quant'altro
    - models      -> glTF

  - src           -> contiene un elenco di gosresd (resource descriptor) che indicano cosa buildare
                     a partire da quanto esiste in /<BASE_FOLDER>/raw
                     Ad esempio, qui troviamo file per la descrizione delle pipeline, dellete texture e dei modelli 3d


### basic resource
u32 UID       crc di filename
u8  type

