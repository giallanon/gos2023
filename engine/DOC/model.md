### VB/IB manager
- crea VB/IB ad hoc e ha dei metodi per riservare spazio in VB/IB


### Shape
- una collezione di vtx/idx con un determinato VtxFormat
Quello che mi interessa davvero e' che questi vtx/idx siano bindati da qualche parte in VB/IB
e che io abbia accesso a quel VB/IB/numPrimitive.
La shape di per se mi serve solo caricarla per infilarla in un VB/IB, poi la posso discardare

### GPUShape
- handler? che punta ad una shape già infilata in VB/IB
  contiene handler di VB, handler di IB e numPrimitive

### material
- dipende dal renderer ma in generale ha un tot di parametri
  come colori e GPUTexture

### materialOwner
- mantiene tutti gli handle dei <material> esistenti




### skeletonOwner
- mantiene tutti gli handle degli <skeleon> esistenti


### modellOwner
- mantiene tutti gli handle dei <model> esistenti

### shapeOwner
- mantiene tutti gli handle delle <shape> esistenti


### Skeleton
- come minimo ha root
- da root partono nodi figli e figli di figli
- ogni nodo ha la sua matrice relativa di poszione/rotazione

### Skeleton instance
- ha uno <skeletonPadre>
- ha il suo set locale di trasformazioni derivato dallo Skeleton padre


### Model
- ha uno <skeleton> (che può essere anche banale, ovvero solo con root)
- ha N1 <GPUShape>
- ha N2 <material>
- ha un array di N3 elementi che associa <GPUShape, material, skeleton-bone>
    In sostanza posso riutilizzare una <GPUShape> attaccandola a un qualune <skeleton-bone> e associandole un <material>
    Allo stesso <skeleton-bone> posso associare molte <GPUShape>.
    La stessa <GPUShape> può essere associata a diversi <skeleton-bone>
    Anche i <material> possono essere associati liberamente
  

### ModelInstance
- ha un <ModelPadre>
- ha uno skeleton-instance creato a partire da <ModelPadre::skeleton>
- utilizza le stesse <GPUShape> di <ModelPadre>
- utilizza gli stti <material> di <ModelPadre> a meno che non ne venga fatto un override nel qual caso
  il materiale sostituisce quello originale

