### GOS ASSET ###

## Struttura delle directory
- <BASE_FOLDER>
	- res
		- 01-shader_txt   -> ogni risorsa e' identificata da un byte (vedi eResType) e qui viene rappresentata
		- 02-img             con una sua directory che inizia con l'id della risorsa, seguito dal nome del tipo di risorsa
    - ,,,

	- assets
		- src			        -> contiene gli script con estensione .gosres_d che contengono le istruzioni per buildare un asset. Sono ammesse sottodirectory.
  	- bin			        -> tutti gli <asset> buildati e pronti all'uso finiscono qui, che e' dove il runtime si aspetta di trovarle
  

## Asset UID
Ogni asset è identificato da un UID a 64bit.
I 32 bit LSB sono un CRC32 dei parametri di build dell'asset stesso.
I 32 bit MSB assumono il seguente significato (da MSB verso LSB):
  - 0
  - asset::eAssetType
  - asset::eResType
  - 0
  
00 00 00 00    00 00 00 00
    | |  |     |------------------------> CRC32
    | |  |------------------------------> depth
    | |---------------------------------> eResType
    |-----------------------------------> eAssetType


## Resource
E' un file (testo, immagine, ...) non direttamente consumabile dall'engine.
Una o piu' risorse combinate in un certo modo danno origine ad un Asset.
Ad esempio:
  - uno o più file di testo possono dare origine ad un vtx/pxl shader in formato spv
  - una o più immagine (jpg, bmp, tga..) possono dare origine ad un texture, magari con mipmap, magari una texture 3D

I file risorsa non hanno dipendenze (ci sono un paio di eccezione, vedi sotto) il che significa che sono sostanzialmente
i mattoncini grazie ai quali vengono costruiti gli asset.

Tutte le risorse note sono enumerate in <asset::eResType>.
Le risorse di tipo <eResType::shader_txt> sono dei file di testo con il src degli shader. Dato che in questi file ci possono
essere delle #include, di fatto questi file di risorsa possono avere delle dipendenze da altri file. La cosa è gestita
in maniera trasparente da <asset::Builder>.

Risorse un po' particolari sono le risorse di tipo <eResType::gosasset_d>.
Questi file sono degli IniFile che contengono le istruzioni per il build degli asset.
In sostante, <asset::Builder> cerca tutti i file con estensione .gosasset_d e li parsa; a partire dalle sezioni che trova in
questi file, builda i vari tipi di asset.


## Asset
E' una risorsa direttamente consumabile dall'engine e viene creata dal processo di BUILD assemblando varie <Resource>.
Per creare un asset, è necessario creare una sezione all'interno di un file .gosasset_d.
Ogni <Asset> ha bisogno di un <AssetBuilder> e di un <AssetLoader> che si occupano rispettivamente di buildare la risorsa
durante il processo di build, e di caricarla a runtime durante il normale funzionamento dell'engine.


## RuntimeName
Ogni <asset> può avere uno o più runtimeName.
Un runtimeName è una stringa che può essere utilizzata dall'engine per caricare un asset.
I runtimeName sono definiti all'interno dei file .gosasset_d insieme alla dichiarazione dell'asset stesso.
Diversi runtimeName possono puntare allo stesso asset.



## File .gosasset_d e dipendeze tra asset
In generale, nel .h delle classi AssetBuilder c'è un esempio di dichiarazione di asset.
Fare riferimento a questi .h per la documentazione completa dei parametri mandatori e opzionali
da utilizzare nella dichiarazione degli asset.

NB: al termine del processo di build, nella cartella /assets/src il BUILDER crea i file "__build.gosasset_d" e "__dependencies.txt" che
    contengono informazioni circa le dipendenze e gli asset creati

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


Una dichiarazione di asset può quindi contenere a sua volta dichiarazioni di asset, ricorsivamente.
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

Questa dichiarazione genera un Asset di tipo vtx_shader, con runtimeName "esempio_1", che si basa sulla risorsa "phong.vert.shader"

Esempio 2:
  @vtx_shader: esempio_2
  {
    src: phong.vert.shader
  }

Questa dichiarazione genera un Asset di tipo vtx_shader, con runtimeName "esempio_2", che si basa sulla risorsa "phong.vert.shader"
Essendo che "esempio_1" e "esempio_2" sono identici, il builder genera un solo asset e gli associa entrambi i runtimeName.
Durante l'uso dell'engine, richiedere l'asset "esempio_1" oppure "esempio_2" produce lo stesso risultato: l'asset caricato è sempre lo stesso.
Entrambi i runtimeName risolvono sullo stesso asset::UID.

Esempio 3:
    @vtx_shader: esempio_3
    {
      src: phong.vert.shader
      def: DEFINE1 DEFINE2 DEFINE3
    }
  Questa dichiarazione genera un Asset di tipo vtx_shader, con runtimeName "esempio_3", che si basa sulla risorsa "phong.vert.shader".
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

Questi 3 asset di tipo pipeline_def generano tutti lo stessi UID e sono equivalenti tra di loro (posto che i "param" siano identici)
"pipelinedef_5" necessita dello shader "esempio_1".
"pipelinedef_6" crea uno shader inline ma, dato che la dichiarazione dello shader è equivalente a quella di "esempio_1", di fatto "pipelinedef_6" linka allo stesso shader::UID di esempio_1
"pipelinedef_7" è come "pipelinedef_6" ma, in più, aggiunge un ulteriore nuovo runtimeName allo shader "esempio_1".

Alla fine del processo di build, lo shader "esempio_1" ha 3 runtimeName: "esempio_1", "esempio_2" e "mio_shader_7"
