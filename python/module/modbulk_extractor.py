from ctypes import *

class BeHandle(Structure):
    _fields_ = [("descriptor", c_int)]

#               return type,  flag,     arg,      feature_recorder_name, feature,  feature_len, context,  context_len
BeCallback = CFUNCTYPE(c_int, c_uint32, c_uint64, c_char_p,              c_char_p, c_size_t,    c_char_p, c_size_t   )

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

FLAG_FEATURE = 0x0001
FLAG_HISTOGRAM = 0x0002
FLAG_CARVE = 0x0004

class BulkExtractor():
    def __init__(self):
        self.handle = bulk_extractor_open()
        # raw callback supersedes callback convenience wrapping
        self.raw_cb = None
        # default wrapped callbacks
        self.feature_cb = lambda recorder_name,feature,context: None
        self.histogram_cb = lambda recorder_name,feature,count: None
        self.carve_cb = lambda recorder_name,carved_filename,carved_data: None
    # BE object as a context (with BulkExtractor() as b_e:)
    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
        return False
    # wrap a trio of specialized python callbacks into one C callback
    def _wrap_cb(self, feature_cb, histogram_cb, carve_cb):
        if feature_cb is None:
            feature_cb = self.feature_cb
        if histogram_cb is None:
            histogram_cb = self.histogram_cb
        if carve_cb is None:
            carve_cb = self.carve_cb
        def _wrapped_cb(flag, arg, recorder_name, feature, feature_len, context, context_len):
            if flag & FLAG_FEATURE:
                return feature_cb(recorder_name, feature, context)
            elif flag & FLAG_HISTOGRAM:
                return histogram_cb(recorder_name, feature, arg)
            elif flag & FLAG_CARVE:
                return carve_cb(recorder_name, context, feature)
            return -1
        return _wrapped_cb
    def close(self):
        if self.handle is not None:
            bulk_extractor_close(self.handle)
            self.handle = None
    #
    # analysis methods
    #

    # raw analysis method: selects appropriate C callback and calls the
    # requested analysis function with it
    def _analyze(self, func, source, raw_cb, feature_cb, histogram_cb, carve_cb):
        callback = None
        if raw_cb is not None:
            callback = raw_cb
        elif not None in (feature_cb, histogram_cb, carve_cb):
            callback = self._wrap_cb(feature_cb, histogram_cb, carve_cb)
        elif self.raw_cb is not None:
            callback = self.raw_cb
        else:
            callback = self._wrap_cb(self.feature_cb, self.histogram_cb, self.carve_cb)

        func(self.handle, callback, source)

    # user analysis methods: use of optional cb_ arguments allow for a one-run
    # override of the object-wide callback methods.
    def analyze_dev(self, device_path, raw_cb=None, feature_cb=None, histogram_cb=None, carve_cb=None):
        return self._analyze(bulk_extractor_analyze_dev, device_path, raw_cb, feature_cb, histogram_cb, carve_cb)
    def analyze_dir(self, dir_path, raw_cb=None, feature_cb=None, histogram_cb=None, carve_cb=None):
        return self._analyze(bulk_extractor_analyze_dir, dir_path, raw_cb, feature_cb, histogram_cb, carve_cb)
    def analyze_buf(self, buf_data, raw_cb=None, feature_cb=None, histogram_cb=None, carve_cb=None):
        return self._analyze(bulk_extractor_analyze_buf, buf_data, raw_cb, feature_cb, histogram_cb, carve_cb)

def main():
    def raw_callback(flag, arg, recorder_name, feature, feature_len, context, context_len):
        args = locals()
        for argname in args:
            print(str(argname), ":", str(args[argname]), end=" ")
        print()
        return 0;

    def feature_callback(recorder_name, feature, context):
        print("got a feature from", recorder_name, "(", feature, ")", "(", context, ")")
        return 0;
    def histogram_callback(recorder_name, feature, count):
        print("got histogram data from", recorder_name, ":", feature, "x", count)
        return 0;
    def carve_callback(recorder_name, carved_filename, carved_data):
        print("got carved data from", recorder_name, "with filename", carved_filename, "and length", len(carved_data))
        return 0;

    with BulkExtractor() as b_e:
        print(b_e.handle)

        b_e.raw_cb = raw_callback

        b_e.analyze_dev("/dev/null")
        b_e.analyze_dir("/home/nobody")
        b_e.analyze_buf("abcdefghijklmnopqrstuvwxyz0123456789")

        b_e.raw_cb = None
        b_e.feature_cb = feature_callback
        b_e.histogram_cb = histogram_callback
        b_e.carve_cb = carve_callback

        b_e.analyze_dev("/dev/null")
        b_e.analyze_dir("/home/nobody")
        b_e.analyze_buf("abcdefghijklmnopqrstuvwxyz0123456789")

if __name__ == "__main__":
    main()
