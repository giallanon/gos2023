#include "Model.h"

using namespace gos;



//********************************
Model::Model ()
{
    localAllocator = NULL;
    flag.zero();
}

//********************************
void Model::freeAll()
{
    if (NULL == localAllocator)
        return;

    nodeList.unsetup ();
    meshList.unsetup();
    nodeNameIndexList.unsetup ();
    nodeNameList.unsetup ();

    localAllocator = NULL;
}

//********************************
void Model::begin (gos::Allocator *allocator, u32 startingNumNodes, u32 startingNumOfMesh)
{
    assert (NULL == localAllocator);
    assert (NULL != allocator);
    assert (startingNumNodes > 0);
    assert (startingNumOfMesh > 0);

    localAllocator = allocator;
    flag.zero();
    flag.set (FLAG__BUILD_MODE);

    nodeList.setup (localAllocator, startingNumNodes);
    meshList.setup (localAllocator, startingNumOfMesh);
    nodeNameIndexList.setup (localAllocator, startingNumNodes);
    nodeNameList.setup (localAllocator, 32*startingNumNodes);

    //il nodo di root lo addo di default
    mat4x4f matW;
    matW.identity();
    priv_newNode (matW, "root");
}

//********************************
u16 Model::priv_newNode (const gos::mat4x4f &localMatrix, const char *name)
{
    assert (flag.isBitSet(FLAG__BUILD_MODE));

    const u32 i = nodeList.getNElem();
    nodeList[i].localMatrix = localMatrix;
    nodeList[i].indexOf_firstChild = u16MAX;
    nodeList[i].indexOf_firstSibling = u16MAX;
    nodeList[i].indexOf_firstMesh = 0;
    nodeList[i].numMesh = 0;

    //addo il nome alla lista dei nomi dei nodi
    assert (nodeNameIndexList.getNElem() == i);
    if (NULL != name)
        nodeNameIndexList[i] = nodeNameList.add (name);
    else
    {
        char defaultName[32];
        sprintf_s (defaultName, sizeof(defaultName), "node%d", i);
        nodeNameIndexList[i] = nodeNameList.add (defaultName);
    }

    assert (i < u16MAX);
    return static_cast<u16>(i);
}

//********************************
void Model::priv_addAsLastChild   (u16 indexOf_existingNode, u16 indexOf_newNode)
{
    assert (flag.isBitSet(FLAG__BUILD_MODE));
    assert (indexOf_existingNode < nodeList.getNElem());
    assert (indexOf_newNode < nodeList.getNElem());

    //deve essere un nodo appena creato senza parentele
    assert (nodeList(indexOf_newNode).indexOf_firstChild == u16MAX);
    assert (nodeList(indexOf_newNode).indexOf_firstSibling == u16MAX);

    if (u16MAX == nodeList(indexOf_existingNode).indexOf_firstChild)
        nodeList[indexOf_existingNode].indexOf_firstChild = indexOf_newNode;
    else
        priv_addAsLastSibling (nodeList[indexOf_existingNode].indexOf_firstChild, indexOf_newNode);
}

//********************************
void Model::priv_addAsLastSibling (u16 indexOf_existingNode, u16 indexOf_newNode)
{
    assert (flag.isBitSet(FLAG__BUILD_MODE));
    assert (indexOf_existingNode < nodeList.getNElem());
    assert (indexOf_newNode < nodeList.getNElem());

    //deve essere un nodo appena creato senza parentele
    assert (nodeList(indexOf_newNode).indexOf_firstChild == u16MAX);
    assert (nodeList(indexOf_newNode).indexOf_firstSibling == u16MAX);


    u16 index = nodeList(indexOf_existingNode).indexOf_firstSibling;
    while (u16MAX != index)
    {
        index = nodeList(index).indexOf_firstSibling;
    }
    nodeList[index].indexOf_firstSibling = indexOf_newNode;
}

//********************************
u16 Model::priv_addMesh (u16 indexOf_shape, u16 indexOf_material)
{
    const u32 n = meshList.getNElem();
    meshList[n].indexOf_shape = indexOf_shape;
    meshList[n].indexOf_material = indexOf_material;
    return static_cast<u16>(n);
}

//********************************
u16 Model::addChild (u16 indexOf_fatherNode, const gos::mat4x4f &localMatrix, const char *name)
{
    assert (flag.isBitSet(FLAG__BUILD_MODE));
    if (indexOf_fatherNode >= nodeList.getNElem())
    {
        DBGBREAK;
        return u16MAX;
    }

    const u16 indexOf_newNode = priv_newNode (localMatrix, name);
    priv_addAsLastChild (indexOf_fatherNode, indexOf_newNode);
    return indexOf_newNode;
}

//********************************
u16 Model::addSibling (u16 indexOf_siblingNode, const gos::mat4x4f &localMatrix, const char *name)
{
    assert (flag.isBitSet(FLAG__BUILD_MODE));
    if (indexOf_siblingNode >= nodeList.getNElem())
    {
        DBGBREAK;
        return u16MAX;
    }

    const u16 indexOf_newNode = priv_newNode (localMatrix, name);
    priv_addAsLastSibling (indexOf_siblingNode, indexOf_newNode);
    return indexOf_newNode;
}

//********************************
void Model::addMeshToNode (u16 indexOf_node, u16 indexOf_shape, u16 indexOf_material)
{
    assert (flag.isBitSet(FLAG__BUILD_MODE));
    assert (indexOf_node < nodeList.getNElem());

    sNodo *nodo = &nodeList[indexOf_node];
    if (0 == nodo->numMesh)
    {
        nodo->numMesh = 1;
        nodo->indexOf_firstMesh = priv_addMesh (indexOf_shape, indexOf_material);
        return;
    }

    //ogni nodo deve avere un elenco di "mesh" contiguo
    if (meshList.getNElem() == nodo->indexOf_firstMesh + nodo->numMesh)
    {
        //caso fortunato, posso mettere la nuova mesh in fondo all'array dell mesh
        //mantenendo la continuita'
        nodo->numMesh++;
        priv_addMesh (indexOf_shape, indexOf_material);
        return;
    }

    //il nodo si riferisce ad un gruppo di mesh che e' "in mezzo" all'array di mesh.
    //Per mantenere il gruppo "contiguo", lo devo sostanzialmente ricreare a partire
    //dal fondo dell'array di mesh.
    //In questa fase non sto a fare ottimizzazioni, l'array delle mesh viene poi ottimizzato durante
    //la end()
    flag.set (FLAG__NEED_MESH_OPTIMIZATION);

    u16 indexOld = nodo->indexOf_firstMesh;
    u16 n = meshList.getNElem();
    nodo->indexOf_firstMesh = n;
    for (u16 i=0; i<nodo->numMesh; i++)
        meshList[n++] = meshList[indexOld++];

    
     priv_addMesh (indexOf_shape, indexOf_material);
     nodo->numMesh++;

     assert (nodo->indexOf_firstMesh + nodo->numMesh == meshList.getNElem());
}

//********************************
void Model::priv_optimizeMeshArray (sNodo *nodo, FastArray<sMesh> &tempMeshArray)
{
    if (0 == nodo->numMesh)
        return;

    u32 meshIndex = nodo->indexOf_firstMesh;
    u32 n = tempMeshArray.getNElem();
    nodo->indexOf_firstMesh = n;
    for (u32 i=0; i<nodo->numMesh; i++)
        tempMeshArray[n++] = meshList[meshIndex++];
}

//********************************
void Model::priv_recursiveMeshArrayOptimization (u16 nodeIndex, FastArray<sMesh> &tempMeshArray)
{
    sNodo *nodo = &nodeList[nodeIndex];

    //ottimizzo le mesh di questo nodo
    priv_optimizeMeshArray (nodo, tempMeshArray);

    //ricorsione sui miei figli
    u16 index = nodo->indexOf_firstChild;
    while (u16MAX != index)
    {
        priv_recursiveMeshArrayOptimization (index, tempMeshArray);
        index = nodeList(index).indexOf_firstSibling;
    }
}

//********************************
void Model::end ()
{
    assert (flag.isBitSet(FLAG__BUILD_MODE));
    flag.clear(FLAG__BUILD_MODE);

    if (flag.isBitSet(FLAG__NEED_MESH_OPTIMIZATION))
    {
        flag.clear(FLAG__NEED_MESH_OPTIMIZATION);
        
        //durante l'aggiunta delle mesh ai vari nodi, si sono creati dei "buchi" all'interno di meshList.
        //Ottimizzo l'array usando un array temporaneo
        FastArray<sMesh> temp(gos::getScrapAllocator(), meshList.getNElem());
        priv_recursiveMeshArrayOptimization (0, temp);

        //ora copio temp in meshList
        meshList.reset();
        meshList.copyFrom (temp);
        temp.unsetup();
    }

}