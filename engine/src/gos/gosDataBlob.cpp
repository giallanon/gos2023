#include "gosDataBlob.h"
#include "gos.h"

using namespace gos;
using namespace gos::datablob;

//******************************** 
void sElemHeader::decodeFromBuffer (const u8 *buffer)
{
    BufferR reader;
    reader.setup (buffer, 256, eEndianess::big);

    this->elemType = static_cast<eDataBlobElemType> (reader.readU8());
    this->nameLen = reader.readU8 ();
    this->next = reader.readU16 ();
    this->absOffset = reader.readU16 ();
    this->paddedSize = reader.readU16 ();

    this->elemName = NULL;
    if (this->nameLen)
    {
        this->elemName = reinterpret_cast<const char*>(reader.getPointer (reader.tell()));
        reader.advanceCursor (nameLen);
    }

    this->sizeof_thisHeader = reader.tell();
}