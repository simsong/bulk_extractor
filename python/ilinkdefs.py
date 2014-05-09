ShowWindowStyle = {1: "SW_SHOWNORMAL",
                   3: "SW_SHOWMAXIMIZED",
                   7: "SW_SHOWMINNOACTIVE"
                   };


HasLinkTargetIDList = 0x0000001
HasLinkInfo = 0x00000002
HasName = 0x0000004
HasRelativePath = 0x00000008
HasWorkingDir = 0x0000010
HasArguments = 0x00000020
HasIconLocation = 0x0000040
IsUnicode = 0x0000080
LinkFlags = {HasLinkTargetIDList: "HasLinkTargetIDList",
             HasLinkInfo: "HasLinkInfo",
             HasName: "HasName",
             HasRelativePath: "HasRelativePath",
             HasWorkingDir: "HasWorkingDir",
             HasArguments: "HasArguments",
             HasIconLocation: "HasIconLocation",
             IsUnicode: "IsUnicode",
             0x0000100: "ForceNoLinkInfo",
             0x0000200: "HasExpString",
             0x0000400: "RunInSeparateProcess",
             0x0000800: "Unused1",
             0x0001000: "HasDarwinId",
             0x0002000: "RunAsUser",
             0x0004000: "HasExpIcon",
             0x0008000: "NoPidlAlias",
             0x0010000: "Unused2",
             0x0020000: "RunWithShimLayer",
             0x0040000: "ForceNoLinkTrack",
             0x0080000: "EnableTargetMetadata",
             0x0100000: "DisableLinkPathTracking",
             0x0200000: "DisableKnownFolderTracking",
             0x0400000: "DisableKnownFolderAlias",
             0x0800000: "AllowLinkToLink",
             0x1000000: "UnaliasOnSave",
             0x2000000: "PreferEnvironmentPath",
             0x4000000: "KeepLocalIDListForUNCTarget",
            }


FileAttributeFlags = {0x0001: "FILE_ATTRIBUTE_READONLY",
                      0x0002: "FILE_ATTRIBUTE_HIDDEN",
                      0x0004: "FILE_ATTRIBUTE_SYSTEM",
                      0x0008: "Reserved1", #volume label?
                      0x0010: "FILE_ATTRIBUTE_DIRECTORY",
                      0x0020: "FILE_ATTRIBUTE_ARCHIVE",
                      0x0040: "Reserved2", #encrypted?
                      0x0080: "FILE_ATTRIBUTE_NORMAL",
                      0x0100: "FILE_ATTRIBUTE_TEMPORARY",
                      0x0200: "FILE_ATTRIBUTE_SPARSE_FILE",
                      0x0400: "FILE_ATTRIBUTE_REPARSE_POINT",
                      0x0800: "FILE_ATTRIBUTE_COMPRESSED",
                      0x1000: "ILE_ATTRIBUTE_OFFLINE",
                      0x2000: "FILE_ATTRIBUTE_NOT_CONTENT_INDEXED",
                      0x4000: "FILE_ATTRIBUTE_ENCRYPTED"
                      }


VolumeTypes = {0x000: "Unknown",
               0x001: "No Root Directory",
               0x002: "Removable",
               0x003: "Fixed",
               0x004: "Remote",
               0x005: "CD-ROM",
               0x006: "RAM Drive"
               }


VirtualKeyCodes = {0x30: "0",
                   0x31: "1",
                   0x32: "2",
                   0x33: "3",
                   0x34: "4",
                   0x35: "5",
                   0x36: "6",
                   0x37: "7",
                   0x38: "8",
                   0x39: "9",
                   0x41: "A",
                   0x42: "B",
                   0x43: "C",
                   0x44: "D",
                   0x45: "E",
                   0x46: "F",
                   0x47: "G",
                   0x48: "H",
                   0x49: "I",
                   0x4A: "J",
                   0x4B: "K",
                   0x4C: "L",
                   0x4D: "M",
                   0x4E: "N",
                   0x4F: "O",
                   0x50: "P",
                   0x51: "Q",
                   0x52: "R",
                   0x53: "S",
                   0x54: "T",
                   0x55: "U",
                   0x56: "V",
                   0x57: "W",
                   0x58: "X",
                   0x59: "Y",
                   0x5A: "Z",
                   0x70: "VK_F1",
                   0x71: "VK_F2",
                   0x72: "VK_F3",
                   0x73: "VK_F4",
                   0x74: "VK_F5",
                   0x75: "VK_F6",
                   0x76: "VK_F7",
                   0x77: "VK_F8",
                   0x78: "VK_F9",
                   0x79: "VK_F10",
                   0x7A: "VK_F11",
                   0x7B: "VK_F12",
                   0x7C: "VK_F13",
                   0x7D: "VK_F14",
                   0x7E: "VK_F15",
                   0x7F: "VK_F16",
                   0x80: "VK_F17",
                   0x81: "VK_F18",
                   0x82: "VK_F19",
                   0x83: "VK_F20",
                   0x84: "VK_F21",
                   0x85: "VK_F22",
                   0x86: "VK_F23",
                   0x87: "VK_F24",
                   0x90: "VK_NUMLOCK",
                   0x91: "VK_SCROLL"
                   }


ModifierKeyCodes = {0x01: "HOTKEYF_SHIFT",
                    0x02: "HOTKEYF_CONTROL",
                    0x04: "HOTKEYF_ALT"
                    }


VolumeIDAndLocalBasePath = 0x01
CommonNetworkRelativeLinkAndPathSuffix = 0x02
LinkInfoFlags = {VolumeIDAndLocalBasePath: "VolumeIDAndLocalBasePath",
                 CommonNetworkRelativeLinkAndPathSuffix: "CommonNetworkRelativeLinkAndPathSuffix"
                 }


DriveTypeFlags = {0x0000: "DRIVE_UNKNOWN",
                  0x0001: "DRIVE_NO_ROOT_DIR",
                  0x0002: "DRIVE_REMOVABLE",
                  0x0003: "DRIVE_FIXED",
                  0x0004: "DRIVE_REMOTE",
                  0x0005: "DRIVE_CDROM",
                  0x0006: "DRIVE_RAMDISK"
                  }


ValidDevice = 0x0001
ValidNetType = 0x0002
CommonNetworkRelativeLinkFlags = {ValidDevice: "ValidDevice",
                                  ValidNetType: "ValidNetType"
                                  }


NetworkProviderType = {0x001A0000: "WNNC_NET_AVID",
                       0x001B0000: "WNNC_NET_DOCUSPACE",
                       0x001C0000: "WNNC_NET_MANGOSOFT",
                       0x001D0000: "WNNC_NET_SERNET",
                       0x001E0000: "WNNC_NET_RIVERFRONT1",
                       0x001F0000: "WNNC_NET_RIVERFRONT2",
                       0x00200000: "WNNC_NET_DECORB",
                       0x00210000: "WNNC_NET_PROTSTOR",
                       0x00220000: "WNNC_NET_FJ_REDIR",
                       0x00230000: "WNNC_NET_DISTINCT",
                       0x00240000: "WNNC_NET_TWINS",
                       0x00250000: "WNNC_NET_RDR2SAMPLE",
                       0x00260000: "WNNC_NET_CSC",
                       0x00270000: "WNNC_NET_3IN1",
                       0x00290000: "WNNC_NET_EXTENDNET",
                       0x002A0000: "WNNC_NET_STAC",
                       0x002B0000: "WNNC_NET_FOXBAT",
                       0x002C0000: "WNNC_NET_YAHOO",
                       0x002D0000: "WNNC_NET_EXIFS",
                       0x002E0000: "WNNC_NET_DAV",
                       0x002F0000: "WNNC_NET_KNOWARE",
                       0x00300000: "WNNC_NET_OBJECT_DIRE",
                       0x00310000: "WNNC_NET_MASFAX",
                       0x00320000: "WNNC_NET_HOB_NFS",
                       0x00330000: "WNNC_NET_SHIVA",
                       0x00340000: "WNNC_NET_IBMAL",
                       0x00350000: "WNNC_NET_LOCK",
                       0x00360000: "WNNC_NET_TERMSRV",
                       0x00370000: "WNNC_NET_SRT",
                       0x00380000: "WNNC_NET_QUINCY",
                       0x00390000: "WNNC_NET_OPENAFS",
                       0x003A0000: "WNNC_NET_AVID1",
                       0x003B0000: "WNNC_NET_DFS",
                       0x003C0000: "WNNC_NET_KWNP",
                       0x003D0000: "WNNC_NET_ZENWORKS",
                       0x003E0000: "WNNC_NET_DRIVEONWEB",
                       0x003F0000: "WNNC_NET_VMWARE",
                       0x00400000: "WNNC_NET_RSFX",
                       0x00410000: "WNNC_NET_MFILES",
                       0x00420000: "WNNC_NET_MS_NFS",
                       0x00430000: "WNNC_NET_GOOGLE"
                       }

ConsoleDataBlockSig = 0xA0000002
ConsoleFEDataBlockSig = 0xA0000004
DarwinDataBlockSig = 0xA0000006
EnvironmentVariableDataBlockSig = 0xA0000001
IconEnvironmentDataBlockSig = 0xA0000007
KnownFolderDataBlockSig = 0xA000000B
PropertyStoreDataBlockSig = 0xA0000009
ShimDataBlockSig = 0xA0000008
SpecialFolderDataBlockSig = 0xA0000005
TrackerDataBlockSig = 0xA0000003
VistaAndAboveIDListDataBlockSig = 0xA000000C
ExtraDataSignatures = {ConsoleDataBlockSig: "ConsoleDataBlock",
                       ConsoleFEDataBlockSig: "ConsoleFEDataBlock",
                       DarwinDataBlockSig: "DarwinDataBlock",
                       EnvironmentVariableDataBlockSig: "EnvironmentVariableDataBlock",
                       IconEnvironmentDataBlockSig: "IconEnvironmentDataBlock",
                       KnownFolderDataBlockSig: "KnownFolderDataBlock",
                       PropertyStoreDataBlockSig: "PropertyStoreDataBlock",
                       ShimDataBlockSig: "ShimDataBlock",
                       SpecialFolderDataBlockSig: "SpecialFolderDataBlock",
                       TrackerDataBlockSig: "TrackerDataBlock",
                       VistaAndAboveIDListDataBlockSig: "VistaAndAboveIDListDataBlock"}

FillAttributes = {0x0001: "FOREGROUND_BLUE",
                  0x0002: "FOREGROUND_GREEN",
                  0x0004: "FOREGROUND_RED",
                  0x0008: "FOREGROUND_INTENSITY",
                  0x0010: "BACKGROUND_BLUE",
                  0x0020: "BACKGROUND_GREEN",
                  0x0040: "BACKGROUND_RED",
                  0x0080: "BACKGROUND_INTENSITY"
                  }

FontFamily = {0x0000: "FF_DONTCARE",
              0x0010: "FF_ROMAN",
              0x0020: "FF_SWISS",
              0x0030: "FF_MODERN",
              0x0040: "FF_SCRIPT",
              0x0050: "FF_DECORATIVE"}

                       
