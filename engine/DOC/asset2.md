### GOS ASSET 2 ###

## Struttura delle directory
- <BASE_FOLDER>
  - assets2.sqlite3   ->  e' il DB
	- asset_bin         -> tutti gli <asset> buildati e pronti all'uso finiscono qui, che e' dove il runtime si aspetta di trovarle
  - asset_src         -> e' dove stanno i file .gosasset_d che descrivono gli asset da buildare
  

## Resource
E' un file (testo, immagine, ...) non direttamente consumabile dall'engine.
Una o piu' risorse combinate in un certo modo danno origine ad un Asset.
Ad esempio:
  - uno o più file di testo possono dare origine ad un vtx/pxl shader in formato spv
  - una o più immagine (jpg, bmp, tga..) possono dare origine ad un texture, magari con mipmap, magari una texture 3D

## Virtual-asset
All'interno dei file di tipo .gosasset_d ci sono le descrizioni dei virtual-asset. Ogni virtual-asset può avere un runtime-name ad
esso associato.
Si chiamano virtual-asset perchè non tutti i virtual-asset corrispondono ad un diverso concrete-asset.
E' possibile per esempio descrivere lo stesso identico shader in file gosasset_d differenti, con runtime-name differenti.
Il builder si accorge di questa cosa e associa lo stesso concrete-asset ai 2 virtual-asset i quali, in sostanza, puntano allo stesso
concrete-asset anche se lo fanno utilizzando 2 diversi runtime-name.
Ai fini del runtime, caricare lo shader usando il runtime-name-1 o il runtime-name-2 equivale a caricare lo stesso identico concrete-shader

## Concrete-asset
E' l'unica risorsa direttamente consumabile dall'engine e viene creata dal processo di BUILD assemblando varie <Resource>.
Per creare un asset, è necessario creare una sezione all'interno di un file .gosasset_d.
Ogni <Asset> ha bisogno di un <AssetBuilder> e di un <AssetLoader> che si occupano rispettivamente di buildare la risorsa
durante il processo di build, e di caricarla a runtime durante il normale funzionamento dell'engine.


## RuntimeName
Ogni <asset> può avere uno o più runtimeName.
Un runtimeName è una stringa che può essere utilizzata dall'engine per caricare un asset.
I runtimeName sono definiti all'interno dei file .gosasset_d insieme alla dichiarazione del virtual-asset.
Diversi runtimeName possono puntare allo stesso asset.


## Asset UID
Ogni asset/risorsa/virtual-asset è identificato da un UID a 64bit.
I 32 bit LSB sono un CRC32 dei parametri di build dell'asset stesso oppure, nel caso delle risorse, sono un crc32 del filename.
I 32 bit MSB assumono il seguente significato (da MSB verso LSB):
  - 0x00 oppure 0x01        (0x01 solo se si tratta di virtual-asset)
  - asset::eAssetType       (valido sia per i concrete che per i virtual asset)
  - asset::eResType         (valido solo per le risorse)
  - 0x00
  
00 00 00 00    00 00 00 00
  | | |  |     |------------------------> CRC32
  | | |  |------------------------------> unused
  | | |---------------------------------> eResType
  | |-----------------------------------> eAssetType
  |-------------------------------------> 0x00 oppure 0x01 se virtual-asset




## File .gosasset_d e dipendeze tra asset
In generale, nel .h delle classi AssetBuilder c'è un esempio di dichiarazione di asset.
Fare riferimento a questi .h per la documentazione completa dei parametri mandatori e opzionali da utilizzare nella dichiarazione degli asset.

In generale comunque, una dichiarazione di asset ha la seguente forma:
@<asset_type>:  <optional runtimeAssetName>
{
  param1: 
  param2:
  ...
  paramN:

  @<optional asset_type>:  <mandatory runtimeAssetName>
  ...

  @<optional asset_type>:  <optional runtimeAssetName>
  {
    param1: 
    param2:
    ...
    paramN:
  }
  ...
}

Se un .gosasset_d ha bisogno di asset dichiarati in altri file .gosasset_d, li può includere usando la direttiva:
@include: <nome-del-file-gosasset_d>

Tutti i path all'interno di un gosasset_d sono da intendersi come path assoluti, oppure relativi al file gosasset_d stesso.

Una dichiarazione di asset può contenere a sua volta dichiarazioni di asset, ricorsivamente.
Un asset dichiarato all'interno di un altro asset può essere dichiarato in 2 modi:
  1- con un riferimento ad un runtimeName
  2- con una dichiarazione completa, come se lo avessi dichiarato al di fuori dell'asset padre
  

Nel primo caso, la dichiarazione ha la seguente sintassi:
  @<asset_type>:  <runtimeAssetName>
Durante il processo di build, il builder cerca un asset di nome <runtimeAssetName> e lo linka all'asset padre.
Se <runtimeAssetName> non esiste, il builder genera un errore.

Nel secondo caso, la dichiarazione è esattamente identica ad una dichiarazione "padre", ovvero necessita di una sezione
tra parentisi graffe con i parametri (ed eventualmente altri asset) necessari alla creazione dell'asset figlio.

Il builder è sufficientemente intelligente da riconoscere asset equivalenti per cui non genera asset distinti a meno che non sia assolutamente necessario.

Esempio 1:
    @vtx_shader: esempio_1
    {
      src: phong.vert.shader
    }

Questa dichiarazione genera un virtual-asset di tipo vtx_shader, con runtimeName "esempio_1", che si basa sulla risorsa "phong.vert.shader"

Esempio 2:
  @vtx_shader: esempio_2
  {
    src: phong.vert.shader
  }

Questa dichiarazione genera un virtual-asset di tipo vtx_shader, con runtimeName "esempio_2", che si basa sulla risorsa "phong.vert.shader"
Essendo che "esempio_1" e "esempio_2" sono identici, il builder genera un solo concrete-asset e gli associa entrambi i runtimeName.
Durante l'uso dell'engine, richiedere l'asset "esempio_1" oppure "esempio_2" produce lo stesso risultato: il concrete-asset caricato è sempre lo stesso.
Entrambi i runtimeName risolvono sullo stesso asset::UID.

Esempio 3:
    @vtx_shader: esempio_3
    {
      src: phong.vert.shader
      def: DEFINE1 DEFINE2 DEFINE3
    }
  Questa dichiarazione genera un virtual-asset di tipo vtx_shader, con runtimeName "esempio_3", che si basa sulla risorsa "phong.vert.shader".
  Durante la compilazione dello shader, le define "DEFINE1" "DEFINE2" "DEFINE3" vengono passate a glslc.
  Il risultato è un asset::UID diverso da quelli di esempio_1 ed esempio_2.

Esempio 4:
    @vtx_shader: esempio_4
    {
      src: phong.vert.shader
      def: DEFINE2 DEFINE3 DEFINE1
    }
Questo genera lo stesso identico asset::UID di "esempio_3", anche se le "define" non sono nello stesso ordine.


Esempio 5, 6, 7:
@pipeline_def : pipelinedef_5
{
  param: ..
  param: ..

  @vtx_shader: esempio_1
}

@pipeline_def : pipelinedef_6
{
  param: ..
  param: ..

  @vtx_shader
  {
    src: phong.vert.shader
  }
}

@pipeline_def : pipelinedef_7
{
  param: ..
  param: ..

  @vtx_shader : mio_shader_7
  {
    src: phong.vert.shader
  }
}

Questi 3 virtual-asset di tipo pipeline_def generano puntano tutti allo stesso concrete-asset e sono equivalenti tra di loro (posto che i "param" siano identici)
"pipelinedef_5" necessita dello shader "esempio_1".
"pipelinedef_6" crea uno shader inline ma, dato che la dichiarazione dello shader è equivalente a quella di "esempio_1", di fatto "pipelinedef_6" linka allo stesso shader::UID di esempio_1
"pipelinedef_7" è come "pipelinedef_6" ma, in più, aggiunge un ulteriore nuovo runtimeName allo shader "esempio_1".

Alla fine del processo di build, lo shader "esempio_1" ha 3 runtimeName: "esempio_1", "esempio_2" e "mio_shader_7"



### pensate per importazione model 3D ###

@imported-3dmodel: <rtname>
{
    (mandatory) src: ...xxx.glb                         => il modello 3d da importare
    (optional)	scale: [varie opzioni]
                        uniform-resize-y; <number>      => dato AABB del modello, riscala il modello in maniera uniforme affinchè la dimy di AABB sia esattamente uguale a <number>
                        uniform-resize-x; <number>
                        uniform-resize-z; <number>
                        
    (optional)	translate: [varie opzioni]
                            center-at; <x>; <y>; <z>            => dato AABB del modello, muove il centro dell'AABB alle coordinate x,y,z
                            bottom-center-at; <x>; <y>; <z>     => dato AABB del modello, muove il centro della faccia bottom dell'AABB alle coordinate x,y,z
}
Questo genera N asset di tipo shape, M asset di tipo Material (che a sua volta possono riferire ad asset di tipo Texture), J asset
di tipo Skeleton



@3dmodel: <rtname>
{
    src: <rtname-of-imported-3dmodel>  => crea un 3dmodel utilizzando tutte le shape/material/skeleton di un determinato <imported-3dmodel> 
}

@3dmodel: <rtname>
{
    shape:  <shape-name>;<rtname-of-imported-3dmodel>.<name>    => definisce una shape di nome <shape-name> presa dalla shape di nome <name> di un determinato <imported-3dmodel>
    ...
    shape: ...
    
    material: <material-name>;<rtname-of-imported-3dmodel>.<material-name>
    ...
    material: ...
    
    
    skeleton: none                          => vuol dire uno skeleton di default consistente del solo nodo root automaticamente generato
              <rtname-of-imported-3dmodel>     => lo sk di un determinato <imported-3dmodel>
    
    
    mesh: <shape-name>;<material-name>;<bone-name>;<local-transform-matrix3x3>
    ...
    mesh: ...
}