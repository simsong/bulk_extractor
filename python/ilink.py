#!/usr/bin/python

"""Windows LNK files parser/dumper"""

import os, sys, struct
import ilinkdefs
from time import strftime
from ilinkdefs import (ShowWindowStyle, LinkFlags, FileAttributeFlags,
    VirtualKeyCodes, ModifierKeyCodes, LinkInfoFlags, DriveTypeFlags, HasLinkTargetIDList, HasLinkInfo,
    HasName, HasRelativePath, HasWorkingDir, HasArguments, HasIconLocation, IsUnicode,
    VolumeIDAndLocalBasePath, CommonNetworkRelativeLinkAndPathSuffix, CommonNetworkRelativeLinkFlags,
    ValidDevice, ValidNetType, NetworkProviderType, ExtraDataSignatures, ConsoleDataBlockSig,
    ConsoleFEDataBlockSig, DarwinDataBlockSig, EnvironmentVariableDataBlockSig,
    IconEnvironmentDataBlockSig, KnownFolderDataBlockSig, PropertyStoreDataBlockSig,
    ShimDataBlockSig, SpecialFolderDataBlockSig, TrackerDataBlockSig,
    VistaAndAboveIDListDataBlockSig, FillAttributes, FontFamily)


def getFlagsString(flags, StringDict):
    fString = ""
    for key, value in StringDict.items():
        if flags & key == key:
            flags -= key
            fString += value + " | "
    if flags != 0:
        fString += "{0:08X}".format(flags)
    else:
        fString = fString[:-3] #delete trailing " | "
    return fString


def getValueString(value, StringDict):
    if value in StringDict:
        vString = StringDict[value]
    else:
        vString = "{0:08x}".format(value)
    return vString


def getHotKey(field):
 return getValueString(field & 0xFF, VirtualKeyCodes) + " + " + getFlagsString(field >> 8, ModifierKeyCodes)



def readASCIIZ(dataStr, sz = 1):
    string, c, idx = "", 0, 0
    while c != '\00' and idx < len(dataStr):
        if sz > 1:
            (c,) = struct.unpack("=H", dataStr[idx:idx + 2])
        else:
            (c,) = struct.unpack("=B", dataStr[idx:idx + 1])
        if c != 0:
            string += unichr(c);
        idx += sz
    return string


class LinkStructBase:
    def __init__(self, dataStr):
        self.data = dataStr;
        self.parseData();

    def parseData(self):
        pass

    def getString(self, pad):
        outStr = ""
        for item in self.Fields:
            field, fmt = item
            if hasattr(self, field):
                outStr += pad + field + " = "
                if isinstance(fmt, tuple):
                    fmtStr, val = fmt
                else:
                    fmtStr = fmt
                #print "field =", field, "fmtStr =", fmtStr
                if fmtStr.upper() == "VAL":
                    outStr += getValueString(getattr(self, field), val)
                elif fmtStr.upper() == "FLG":
                    outStr += getFlagsString(getattr(self, field), val)
                elif fmtStr.upper() == "OBJ":
                    outStr += "\n" + getattr(self, field).getString(pad + "  ")
                elif fmtStr.upper() == "FCT":
                    outStr += val(getattr(self, field))
                elif fmtStr.upper() == "LST":
                    outStr += "["
                    lst = getattr(self, field)
                    for lstItem in lst:
                        if isinstance(lstItem, LinkStructBase):
                            outStr += ""
                        else:
                            outStr += repr(lstItem) + ", "
                    outStr += "]"
                else:
                    outStr += repr(format(getattr(self, field), fmtStr))
                outStr += "\n"
        return outStr

    Fields = []


class LinkFileHeader(LinkStructBase):
    def parseData(self):
        (self.Signature, 
         clsid1, clsid2, clsid3, clsid4, 
         self.LinkFlags, 
         self.FileAttributes, 
         self.CreationTime, 
         self.AccessTime, 
         self.WriteTime, 
         self.FileSize, 
         self.IconIndex, 
         self.ShowCommand,
         self.HotKey,
         reserved1, reserved2, reserved3) = struct.unpack("=LLHHQLLQQQLLLHHLL", self.data[0:76])
        self.LinkCLSID = [clsid1, clsid2, clsid3, clsid4]
        self.Reserved = [reserved1, reserved2, reserved3]

    Fields = [("Signature", "08x"),
              ("LinkCLSID", "LST"),
              ("LinkLinkFlags", ("FLG", LinkFlags)),
              ("FileAttributes", ("FLG", FileAttributeFlags)),
              ("CreationTime", "016x"),
              ("AccessTime", "016x"),
              ("WriteTime", "016x"),
              ("FileSize", "08x"),
              ("IconIndex", "08x"),
              ("ShowCommand", ("VAL", ShowWindowStyle)),
              ("HotKey", ("FCT", getHotKey)),
              ("Reserved", "LST")]
        

class LinkFileIDList(LinkStructBase):
    def parseData(self):
        (itemIDSize,) = (self.IDListSize,) = struct.unpack("=H", self.data[0:2])
        idx = 2
        while itemIDSize != 0:
            (itemIDSize,) = struct.unpack("=H", self.data[idx:idx + 2])
            if itemIDSize == 0:
                break;
            self.IDList.append((itemIDSize, self.data[idx:idx + itemIDSize - 2]))
            idx += itemIDSize

    IDList = []
    Fields = [("IDListSize", "08d"),
              ("IDList", "LST")]


class LinkInfo(LinkStructBase):
    def parseData(self):
        (self.Size,
         self.HeaderSize,
         self.Flags,
         self.VolumeIDOffset,
         self.LocalBasePathOffset,
         self.CommonNetworkRelativeLinkOffset,
         self.CommonPathSuffixOffset) = struct.unpack("=LLLLLLL", self.data[0:28])
        idx = 28
        if self.HeaderSize >= 0x24:
            (self.LocalBasePathOffsetUnicode,) = struct.unpack("=L", self.data[idx:idx + 4])
            self.LocalBasePathUnicode = readASCIIZ(self.data[self.LocalBasePathOffsetUnicode:], 2)
            (self.CommonPathSuffixOffsetUnicode,) = struct.unpack("=L", self.data[idx + 4:idx + 8])
            self.CommonPathSuffixUnicode = readASCIIZ(self.data[self.CommonPathSuffixUnicode])
            idx += 8
        if self.Flags & CommonNetworkRelativeLinkAndPathSuffix != 0:
            (size,) = struct.unpack("=L", self.data[self.VolumeIDOffset:self.VolumeIDOffset + 4])
            self.VolumeID = VolumeID(self.data[self.VolumeIDOffset:self.VolumeIDOffset + size])
            (size,) = struct.unpack("=L", self.data[self.CommonNetworkRelativeLinkOffset:self.CommonNetworkRelativeLinkOffset + 4])
            self.CommonNetworkRelativeLink = CommonNetworkRelativeLink(self.data[self.CommonNetworkRelativeLinkOffset:self.CommonNetworkRelativeLinkOffset + size])
        if self.Flags & VolumeIDAndLocalBasePath != 0:
            self.LocalBasePath = readASCIIZ(self.data[self.LocalBasePathOffset:], 1)
        self.CommonPathSuffix = readASCIIZ(self.data[self.CommonPathSuffixOffset:], 1)

    Fields = [("Size", "08d"),
              ("HeaderSize", "08d"),
              ("Flags", ("FLG", LinkInfoFlags)),
              ("VolumeIDOffset", "08d"),
              ("LocalBasePathOffset", "08d"),
              ("CommonNetworkRelativeLinkOffset", "08d"),
              ("CommonPathSuffixOffset", "08d"),
              ("LocalBasePathOffsetUnicode", "08d"),
              ("LocalBasePathUnicode", "08d"),
              ("CommonPathSuffixOffsetUnicode", "08d"),
              ("CommonPathSuffixUnicode", "s"),
              ("VolumeID", "OBJ"),
              ("CommonNetworkRelativeLink", "OBJ"),
              ("LocalBasePath", "s"),
              ("CommonPathSuffix", "s")]


class VolumeID(LinkStructBase):
    def parseData(self):
        (self.Size,
         self.DriveType,
         self.DriveSerialNumber,
         self.VolumeLabelOffset
         ) = struct.unpack("=LLLL", self.data[0:16])
        if self.VolumeLabelOffset == 0x14:
            self.VolumeLabelOffsetUnicode = struct.unpack("=L", self.data[16:20])
            self.Data = readASCIIZ(self.data[self.VolumeLabelOffsetUnicode:], 2)
        else:
            self.Data = readASCIIZ(self.data[self.VolumeLabelOffset:], 1)

    Fields = [("Size", "08d"),
              ("DriveType", ("VAL", DriveTypeFlags)),
              ("DriveSerialNumber", "08x"),
              ("VolumeLabelOffset", "08d"),
              ("VolumeLabelOffsetUnicode", "s")]



class CommonNetworkRelativeLink(LinkStructBase):
    def parseData(self):
        print "CommonNet"
        idx = 20
        (self.Size,
         self.Flags,
         self.NetNameOffset,
         self.DeviceNameOffset,
         self.NetworkProviderType) = struct.unpack("=LLLLL", self.data[0:20])
        if self.NetNameOffset > 0x14:
            (self.NetNameOffsetUnicode,) = struct.unpack("=L", self.data[idx:idx + 4])
            self.NetNameUnicode = readASCIIZ(self.data[self.NetNameOffsetUnicode:], 2)
            idx += 4
        else:
            self.NetName = readASCIIZ(self.data[self.NetNameOffset:], 1)
        if self.DeviceNameOffset > 0x14:
            (self.DeviceNameOffsetUnicode,) = struct.unpack("=L", self.data[idx:idx + 4])
            self.DeviceNameUnicode = readASCIIZ(self.data[self.DeviceNameUnicode:], 2)
            idx += 4
        else:
            self.DeviceName = readASCIIZ(self.data[self.DeviceNameOffset:], 1)

    Fields = [("Size", "08d"),
              ("Flags", ("FLG", CommonNetworkRelativeLinkFlags)),
              ("DeviceNameOffsetUnicode", "08d"),
              ("DeviceNameUnicode", "s"),
              ("DeviceNameOffset", "08d"),
              ("DeviceName", "s"),
              ("NetworkProviderType", ("FLG", NetworkProviderType)),
              ("NetNameOffsetUnicode", "08d"),
              ("NetNameUnicode", "s"),
              ("NetNameOffset", "08d"),
              ("NetName", "s")]


class StringData(LinkStructBase):
    def __init__(self, dataStr, name, flags):
        self.Flags = flags
        self.Name = name
        LinkStructBase.__init__(self, dataStr)

    def parseData(self):
        strFactor = 1
        if self.Flags & IsUnicode:
            strFactor = 2
        (self.CountCharacters,) = struct.unpack("=H", self.data[0:2])
        self.String = readASCIIZ(self.data[2:2 + self.CountCharacters * strFactor], strFactor)

    Fields = [("Name", "s")]


class ExtraData(LinkStructBase):
    def parseData(self):
        (size,), idx, bType, bName = struct.unpack("=L", self.data[0:4]), 0, 0, ""
        types = {ConsoleFEDataBlockSig: ConsoleFEDataBlock,
                 ConsoleDataBlockSig: ConsoleDataBlock,
                 DarwinDataBlockSig: DarwinDataBlock,
                 EnvironmentVariableDataBlockSig: EnvironmentVariableDataBlock,
                 IconEnvironmentDataBlockSig: IconEnvironmentDataBlock,
                 KnownFolderDataBlockSig: KnownFolderDataBlock,
                 PropertyStoreDataBlockSig: PropertyStoreDataBlock,
                 ShimDataBlockSig: ShimDataBlock,
                 SpecialFolderDataBlockSig: SpecialFolderDataBlock,
                 TrackerDataBlockSig: TrackerDataBlock,
                 VistaAndAboveIDListDataBlockSig: VistaAndAboveIDListDataBlock}
        while size > 4:
            (bType,) = struct.unpack("=L", self.data[idx + 4:idx + 8])
            self.Blocks.append(types.get(bType, ExtraDataBlock)(self.data[idx: idx + size]))
            idx += size
            if idx >= len(self.data):
                size = 0
            else:
                (size,) = struct.unpack("=L", self.data[idx:idx + 4])

    Blocks = []
    Fields = [("Blocks", "LST")]


class ExtraDataBlock(LinkStructBase):
    def parseData(self):
        (self.BlockSize,
         self.BlockSignature) = struct.unpack("=LL", self.data[0:8])

    Fields = [("BlockSize", "08d"),
              ("BlockSignature", "08d")]


class ConsoleDataBlock(ExtraDataBlock):
    def parseData(self):
        ExtraDataBlock.parseData(self)
        (self.FillAttributes,
         self.PopupFillAttributes,
         self.ScreenBufferSizeX, self.ScreenBufferSizeY,
         self.WindowSizeX, self.WindowSizeY,
         self.WindowOriginX, self.WindowOriginY,
         self.Unused1, self.Unused2,
         self.FontSize, self.FontFamily, self.FontWeight) = struct.unpack("=HHHHHHHHLLLLL", self.data[8:44])
        self.FaceName = readASCIIZ(self.data[44:108], 2)
        (self.CursorSize,
         self.FullScreen,
         self.QuickEdit,
         self.InsertMode,
         self.AutoPosition,
         self.HistoryBufferSize,
         self.NumberOfHistoryBuffers,
         self.HistoryNoDup,
         self.ColorTable) = struct.unpack("=LLLLLLLLL", self.data[108:144])


class ConsoleFEDataBlock(ExtraDataBlock):
    def parseData(self):
        ExtraDataBlock.parseData(self)
        (self.CodePage,) = struct.unpack(self.data[8:12])
        self.Fields += [("CodePage", "08x")]


class DarwinDataBlock(ExtraDataBlock):
    def parseData(self):
        ExtraDataBlock.parseData(self)
        self.DarwinDataAnsi = readASCIIZ(self.data[8:])
        self.DarwinDataUnicode = readASCIIZ(self.data[268:], 2)
        self.Fields += [("DarwinDataAnsi", "s"),
                        ("DarwinDataUnicode", "s")]


class EnvironmentVariableDataBlock(ExtraDataBlock):
    def parseData(self):
        ExtraDataBlock.parseData(self)
        self.TargetAnsi = readASCIIZ(self.data[8:])
        self.TargetUnicode = readASCIIZ(self.data[8:])
        self.Fields += [("TargetAnsi", "s"),
                        ("TargetUnicode", "s")]


class IconEnvironmentDataBlock(ExtraDataBlock):
    def parseData(self):
        ExtraDataBlock.parseData(self)
        self.TargetAnsi = readASCIIZ(self.data[8:])
        self.TargetUnicode = readASCIIZ(self.data[8:])
        self.Fields += [("TargetAnsi", "s"),
                        ("TargetUnicode", "s")]


class KnownFolderDataBlock(ExtraDataBlock):
    def parseData(self):
        ExtraDataBlock.parseData(self)
        (KFID1, KFID2, KFID3, KFID4) = struct.unpack("=LLLL", self.data[8:24])
        (self.Offset,) = struct.unpack("=L", self.data[24:32])
        self.KnownFolderID = [KFID1, KFID2, KFID3, KFID4]
        self.Fields += [("KnownFolderID", "LST"),
                        ("Offset", "08x")] #offset into IDList


class PropertyStoreDataBlock(ExtraDataBlock):
    def parseData(self):
        pass


class ShimDataBlock(ExtraDataBlock):
    def parseData(self):
        self.LayerName = readASCIIZ(self.data[8:], 2)
        self.Fields += [("LayerName", "s")]


class SpecialFolderDataBlock(ExtraDataBlock):
    def parseData(self):
        (self.SpecialFolderID, 
        self.Offset) = struct.unpack("=LL", self.data[8:16])
        self.Fields += [("SpecialFolderID", "08x"),
                        ("Offset", "08x")] #offset into IDList
    pass


class TrackerDataBlock(ExtraDataBlock):
    pass


class VistaAndAboveIDListDataBlock(ExtraDataBlock):
    pass
        

class LinkFile(LinkStructBase):
    """Class for Link Files"""
    def __init__(self, fi):
        idx = 76
        strFactor = 1
        self.data = fi.read();
        self.Header = LinkFileHeader(self.data)
        if self.Header.LinkFlags & HasLinkTargetIDList != 0:
            (size,) = struct.unpack("=H", self.data[idx:idx + 2]);
            self.IDList = LinkFileIDList(self.data[idx:idx + size + 2])
            idx += size + 2
        if self.Header.LinkFlags & HasLinkInfo:
            (size,) = struct.unpack("=L", self.data[idx:idx + 4]);
            self.LinkInfo = LinkInfo(self.data[idx: idx + size]);
            idx += size
        idx = self.parseStrings([(HasName, "NameString"),
                                 (HasRelativePath, "RelativePath"),
                                 (HasWorkingDir, "WorkingDir"),
                                 (HasArguments, "Arguments"),
                                 (HasIconLocation, "IconLocation")], idx)
        self.ExtraData = ExtraData(self.data[idx:])

    def parseStrings(self, strList, idx):
        strFactor = 1
        if self.Header.LinkFlags & IsUnicode:
            strFactor = 2
        for flags, member in strList:
            if self.Header.LinkFlags & flags:
                (size,) = struct.unpack("=H", self.data[idx: idx + 2])
                setattr(self, member, StringData(self.data[idx:idx + 2 + size * strFactor], member, self.Header.LinkFlags))
                idx += size * strFactor + 2
        return idx

    Fields = [("Header", "OBJ"),
              ("IDList", "OBJ"),
              ("LinkInfo", "OBJ"),
              ("strList", "LST"),
              ("ExtraData", "OBJ")]


def dumpFile(fileName = ""):
    fi = open(fileName, "rb")
    lf = LinkFile(fi)
    print lf.getString("  ")
    fi.close()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "usage: ilink file.lnk"
    else:
        dumpFile(sys.argv[1])

