### Resource
E' una risorsa non direttamente consumabile dall'engine.
Una o piu' risorse combinate in un certo modo danno origine ad un Asset
Es di risorse sono:
	- file di testo per gli shader
	- immagini in vario formato


### Asset
E' una risorsa direttamente consumabile dall'engine e viene creata dal processo di BUILD assemblando varie <Resource>.
Es di <asset> sono:
	- shader compilati in formato spv gia' pronte per la GPU, assemblati a partire da <resource> di tipo testo
	- texture comprensive di eventuali mip-map, 3d texture e quant'altro, assemblate a partire da <resource> di tipo immagine



### struttura delle directory
- <BASE_FOLDER>
	- res
		- 01-shader_txt   -> ogni risorsa e' identificata da un byte (vedi eResType) e qui viene rappresentata
		- 02-img             con una sua directory che inizia con l'id della risorsa, seguito dal nome del tipo di risorsa
    - ,,,

	- assets
		- src			        -> contiene gli script con estensione .gosres_d che contengono le istruzioni per buildare un asset. Sono ammesse sottodirectory.
  	- bin			        -> tutti gli <asset> buildati e pronti all'uso finiscono qui, che e' dove il runtime si aspetta di trovarle
  


### dipendenza tra assets

@pipeline_def: <optional_runtimeAssetName = pippo>
{
  param...                          |                       |
  ...                               |                       |
  param...                          |                       |
                                    |                       |
  @vtx_shader                       |                       |
  {                                 |                       |
    src:            ex4.vert        |-->  <assetUID = 1234> |--->  <assetUID = 9273>
    define:         pippo pluto     |                       |
  }                                 |                       |
                                    |                       |
  @pxl_shader                       |                       |
  {                                 |                       |
    src:            ex4.frag        |-->  <assetUID = 4567> |
    define:                         |
  }                                 |
}

<runtimeAssetName = pippo> punta all'asset <assetUID = 9273>
  - <assetUID = 9273> e' una risorsa di tipo <pipeline_def> la quale dipende da:
    - <assetUID = 1234> che dipende da
      - <res_shader_txt = ex4.vert>
    - <assetUID = 4567> che dipende da
      - <res_shader_txt = ex4.frag>


<!-- creo una texture2D usando brick.jpg e generando tutte le mipmap. La texure finale e' un file di tipo gosImage -->
@texture2D: <optional_runtimeAssetName = brick_wall>
{
  image:  brick.jpg         |
  mipmap: auto              |-> <assetUID = 6534>
  format: <eImageFormat>    |
}

<runtimeAssetName = brick_wall> punta al <assetUID = 6534>
  - <assetUID = 6534> dipende da:
    - <res_image = brick,jpg>


<!-- creo una shape che viene estratta dal un modello.glTF e alla quale applico un preciso vtxFormat -->
@shape : <optional_runtimeAssetName = scudo>
{
  src:        sponza.glTF:<nome_mesh_nel_modello.glTF>  |
                                                        |- <assetUID = 278>
  vtxFormat:  ...                                       |
}
<runtimeAssetName = scudo> punta a <assetUID = 278>
  - <assetUID = 278> dipende da
    - <res_model3D = sponza.glTF>

<!-- creo un asset di tipo vtx_shader -->
@vtx_shader : <optional_runtimeAssetName = vbPhong_default>
{
  src: vbPhong.vert   |
                      |-> <assetUID = 1934>
  define: ...         |
}
<runtimeAssetName = vbPhong_default> punta a <assetUID = 1934>
  - <assetUID = 1934> dipende da
    - <res_shader_txt = vbPhong.vert>

<!-- creo una pipelinde_def che dipende dall'asset <runtimeAssetName = vbPhong_default> -->
@pipeline_def: <optional_runtimeAssetName = phong>
{
  param...                          |                         |
  ...                               |                         |
  param...                          |                         |
                                                              |
  @vtx_shader: vbPhong_default      |-->  <assetUID = 1934>   |
                                                              |-->  <assetUID = 8732>
  @pxl_shader                       |                         |
  {                                 |                         |
    src:            ex4.frag        |-->  <assetUID = 4567>   |
    define:                         |
  }                                 |
}
<runtimeAssetName = phong> punta a <assetUID = 8732>
  - <assetUID = 8732> dipende da
    - <assetUID = 1934> che dipende da
      - vedi descrizion dell'asset <vbPhong_default>
    - <assetUID = 4567> che dipende da
      - <res_shader_txt = ex4.frag>



### build strategies
Se un file di tipo <res_xxx> viene modificato, allora bisogna cercare nel DB tutti gli asset che dipendono da quella risorsa e rebuildarli.
Avendo rebuildato alcuni asset, bisogna cercare tutti gli asset che dipendono dall'asset rebuildato e rebuildare pure loro, ricorsivamente.

Se un <runtimeAssetName> cambia, nel senso che punta ad una nuovo asset, allora bisogna creare il nuovo <asset>, associarlo al vecchio <runtimeAssetName>
e poi bisogna cercare tutti gli <asset> che dipendono dal <runtimeAssetName> e rebuildarli.

Se un <res_xxx> viene eliminato (cosa difficile da detectare), bisogna invalidare tutti gli asset che lo referenziano e, ricorsivamente, tutti
quelli che referenziano quelli invalidati.

Se un <runtimeAssetName> viene eliminato (cosa difficile da detectare), bisogna invalidare tutti gli asset che lo referenziano e, ricorsivamente, tutti
quelli che referenziano quelli invalidati.

--------------
1-  Collezionare le date di ultima modifica di tutte le <res> per verificare quale di queste sono state modificate dall'ultimo build.
    Come output ho una lista di <res> modificate rispetto all'ultimo build

2-  Per ogni risorsa modificata:
    - elencare gli <asset> che dipendono da questa. Questi <asset> devono essere sicuramente rebuildati


