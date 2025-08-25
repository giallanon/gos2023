#ifndef _gosRes_h_
#define _gosRes_h_
#include "gosResEnumAndDefine.h"

namespace gos
{
    namespace res
    {
        bool    create_folder_structure (const char *baseFolder);

        u32     calc_resUID (eResType resType, const void *buffer, u32 sizeof_buffer);

        void    manufacture_compiled_fullFilePathAndName (const char *baseFolder, u32 resUID, char *out, u32 sizeof_out);

        bool    shader_compile (const char *shaderSRCFile, const char *shaderStage, const char *spaceSeparateDefineList, const char *shaderDSTFile, bool bIncludeDebugInfo);

        bool    resUID_exists (DBHandle &db, u32 resUID);
        bool    find_resUID (DBHandle &db, const char *runtimeResName, u32 *out_resID);

        bool    need_rebuild (sBuilderSession &session, u32 resUID, u64 lastTimeIniSectionWasUpdate);
        void    onResourceBuilt (sBuilderSession &session, u32 resUID, eResType resType, bool bJustRebuilt, const char *runtimeName);


    } //namespace res
} //namespace gos

#endif //_gosRes_h_