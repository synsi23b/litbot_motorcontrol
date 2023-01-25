import struct
import threading
import usb.core
import array
import sys
from time import sleep

# transmitting over usb happens in packets of up to 64 byte
# if we send exactly 64 bytes, the host is not
# sure wether or not the transmission is finished.
# it will than ask the again and we need to transmit a zero length packet
# so by defining the max size at 127 we can finish any transmission in 2 steps and can optimize the device buffer easily
SYNRPC_MAX_MSGSIZE = 127


class _Array(object):
    def __init__(self, parentlist, start, end):
        self._parent = parentlist
        self._start = start
        self._end = end

    def __len__(self):
        return self._end - self._start

    def __getitem__(self, key):
        return self._parent[self._getParentIdx(key)]
        
    def __setitem__(self, key, val):
        self._parent[self._getParentIdx(key)] = val

    def _getParentIdx(self, key):
        if key >= 0:
            key = self._start + key
            if key >= self._end:
                raise IndexError()
        else:
            key = self._end + key # key is negativ, iterate reverse
            if key < self._start:
                raise IndexError()
        return key

    def __iter__(self):
        return _Array._Iter(self._parent, self._start, self._end)

    def __reversed__(self):
        return _Array._Iter(self._parent, self._start, self._end)


    class _Iter:
        def __init__(self, parent, start, stop):
            self._parent = parent
            self._current = start
            self._stop = stop

        def __next__(self):
            if self._current < self._stop:
                val = self._current
                self._current += 1
                return self._parent[val]
            else:
                raise StopIteration()

        next = __next__  # Python 2 compatibility


    class _RevIter:
        def __init__(self, parent, start, stop):
            self._parent = parent
            self._current = stop - 1
            self._stop = start

        def __next__(self):
            if self._current >= self._stop:
                val = self._current
                self._current -= 1
                return self._parent[val]
            else:
                raise StopIteration()

        next = __next__  # Python 2 compatibility


class PacketHandler:
    """Receive and send SynRPCMessages using this handler.
example usage ->
mymsg = BarFooMsg()
mymsg.bla = int('7AAAAAAA', 16)
mymsg.nignog = int('BBBB', 16)
mymsg.data = [int('CC', 16)] * 10

def barfoohhandler(msg):
    print hex(msg.bla), hex(msg.nignog), st

ph = PacketHandler()
ph.addMessageHandler(BarFooMsg, barfoohandler)
ph.start()

while True:
    ph.sendMessage(mymsg)
    sleep(1)
"""
    def __init__(self):
        self._running = False
        self._usbdev = None
        if sys.version_info < (3, 0):
            self._recv = PacketHandler._py2_recv
        else:
            self._recv = PacketHandler._py3_recv
        self._recthread = threading.Thread(target=self._receiver)

    def start(self):
        self._running = True
        self._recthread.start()

    def shutdown(self):
        self._running = False
        self._recthread.join(2.0)
        self._usbdev = None

    def addMessageHandler(self, MsgClass, handler):
        _msgHandlers[MsgClass._type] = handler

    def sendMessage(self, message):
        ret = False
        if self._usbdev:
            msg = array.array('B', message._serialize())
            ret = self._usbdev.write(0x80, msg, 1000)
            ret = (ret - 5) == message._size
        return ret

    def _receiver(self):
        buffer = []
        # find and reset the usb device
        dev = usb.core.find(idVendor=0x0483, idProduct=0x5740)
        if not dev:
            print("Couldn't find SynRPC device")
            self._running = False
        else:
            print("synrpc: found device -> reseting")
            dev.reset()
            print("synrpc: checking kernel driver")
            if dev.is_kernel_driver_active(1):
                print("synrpc: active -> attempt detach")
                dev.detach_kernel_driver(1)
            print("synrpc: loading device default configuration")
            dev.set_configuration()
            self._usbdev = dev
            print("synrpc: reset done, running read loop")
        while self._running:
            # try to read up to 64 bytes with a 1000 millisec timeout just to be able to leave the loop
            # the function will return after a single bulk transfer of even less than 64 bytes
            try:
                arr = dev.read(0x81, 64, 1000)
            except usb.core.USBTimeoutError:
                continue
            if arr:
                buffer += arr.tostring()
            if buffer:
                buffer = self._recv(buffer)

    @staticmethod
    def _py2_recv(buffer):
        # check buffer 0 for expected packet size and if the buffer is big enough
        msgsize = ord(buffer[0])
        if(msgsize < 6):
            # if the indicated message size is below 6, there has to be an error.
            # the meta data is at least 5 bytes and the payload is at least 1 byte.
            buffer.pop(0)
        elif len(buffer) >= msgsize:
            # check if end of packet matches msgsize
            msgsizerev = ord(buffer[msgsize - 1])
            if (SYNRPC_MAX_MSGSIZE - msgsizerev) == msgsize:
                msg = ''.join(buffer[:msgsize])
                if _tryExecMessage(ord(buffer[1]), msg):
                    buffer = buffer[msgsize:]
                else:
                    buffer.pop(0)
            else: # data missmatch, purge first byte and try again
                buffer.pop(0)
        else: # not enough data in yet, sleep a bit
            # probably never happens because of lots and lots of queing in the os driver and all that
            sleep(0.01)
        return buffer
    
    @staticmethod
    def _py3_recv(buffer):
        # check buffer 0 for expected packet size and if the buffer is big enough
        msgsize = buffer[0]
        if(msgsize < 6):
            buffer.pop(0)
        elif len(buffer) >= msgsize:
            # check if end of packet matches msgsize
            msgsizerev = buffer[msgsize - 1]
            if (SYNRPC_MAX_MSGSIZE - msgsizerev) == msgsize:
                msg = bytes(buffer[:msgsize])
                if _tryExecMessage(buffer[1], msg):
                    buffer = buffer[msgsize:]
                else:
                    buffer.pop(0)
            else: # data missmatch, purge first byte and try again
                buffer.pop(0)
        else: # not enough data in yet, sleep a bit
            # probably never happens because of lots and lots of queing in the os driver and all that
            sleep(0.01)
        return buffer

    @staticmethod
    def _errmsgHandler(synrpcerror):
        print(synrpcerror)


_msgHandlers = { 0 : PacketHandler._errmsgHandler }


def _tryExecMessage(msgtype, msg):
    try:
        msg = _msgGenerators[msgtype](msg)
        if msg._type in _msgHandlers:
            try:         
                _msgHandlers[msg._type](msg)
            except Exception as e:
                print("Exception during handler: " + str(e))
            return True
        else:
            print("Warning -> Missing handler for message: " + str(type(msg)))
    except IndexError as e:
        print("Unknown message type: " + str(msgtype))
    except struct.error as e:
        print(e, " - buffer actual length: " + str(len(buffer)))
    except ValueError as e:
        print("Exception during message generation: " + str(e))
    return False


class SynRpcError(object):
    _type = 0
    _size = 32
    _header = int('ffff', 16) << 16 | 0 << 8 | 32
    _footer = SYNRPC_MAX_MSGSIZE - 32
    _packer = struct.Struct("<LB26sB")

    def __init__(self, buffer):
        data = SynRpcError._packer.unpack_from(buffer)
        if data[0] != SynRpcError._header:
            raise ValueError("header missmatch for SynRpcError")
        self.errtype = data[1]
        self.errmsg = data[2]

    def __str__(self):
        return "Type: {} Error: {}".format(_msgNames[self.errtype], self.errmsg)

    @staticmethod
    def _unserialize(buffer):
        return SynRpcError(buffer)


class motorcontrolMsg(object):
    """
Message fields:

 set the motor position 0 .. 100 %
uint8_t position[5]
 set the speed at which each motor is supposed to move 0 .. 100%
uint8_t speed[5]

"""
    _size = 15
    _type = 1
    _sha1 = int('8f57', 16)
    _header = int('8f57', 16) << 16 | 1 << 8 | 15
    _footer = SYNRPC_MAX_MSGSIZE - 15
    _packer = struct.Struct("<L5B5BB")

    def __init__(self, buffer=None):
        if buffer:
            self._data = motorcontrolMsg._packer.unpack_from(buffer)
            if self._data[0] != motorcontrolMsg._header:
                raise ValueError("header missmatch for motorcontrolMsg")
        else:
            self._data = [motorcontrolMsg._header]  + [0] * 5 + [0] * 5 + [motorcontrolMsg._footer]

    def _serialize(self):
        return motorcontrolMsg._packer.pack(*self._data)
    
    @staticmethod
    def _unserialize(buffer):
        return motorcontrolMsg(buffer)

    @property
    def position(self):
        return _Array(self._data, 1, 6)

    @position.setter
    def position(self, value):
        if len(value) == 5:
            idx = 1
            for v in value:
                self._data[idx] = v
                idx += 1
        else:
            raise ValueError("Attempt to overwrite array with wrong length: {} Expected length: 5".format(len(value)))

    @property
    def speed(self):
        return _Array(self._data, 6, 11)

    @speed.setter
    def speed(self, value):
        if len(value) == 5:
            idx = 6
            for v in value:
                self._data[idx] = v
                idx += 1
        else:
            raise ValueError("Attempt to overwrite array with wrong length: {} Expected length: 5".format(len(value)))



class motorpositionMsg(object):
    """
Message fields:

 return the current motor position 0 .. 100 %
uint8_t position[5]

"""
    _size = 10
    _type = 2
    _sha1 = int('5a84', 16)
    _header = int('5a84', 16) << 16 | 2 << 8 | 10
    _footer = SYNRPC_MAX_MSGSIZE - 10
    _packer = struct.Struct("<L5BB")

    def __init__(self, buffer=None):
        if buffer:
            self._data = motorpositionMsg._packer.unpack_from(buffer)
            if self._data[0] != motorpositionMsg._header:
                raise ValueError("header missmatch for motorpositionMsg")
        else:
            self._data = [motorpositionMsg._header]  + [0] * 5 + [motorpositionMsg._footer]

    def _serialize(self):
        return motorpositionMsg._packer.pack(*self._data)
    
    @staticmethod
    def _unserialize(buffer):
        return motorpositionMsg(buffer)

    @property
    def position(self):
        return _Array(self._data, 1, 6)

    @position.setter
    def position(self, value):
        if len(value) == 5:
            idx = 1
            for v in value:
                self._data[idx] = v
                idx += 1
        else:
            raise ValueError("Attempt to overwrite array with wrong length: {} Expected length: 5".format(len(value)))



class commandMsg(object):
    """
Message fields:

 send general commands to motor controler
 1 motors on
 2 motors off
 3 initialize (find motor zero position and reset current position)
uint16_t command

"""
    _size = 7
    _type = 3
    _sha1 = int('6bc8', 16)
    _header = int('6bc8', 16) << 16 | 3 << 8 | 7
    _footer = SYNRPC_MAX_MSGSIZE - 7
    _packer = struct.Struct("<LHB")

    def __init__(self, buffer=None):
        if buffer:
            self._data = commandMsg._packer.unpack_from(buffer)
            if self._data[0] != commandMsg._header:
                raise ValueError("header missmatch for commandMsg")
        else:
            self._data = [commandMsg._header]  + [0] + [commandMsg._footer]

    def _serialize(self):
        return commandMsg._packer.pack(*self._data)
    
    @staticmethod
    def _unserialize(buffer):
        return commandMsg(buffer)

    @property
    def command(self):
        return self._data[1]

    @command.setter
    def command(self, value):
        self._data[1] = value

_msgGenerators = [
    SynRpcError._unserialize,
    motorcontrolMsg._unserialize,
    motorpositionMsg._unserialize,
    commandMsg._unserialize,
]

_msgNames = [
    'SynRpcError',
    'motorcontrolMsg',
    'motorpositionMsg',
    'commandMsg',
]
