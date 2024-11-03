#ifndef _gosModel_h_
#define _gosModel_h_
#include "../gos/gos.h"
#include "../gosShape/gosShape.h"
#include "../gosGeom/gosGeomPos3.h"
#include "../gos/string/gosStringList.h"
#include "../gos/gosBit.h"


namespace gos
{
    /**
     * @brief Model
     * E' un elenco di [shape] e relativi [material_index] organizzati gerarchicamente
     * in un albero (skeleton)
     */
    class Model
    {
    public:
        struct sMesh
        {
            u16 indexOf_shape;
            u16 indexOf_material;
        };


    public:
                Model ();
                ~Model ()                                                                           { freeAll(); }


        void    freeAll();

        /**
         * @brief indica che posso iniziare a "addare" cose al modello
         */
        void    begin (gos::Allocator *allocator, u32 startingNumNodes, u32 startingNumOfMesh);

        /**
         * @brief fine della fase di creazione del modello
         */
        void    end();
        
        /**
         * @brief aggiunge un nodo come ultimo figlio del nodo [indexOf_fatherNode]
         * [name] e' opzionale e defaulta a nodeXX
         * Ritorna l'index del nodo creato
         */
        u16     addChild (u16 indexOf_fatherNode, const gos::mat4x4f &localMatrix, const char *name = NULL);

        /**
         * @brief aggiunge un nodo come ultimo fratello del nodo [indexOf_siblingNode].
         * [name] e' opzionale e defaulta a nodeXX
         * * Ritorna l'index del nodo creato
         */
        u16     addSibling (u16 indexOf_siblingNode, const gos::mat4x4f &localMatrix, const char *name = NULL);


        /**
         * @brief aggiunge la mesh [indexOf_shape, indexOf_material] al nodo [indexOf_node].
         * Un nodo puo' avere piu' di una mesh.
         * 
         * NB: le shape sono fisicamente mantenute da qualche altro oggetto, qui dentro
         * manteniamo solo l'index della shape
         */
        void     addMeshToNode (u16 indexOf_node, u16 indexOf_shape, u16 indexOf_material);


    private:
        static constexpr u8 FLAG__BUILD_MODE                = 0;
        static constexpr u8 FLAG__NEED_MESH_OPTIMIZATION    = 1;

    private:
        struct sNodo
        {
            gos::mat4x4f    localMatrix;
            u16             indexOf_firstChild;
            u16             indexOf_firstSibling;
            u16             indexOf_firstMesh;      //un nodo contiene un elenco di [numMesh] mesh a partire
            u16             numMesh;                //da meshList[indexOf_firstMesh]
        };


    private:
        u16     priv_newNode (const gos::mat4x4f &localMatrix, const char *name = NULL);
        void    priv_addAsLastChild   (u16 indexOf_existingNode, u16 indexOf_newNode);
        void    priv_addAsLastSibling (u16 indexOf_existingNode, u16 indexOf_newNode);
        void    priv_addShape (u16 nodeIndex, u16 shapeIndex);
        u16     priv_addMesh (u16 indexOf_shape, u16 indexOf_material);
        void    priv_recursiveMeshArrayOptimization (u16 nodeIndex, FastArray<sMesh> &tempMeshArray);
        void    priv_optimizeMeshArray (sNodo *nodo, FastArray<sMesh> &tempMeshArray);

    private:
        gos::Allocator          *localAllocator;
        FastArray<sNodo>        nodeList;
        FastArray<u32>          nodeNameIndexList;  //alla posizione [i] c'è l'offset stringa contenente il nome del nodo[i]
        FastArray<sMesh>        meshList;
        gos::StringList         nodeNameList;       //tabella con le stringhe contenenti i nomi dei nodi
        gos::Flag8              flag;
    };
        

} //namespace gos

#endif //_gosModel_h_

