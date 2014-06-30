#!/usr/bin/env python2
"""
Analyze drives and other bulk data with bulk_extractor.  Features located are
returned through a callback function; it is up to the recipient to organize and
store the data.  When using a Session object histogram data is managed
internally and is available as a dictionary of feature recorder names to
histograms after the closure of the session.  See bulk_extractor documentation
for general operating guidelines.

It is recommended to use init() and Session, but low-level access functions are
provided as well.

bulk_extractor configuration can only be set once, and must be set by calling
init() before using a Session object.
"""

from collections import namedtuple
from ctypes import *
import os

class Session(object):
    """
    Single bulk_extractor session.  Module-level init() must be called before
    opening a session.  Any number of drives and buffers may be processed to
    contribute to a common set of histograms.  Feature data is returned by an
    optional feature_callback, and histograms are made available in a dict
    after finalizing the session.  Because bulk_extractor is long-running, a
    heartbeat callback may be provided for periodic reassurance that progress
    is being made.
    """
    def __init__(self,
            histogram_limit=50,
            feature_callback=lambda a,b,c,d,e: 0,
            carving_callback=lambda: 0,
            heartbeat_callback=lambda: 0,
            user_arg=None):
        """
        Prepare for bulk_extractor use.  Max histogram size and feature/carving
        callbacks may be configured on a per-session basis but no other
        configuration is possible here.

        callback signatures:

        feature_callback(user_arg, recorder_name, forensic_path, feature,
                context)
        carving_callback() (not yet implemented by bulk_extractor)
        heartbeat_callback()
        """
        if not initialized():
            raise BulkExtractorException("Session opened before init")

        # wrap supplied callback to handle some callbacks transparently where
        # appropriate and to allow implicit access to self by scope since C
        # won't supply self as an explicit first argument.
        def _callback_wrapper(user, code, arg, name, fpath, feature,
                feature_len, context, context_len):
            # features and carved files have no sensible default handling, so
            # punt to the supplied callbacks.
            if code == API_CODE_FEATURE:
                result = feature_callback(user, name, fpath, feature, context)
                if result is None:
                    result = 0
                return result
            if code == API_CODE_CARVED:
                result = carving_callback()
                if result is None:
                    result = 0
                return result
            # histogram data is organized into a dictionary by feature recorder
            if code == API_CODE_HISTOGRAM:
                if name not in self._histograms:
                    self._histograms[name] = list()
                # ignore histogram entries with zero counts because they can't
                # occur in real data
                if arg == 0:
                    return 0
                self._histograms[name].append(HistElem(
                    feature=feature, count=arg))
                return 0
            if code == API_CODE_HEARTBEAT:
                result = heartbeat_callback()
                if result is None:
                    result = 0
                return result
            # pythonify exceptions raised by bulk_extractor
            if code == API_EXCEPTION:
                # Exceptional callbacks place the reason in name and supply the
                # forensic path that caused the issue.
                raise BulkExtractorException("{} @ {}".format(name, fpath))
            # unknown callbacks are exceptional rather than being ignored or
            # passed to a user callback
            raise BulkExtractorException(
                    "unknown callback code {}".format(code))
        # keep reference to BeCallback to avoid garbage collection and
        # heisenbuggy segfaults
        self._callback = BeCallback(_callback_wrapper)
        self._handle = open_handle(self._callback)

        global scanners
        global featurefiles
        self._scanners = scanners
        self._featurefiles = featurefiles
        self._histograms = dict()

        # PROCESS_COMMANDS is necessary to load feature recorders, calling it
        # without having any commands to process prevents BE from blowing up
        # for modifying config twice.
        configure(self._handle, PROCESS_COMMANDS, "", 0)
        # switching to in-memory mode must be done for each handle and must be
        # done after the feature_recorder_set is populated (by
        # PROCESS_COMMANDS)
        configure(self._handle, MEMHIST_ENABLE, "", 0);
        for featurefile in self._featurefiles:
            configure(self._handle, FEATURE_DISABLE, featurefile, 0);
            configure(self._handle, MEMHIST_LIMIT, featurefile,
                    histogram_limit);

    def analyze_buffer(self, buf):
        """Run bulk_extractor over the supplied buffer."""
        handle = self._get_handle()
        analyze_buffer(handle, buf)

    def analyze_device(self, path, sample_rate, sample_size):
        """
        Run bulk_extractor in sampling mode over the device at the given path.
        sample_rate is a value between 0 and 1 that dictates the percentage of
        the device to sample.  sample_size is the size in bytes of individual
        samples to capture.
        """
        handle = self._get_handle()
        if type(path)==str: path=path.encode('latin1')
        analyze_device(handle, path, sample_rate, sample_size)

    def scanners(self, scanners=None):
        """Return the list of active scanners."""
        return list(self._scanners)

    def featurefiles(self):
        """Return the list of active feature files."""
        return list(self._featurefiles)

    def histograms(self):
        """
        Return the dictionary of histograms.  This will be empty before
        calling finalize().
        """
        return dict(self._histograms)

    def finalize(self):
        """
        End the bulk_extractor session.  It is exceptional to interface with
        bulk_extractor through this object after calling this method.
        """
        handle = self._get_handle()
        close_handle(handle)
        self._handle = None

    def _get_handle(self):
        """Private handle access with handling of use-after-close."""
        if self._handle is None:
            raise BulkExtractorException("use after close")
        return self._handle

HistElem = namedtuple("HistElem", "count feature")

class BulkExtractorException(Exception):
    """
    Exception encompasing incorrect bulk_extractor use and errors reported
    by bulk_extractor.
    """
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

# because bulk_extractor still has global variables, some configuration must be
# done at the module level.
scanners = None
featurefiles = None
def init(requested_scanners, lib_name=None):
    """
    Configure the global state of bulk_extractor.  Set the list of enabled
    scanners and disable all others.  This function must be called exactly once
    and must be called before using a Session object.
    """
    if initialized():
        raise BulkExtractorException("init may only be called once")

    lib_init(lib_name)

    global scanners
    global featurefiles
    scanners = set(requested_scanners)
    featurefiles = set()

    # short-lived handle for configuration and feature file enumeration
    def _callback(user, code, arg, name, fpath, feature, feature_len,
            context, context_len):
        if code == API_CODE_FEATURELIST:
            featurefiles.add(name)
            return 0
        if code == API_CODE_HISTOGRAM:
            # ignore histograms since they're always produced on handle close
            # but we don't need them
            return 0
        raise BulkExtractorException("non FEATURELIST callback " +
                "code {} during init()".format(code))
    callback = BeCallback(_callback)
    handle = open_handle(callback)

    # set enabled scanners
    configure(handle, DISABLE_ALL, "", 0)
    for scanner in scanners:
        configure(handle, SCANNER_ENABLE, scanner, 0)
    configure(handle, PROCESS_COMMANDS, "", 0)
    # featurefiles will be populated by callbacks during this call
    configure(handle, FEATURE_LIST, "", 0)

    # set in-memory mode
    configure(handle, MEMHIST_ENABLE, "", 0);
    for featurefile in featurefiles:
        configure(handle, FEATURE_DISABLE, featurefile, 0);
        configure(handle, MEMHIST_LIMIT, featurefile, 0);

    close_handle(handle)

def soft_init(requested_scanners):
    """
    Initialize bulk_extractor iff it isn't already.  Beware of subtle bugs when
    session behavior appears wrong because initialization was done differently
    beforehand.  This method will silently allow old configuration to take
    precedence.
    """
    if initialized():
        return
    init(requested_scanners)

def initialized():
    """Return whether bulk_extractor's global state has been initialized."""
    global scanners
    global featurefiles

    return scanners is not None or featurefiles is not None

#
# Low-level access
#

def open_handle(callback, user_arg=None):
    """Start a new bulk_extractor session with the supplied callback."""
    return lib_be.bulk_extractor_open(user_arg, callback);
def configure(handle, cmd, scanner, arg):
    """
    Configure the given handle.  Send a command with an argument to the named
    scanner.  See bulk_extractor documentation for details.
    """
    if type(scanner)==str: scanner=scanner.encode('latin1')
    return lib_be.bulk_extractor_config(handle, cmd, scanner, arg)
def analyze_buffer(handle, buf):
    """
    Analyze the supplied buffer with bulk_extractor, returning results via the
    given handle's callback.  Buf can be either a bytes object or a string.
    """
    return lib_be.bulk_extractor_analyze_buf(handle, buf, len(buf))
def analyze_device(handle, path, sample_rate, sample_size):
    """
    Analyze the device at the given path with bulk_extractor, returning results
    via the given handle's callback.  Sample rate is a value in (0, 1]
    indicating the fraction of the drive to analyze.  Sample size dictates the
    size in bytes of each sample.
    """
    return lib_be.bulk_extractor_analyze_dev(handle, path, sample_rate,
            sample_size)
def close_handle(handle):
    """
    End the given bulk_extractor session.  Trigger the return of histogram
    data via the given handle's callback.
    """
    return lib_be.bulk_extractor_close(handle)

lib_be = None

def lib_init(lib_name=None):
    """Initialize the underlying bulk_extractor shared library."""

    global lib_be
    if lib_be is not None:
        return

    if "BULK_EXTRACTOR_LIB_PATH" in os.environ:
        lib_be = cdll.LoadLibrary(os.environ["BULK_EXTRACTOR_LIB_PATH"])
    elif lib_name is None:
        lib_be = cdll.LoadLibrary("libbulkextractor.so")
    else:
        lib_be = cdll.LoadLibrary(lib_name)

    lib_be.bulk_extractor_open.restype = BeHandle
    lib_be.bulk_extractor_open.argtypes = [
            c_void_p,   # arbitrary user data
            BeCallback, # callback for the duration of the handle
            ]
    lib_be.bulk_extractor_config.restype = None
    lib_be.bulk_extractor_config.argtypes = [
            BeHandle, # session obtained from bulk_extractor_open
            c_uint32, # configuration command
            c_char_p, # affected scanner
            c_int64,  # configuration argument
            ]
    lib_be.bulk_extractor_analyze_buf.restype = c_int
    lib_be.bulk_extractor_analyze_buf.argtypes = [
            BeHandle, # session obtained from bulk_extractor_open
            c_char_p, # the buffer
            c_size_t, # buffer length
            ]
    lib_be.bulk_extractor_analyze_dev.restype = c_int
    lib_be.bulk_extractor_analyze_dev.argtypes = [
            BeHandle, # session obtained from bulk_extractor_open
            c_char_p, # path to the device
            c_float,  # fraction of the drive to sample
            c_int,    # size of each sample (in bytes?)
            ]
    lib_be.bulk_extractor_close.restype = c_int
    lib_be.bulk_extractor_close.argtypes = [
            BeHandle, # session obtained from bulk_extractor_open
            ]


# see also bulk_extractor_api.h

# callback type codes
API_CODE_HEARTBEAT    =    0
API_CODE_FEATURE      =    1
API_CODE_HISTOGRAM    =    2
API_CODE_CARVED       =    3
API_CODE_FEATURELIST  =   10
API_EXCEPTION         = 1000
# scanner configuration commands
PROCESS_COMMANDS = 0 # commit pending enable/disable commands
SCANNER_DISABLE  = 1
SCANNER_ENABLE   = 2
FEATURE_DISABLE  = 3
FEATURE_ENABLE   = 4
MEMHIST_ENABLE   = 5 # in-memory histogram instead of feature file
MEMHIST_LIMIT    = 6
DISABLE_ALL      = 7
FEATURE_LIST     = 8
SCANNER_LIST     = 9

# handle is opaque to python module for simplicity and loose coupling
BeHandle = c_void_p

BeCallback = CFUNCTYPE(
        c_int,    # return type

        c_void_p, # arbitrary user data
        c_uint32, # callback type code
        c_uint64, # multi-use callback argument
        c_char_p, # feature recorder name or other
        c_char_p, # feature forensic path
        c_char_p, # feature data
        c_size_t, # feature length
        c_char_p, # feature context data
        c_size_t, # context length
        )


if __name__=="__main__":
    print("Program to demonstrate the python module")
    import bulkextractor
    bulkextractor.lib_init('libbulkextractor.so')
    bulkextractor.soft_init(['email','accts'])
    be = bulkextractor.Session()
    be.analyze_buffer(b"  user@company.com  617-555-1212 ")
    be.finalize()
    histograms = be.histograms()
    print(histograms)

