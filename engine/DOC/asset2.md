### stuttura della directory
[resources]
  - qui ci sono tutte le risorse "raw", tipo immagini, modelli3d da importare, shader da compilare...

[assets]
  -qui ci sono gli asseti compilati, digeribili nativamente dall'engine senza che debbano essere
   processati ulteriormente
  [texture]
  [material]
  [shader]
  [model3D]
  [...]


### AssetID
  u32 uid
  u8  unused
  u8  assetType
  u16 sub-resource-id

L'idea è che le risorse che producono + di 1 asset, condivino <uid> ma variano di <assetType> e <sub-resource-id> in modo che, dato
un UID, posso velocemente trovare tutti i sotto AssetID che ne derivano

### AssetRegistry
Contiene la lista degli asset esistenti
Ogni Asset ha un UID e un friendly name (derivato dal nome del file da cui origina)


### Raw resources to Asset
Classi specializzate prendono una o + risorse e le trasformano in un asset.
Nel far questo, creano anche un file .import che indica l'UID di destinazione, il friendly name e i parametri utilizzati per la creazione dell'asset
in modo da poter re-importare l'asset (hot reload)


### Model3D
input:    un file .glb e le relative immagini (texture)
output:   
  1 file gosModel con le shape
  N asset di tipo Texture       (shared, controlla se esistono gia ed eventualmente non le rilavora. Diventano un asset slegato dal modello)
  N asset di tipo MaterialPBR 
  1 file gosSkeleton (?)        (shared, controlla se esistono gia ed eventualmente non le rilavora. Diventano un asset slegato dal modello)
  N file gosAnimation (?)       (shared, controlla se esistono gia ed eventualmente non le rilavora. Diventano un asset slegato dal modello)
  1 file .import che elenca il file glb sorgente e l'UID che gli è stato associato



