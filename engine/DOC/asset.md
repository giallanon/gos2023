### AssetID
Ogni asset ha un UID che non cambia mai nel tempo
    u32     crc32_relativePath_fileName
    u8      asset type: texture, shape...
    u8      not used
    u8      not used
    u8      not used


### AssetManager
baseFolder:
  * tutti i path sono relativi al baseFolder

stringTable:
  * utility, contiene tutte le stringhe che servono a AssetManager
  * u32 addString (const char *s)

assetID-to-filename:
  * hashTable<AssetID, offsetInStringTable>
  * const char* getFilename (const AssetID &id)

asset_import:
  * prende un asset di un certo tipo e lo importa all'interno di baseFolder assegnandogli un AssetID definitivo e creando i
    file necessari affinche l'asset sia caricabile da disco alla bisogna.
    Nel caso in cui una copia dell'asset in questione esisteva gia', allora AssetID viene mantenuto identico ma i file precedentemente
    creati sono eliminati e dei nuovi file sono creati basandosi sulla nuova versione dell'asset.
    E' imperativo che l'AssetID rimanga invariato rispetto al passato.

  * "Assets di base", sono asset non scomponibili, rappresentano l'unità minima e sono sempre caricabili da un singolo
    file, senza dipendenze da altri asset:
    - texture
    - shape

  * "Assets compositi", ovvero che sono composti almeno parzialmente da altri asset (quindi dipendono da altri asset):
    - material (linka a texture)
    - model (link a shape e material)

  * "Asset runtime", ovvero asset che sono creati alla bisogna a partire da un "Asset di base"
    - gpuTexture     ->  una texture caricata in memoria GPU
    - gpuShape       ->  una shape caricata in memoria GPU (VB/IB)
    - gpuMaterial?   ->  possibilmente in un UBO dinamico?

