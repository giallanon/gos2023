Ogni risorsa creata dall'engine è associata ad un Handle sul quale si possono usare le seguenti azioni:
	-release
	-incRef
	-const* getData
	
- 	handle ha uno stato che rappresenta la disponibilità della risorsa verso l'esterno (loaded, notLoaded, error...)
	Questo stato viene manipolato solo da engine

-	handle ha anche uno stato che traccia la disponibilità del campo "data" della risorsa che rappresenta. Questo stato è ad uso interno
	del LoaderThread. Questo stato rappresenta lo stato interno di caricamento/scaricamento dell'asset; viene manipolato solo da LoaderThread
	E' perfettamente valido che ad un certo punto lo stato interno sia "loaded" e lo stato esterno sia "loading" per esempio.
	Vuol dire che il Loader ha finito il suo lavoro, ha comunicato all'engine la cosa, ma l'engine non ha ancora processato il messaggio e quindi
	per un utilizzatore esterno, la risorsa non e' ancora dispoibile all'uso
	 
-	handle ha un ResUID che può essere invalido se la risorsa non è stata caricata da un asset
	
-	il handle "mappa" una zona di memoria fissa e mai spostabile nella quale ci sono le info della risorsa che handle rappresenta.
	Ogni risorsa ha un campo "BaseResHandle brh;" (che contiene lo stato della risorsa) ed un campo "data" (che dipende dal tipo di risorsa e contiene in effetti i dati specifici della risorsa)

-	dato un ResUID, engine è in grado di ritornare un handle_as_u32 se l'asset è stato già caricato  (vedi internal__from_asset_to_handle())
	
-	dato un handle, engine è in grado di ritornare il resUID che rappresenta (da handle a "resource" e poi BaseResHandle->uid)


Alla richiesta di creazione di una risorsa, 
	- engine riserva preventivamente la zona di memoria associata alla risorsa
	- memorizza la relazione <handle, resUID>
	- lo stato esterno di handle è notReady
	- se la risorsa è un asset, passa la palla al loader
	- se la risorsa non è un asset, la crea e lo stato esterno di handle diventa ready



========== gestione delle dipendenze per Handle con asset multipli ========

Es:
	model_instance dipende da un model3d

	model3d dipende da
		-skeleton
		-gpushapes
		-material

Solo quando tutte le dipendenze sono in stato "ready" allora anche il padre e' ready.
Serve un meccanismo che, al cambiare dello stato di una risorsa, sia in grado di notificare questo cambio a tutte le risorse che dipendono da essa