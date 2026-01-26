#include "gosModel.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::model;


//*******************************************
Builder::Builder ()
{
    listof_shape.setup (gos::getScrapAllocator(), 64);
    listof_mesh.setup (gos::getScrapAllocator(), 64);
}

//*******************************************
Builder::~Builder()
{
    listof_shape.unsetup();
    listof_mesh.unsetup();
}

//*******************************************
Builder& Builder::begin (Engine *engIN)
{
    eng = engIN;
    bAnyErr = false;
    handle_skeleton.setInvalid();
    listof_shape.reset();
    listof_mesh.reset();
}

//*******************************************
Builder& Builder::skeleton_set (ENGSkeleton handle)
{
    if (bAnyErr)
        return *this;
    handle_skeleton = handle;
    
    const engine::ResSkeleton *res_skeleton;
    if (false == eng->get (handle_skeleton, &res_skeleton))
        bAnyErr = true;
    return *this;
}

//*******************************************
Builder& Builder::mesh_add (ENGGPUShape handle_shape, u32 material_index, const char *boneName)
{
    if (bAnyErr)
        return *this;

    Mesh mesh;


    //shape index
    mesh.shape_index = listof_shape.simpleSearch (handle_shape);
    if (u32MAX == mesh.shape_index)
    {
        mesh.shape_index = listof_shape.getNElem();
        listof_shape.append (handle_shape);
    }

    //material index
    mesh.material_index = material_index;

    //bone index
    mesh.bone_index = 0;
    if (handle_skeleton.isInvalid())
    {
        if (!string::utf8::areEqual(boneName, "root", false))
        {
            //errore: senza uno sk definito, l'unica bone valida e' root
            bAnyErr = true;
        }
    }
    else
    {
        const engine::ResSkeleton *res_skeleton;
        eng->get (handle_skeleton, &res_skeleton);
        
        skeleton::Reader reader(&res_skeleton->data.skeleton);
        mesh.bone_index = reader.bone_get_index_by_name (boneName);
        if (u32MAX == mesh.bone_index)
            bAnyErr = true;
    }

    if (!bAnyErr)
        listof_mesh.append (mesh);

    return *this;
}

//*******************************************
bool Builder::end (gos::Allocator *allocatorIN, Model *out)
{
    if (bAnyErr)
    {
        DBGBREAK;
        return false;
    }
    const u16 num_shape = (u16)listof_shape.getNElem();
    const u16 num_meshes = (u16)listof_mesh.getNElem();
    const u16 num_material = 0;


    const u32 total_size_of_blob = 
          sizeof(u32)   //magic
        + sizeof(u32)   //total_size_of_blob

        + sizeof(u16)   //num_shape
        + sizeof(u16)   //num_material
        + sizeof(u16)   //num_meshes
        + sizeof(u16)   //abs-offset-to MESH 1

        + sizeof(u32)   //ENGSkeleton
        + sizeof(u32) * num_shape   //1 ENGGPUShape per ogni shape
        + sizeof(u16) * 4 * num_meshes;

    out->allocator = allocatorIN;
    out->blob = GOSALLOCT(u8*, out->allocator, total_size_of_blob);
    memset (out->blob, 0, total_size_of_blob);


    u32 ct = 0;
    ct += gos::utils::bufferWriteU32 (&out->blob[ct], GOS_MAGIC__ENGINE_MODEL);
    ct += gos::utils::bufferWriteU32 (&out->blob[ct], total_size_of_blob);

    ct += gos::utils::bufferWriteU16 (&out->blob[ct], num_shape);
    ct += gos::utils::bufferWriteU16 (&out->blob[ct], num_material);
    ct += gos::utils::bufferWriteU16 (&out->blob[ct], num_meshes);
    ct += gos::utils::bufferWriteU16 (&out->blob[ct], 0); ///abs-offset-to MESH 1

    //skeleton
    ct += gos::utils::bufferWriteU32 (&out->blob[ct], handle_skeleton.viewAsU32());

    //shapes
    for (u32 i = 0; i < num_shape; i++)
    {
        ct += gos::utils::bufferWriteU32 (&out->blob[ct], listof_shape(i).viewAsU32());
    }

    //meshes
    const u16 ABS_OFFSET_of_MESH1 = (u16)ct;
    gos::utils::bufferWriteU16 (&out->blob[14], ABS_OFFSET_of_MESH1);
    for (u32 i = 0; i < num_meshes; i++)
    {
        ct += gos::utils::bufferWriteU16 (&out->blob[ct], (u16)listof_mesh(i).shape_index);
        ct += gos::utils::bufferWriteU16 (&out->blob[ct], (u16)listof_mesh(i).bone_index);
        ct += gos::utils::bufferWriteU16 (&out->blob[ct], (u16)listof_mesh(i).material_index);
        ct += gos::utils::bufferWriteU16 (&out->blob[ct], 0);
    }

    assert (ct == total_size_of_blob);

    return true;
}