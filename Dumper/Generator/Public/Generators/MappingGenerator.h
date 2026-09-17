#pragma once

#include <fstream>

#include "Unreal/ObjectArray.h"
#include "Wrappers/MemberWrappers.h"
#include "Wrappers/EnumWrapper.h"


/*
* USMAP-Header:
* 
* uint16 magic;
* uint8 version;                                           // Latest = PropertyFlags (6)
* if (version >= PackageVersioning)
*     int32 bHasVersioning;
*     if (bHasVersioning) 
*         if (version >= EngineVersioning)
*             FEngineVersion EngineVersion;
*         [FileVersionUE4/UE5 + CustomVersions + NetCL]
* uint8 CompressionMethod;
* uint32 CompressedSize;
* uint32 DecompressedSize;
* 
* 
* USMAP-Data:
* 
* uint32 NameCount;
* for (int i = 0; i < NameCount; i++)
*     [uint8|uint16] NameLength;
*     uint8 StringData[NameLength];
* 
* uint32 EnumCount;
* for (int i = 0; i < EnumCount; i++)
*     int32 EnumNameIdx;
*     [uint8|uint16] NumNamesInEnum;              // u8 if version < LargeEnums, else u16
*     for (int j = 0; j < NumNamesInEnum; j++)
*         if (version >= ExplicitEnumValues)
*             uint64 EnumMemberValue;
*         int32 EnumMemberNameIdx;
* 
* if (version >= PropertyFlags)
*     uint32 FlagDictCount;
*     uint64 FlagDict[FlagDictCount];                      // unique EPropertyFlags, first-seen order
* 
* uint32 StructCount;
* for (int i = 0; i < StructCount; i++)
*     int32 StructNameIdx;                                 // <-- START ParseStruct
*     int32 SuperTypeNameIdx;
*     uint16 PropertyCount;
*     uint16 SerializablePropertyCount;
*     for (int j = 0; j < SerializablePropertyCount; j++)
*         uint16 Index;                                    // <-- START ParsePropertyInfo
*         uint8 ArrayDim;
*         int32 PropertyNameIdx;
*         uint8 MappingsTypeEnum;                         // <-- START ParsePropertyType      [[ByteProperty needs to be written as EnumProperty if it has an underlaying Enum]]
*         if (MappingsTypeEnum == EnumProperty || (MappingsTypeEnum == ByteProperty && UnderlayingEnum != null))
*             CALL ParsePropertyType;
*             int32 EnumName;
*         else if (MappingsTypeEnum == StructProperty)
*             int32 StructNameIdx;
*         else if (MappingsTypeEnum == (SetProperty | ArrayProperty | OptionalProperty))
*             CALL ParsePropertyType;
*         else if (MappingsTypeEnum == MapProperty)
*             CALL ParsePropertyType;
*             CALL ParsePropertyType;                       // <-- END ParsePropertyType
*         if (version >= PropertyFlags)
*             [uint8|uint16] FlagIndex;                    // u8 if FlagDictCount <= 255, else u16
*/

class MappingGenerator
{
private:
    using StreamType = std::ofstream;

private:
    enum class EUsmapVersion : uint8
    {
        /* Initial format. */
        Initial,

        /* Adds package versioning to aid with compatibility */
        PackageVersioning,

        /* Adds support for 16-bit wide name-lengths (ushort/uint16) */
        LongFName,

        /* Adds support for enums with more than 255 values */
        LargeEnums,

        /* Adds support for explicit enum values */
        ExplicitEnumValues,

        /* Adds support for engine versioning information */
        EngineVersioning,

        /* Adds a file-level EPropertyFlags dictionary and per-property FlagIndex */
        PropertyFlags,

        LatestPlusOne,
        Latest = LatestPlusOne - 1,
    };

private:
    static constexpr uint16 UsmapFileMagic = 0x30C4;
    static constexpr EUsmapVersion WrittenVersion = EUsmapVersion::Latest;

private:
    static inline uint64 NameCounter = 0x0;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "Mappings";
    static inline std::string SubfolderName = "";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    template<typename InStreamType, typename T>
    static void WriteToStream(InStreamType& InStream, T Value)
    {
        InStream.write(reinterpret_cast<const char*>(&Value), sizeof(T));
    }

    template<typename InStreamType>
    static void WriteToStream(InStreamType& InStream, const std::stringstream& Data)
    {
        InStream << Data.rdbuf();
    }

private:
    /* Utility Functions */
    static EMappingsTypeFlags GetMappingType(UEProperty Property);
    static int32 AddNameToData(std::stringstream& NameTable, const std::string& Name);

private:
    static bool ShouldExcludeEditorOnlyProperties();
    static bool ShouldWriteMappingProperty(const PropertyWrapper& Property);

    static void CollectPropertyFlags(const StructWrapper& Struct);
    static void CollectAllPropertyFlags();
    static void WriteFlagDictionary(std::stringstream& OutData);

    static void GeneratePropertyType(UEProperty Property, std::stringstream& Data, std::stringstream& NameTable);
    static void GeneratePropertyInfo(const PropertyWrapper& Property, std::stringstream& Data, std::stringstream& NameTable, int32& Index);

    static void GenerateStruct(const StructWrapper& Struct, std::stringstream& Data, std::stringstream& NameTable);
    static void GenerateEnum(const EnumWrapper& Enum, std::stringstream& Data, std::stringstream& NameTable);

    static std::stringstream GenerateFileData();
    static void GenerateFileHeader(StreamType& InUsmap, const std::stringstream& Data);

public:
    static void Generate();

    /* Always empty, there are no predefined members for mappings */
    static void InitPredefinedMembers() { }
    static void InitPredefinedFunctions() { }
};
