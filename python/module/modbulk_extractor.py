from ctypes import *

class BeHandle(Structure):
    _fields_ = [("descriptor", c_int)]

#                      return type, flag,  arg,    feature_recorder_name, feature,  feature_len, context,  context_len
BeCallback = CFUNCTYPE(c_int,       c_int, c_uint, c_char_p,              c_char_p, c_size_t,    c_char_p, c_size_t   )

lib_be = cdll.LoadLibrary("libfakelib.so")
lib_be.bulk_extractor_open.restype = POINTER(BeHandle)
lib_be.bulk_extractor_analyze_dev.argtypes = [POINTER(BeHandle), BeCallback, c_char_p]
lib_be.bulk_extractor_analyze_dir.argtypes = [POINTER(BeHandle), BeCallback, c_char_p]
lib_be.bulk_extractor_analyze_buf.argtypes = [POINTER(BeHandle), BeCallback, POINTER(c_ubyte), c_size_t]

def bulk_extractor_open():
    return lib_be.bulk_extractor_open();
def bulk_extractor_analyze_dev(handle, cb, device):
    cb = BeCallback(cb)
    if type(device) == str:
        device = bytes(device, 'utf-8')
    return lib_be.bulk_extractor_analyze_dev(handle, cb, device)
def bulk_extractor_analyze_dir(handle, cb, directory):
    cb = BeCallback(cb)
    if type(directory) == str:
        directory = bytes(directory, 'utf-8')
    return lib_be.bulk_extractor_analyze_dir(handle, cb, directory)
def bulk_extractor_analyze_buf(handle, cb, buf):
    cb = BeCallback(cb)
    if type(buf) == str:
        buf = bytearray(buf, 'utf-8')
    buf = (c_ubyte * len(buf)).from_buffer(buf)
    return lib_be.bulk_extractor_analyze_buf(handle, cb, buf, len(buf))
def bulk_extractor_close(handle):
    return lib_be.bulk_extractor_close(handle)

def main():
    def callback(flag, arg, recorder_name, feature, feature_len, context, context_len):
        args = locals()
        for argname in args:
            print(str(argname), ":", str(args[argname]), end=" ")
        print()
        return 0;

    handle = bulk_extractor_open()
    print(handle)

    bulk_extractor_analyze_dev(handle, callback, "/dev/null")
    bulk_extractor_analyze_dir(handle, callback, "/home/nobody")
    bulk_extractor_analyze_buf(handle, callback, "abcdefghijklmnopqrstuvwxyz0123456789")

    bulk_extractor_close(handle)

if __name__ == "__main__":
    main()
