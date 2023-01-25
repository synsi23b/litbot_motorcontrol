#include "synrpc_usbcon.h"
#include <cstring> // strcopy for error msg
using namespace syn;

// the mailbox into which the usb device will push messages
// it is public, but really isn't.
MailBox<UsbRpc::Packet, SYN_USBRPC_BUFFSIZE> UsbRpc::Handler::_mailbox;

UsbRpc::Handler::Handler() : Thread("PacketHandler", SYN_USBRPC_PRIORITY, SYN_USBRPC_STACKSIZE)
{
}

const char *handlePacket(const UsbRpc::Packet &p);

void UsbRpc::Handler::run()
{
#if (SYN_USBRPC_USELED == 1)
  Led led;
  LedTimer ledtimer(led);
#endif
  Packet *pmsg;
  while (true)
  {
    _mailbox.peek(&pmsg);
#if (SYN_USBRPC_USELED == 1)
    led.on();
    ledtimer.start();
#endif
    const char *err = handlePacket(*pmsg);
    if (err)
    {
      // an error was reported, copy it to packetbuffer and report back to host
      SynRPCError *pe = pmsg->cast<SynRPCError>();
      pe->intype = pmsg->type();
      pe->write_errormsg(err);
      sendMessage(*pe);
    }
    bool mailbox_full = _mailbox.is_full();
    _mailbox.purge();
    if (mailbox_full)
    {
      // we purged a message, it's not full anymore
      UsbRpc::_enable_rx();
    }
  }
}

#if (SYN_USBRPC_USELED == 1)
UsbRpc::Handler::LedTimer::LedTimer(syn::Led &led) : SoftTimer(5, false), _led(led)
{
  _led.off();
}

void UsbRpc::Handler::LedTimer::execute()
{
  _led.off();
}
#endif

template <typename MsgType, typename MsgHandler>
const char *Converter(MsgHandler hndl, const UsbRpc::Packet &p)
{
  if (p.sha1() == MsgType::_sha1)
    return hndl(*(p.cast<MsgType>()));
  return "Sha-1 checksum mismatch";
}
typedef const char *(*converter_t)(const UsbRpc::Packet &p);

template <typename MsgType>
uint16_t Checker(const UsbRpc::Packet &p)
{
  if (p.sha1() == MsgType::_sha1)
    return p.rawSize();
  return 0;
}
typedef uint16_t (*checker_t)(const UsbRpc::Packet &p);

const char* motorcontrolConverter(const UsbRpc::Packet& p){
  return Converter<motorcontrolMsg>(&motorcontrolHandler, p);
}

uint16_t motorcontrolChecker(const UsbRpc::Packet& p){
  return Checker<motorcontrolMsg>(p);
}

const char* motorpositionConverter(const UsbRpc::Packet& p){
  return Converter<motorpositionMsg>(&motorpositionHandler, p);
}

uint16_t motorpositionChecker(const UsbRpc::Packet& p){
  return Checker<motorpositionMsg>(p);
}

const char* commandConverter(const UsbRpc::Packet& p){
  return Converter<commandMsg>(&commandHandler, p);
}

uint16_t commandChecker(const UsbRpc::Packet& p){
  return Checker<commandMsg>(p);
}

converter_t packetconverters[MAX_HANDLER_TYPE] = {
&motorcontrolConverter,
&motorpositionConverter,
&commandConverter,
};

checker_t packetcheckers[MAX_HANDLER_TYPE] = {
&motorcontrolChecker,
&motorpositionChecker,
&commandChecker,
};

const char* handlePacket(const UsbRpc::Packet& p){
  uint8_t type = p.type();
  if(type > MAX_HANDLER_TYPE){
    return "msg type to big";
  }
  else if(type == 0){
    return "msg type is ignored";
  }
  type -= 1;
  return packetconverters[type](p);
}

uint16_t UsbRpc::Handler::plausible(const UsbRpc::Packet &p)
{
  uint8_t type = p.type();
  if (type > MAX_HANDLER_TYPE)
  {
    return 0;
  }
  else if (type == 0)
  {
    return 0;
  }
  type -= 1;
  return packetcheckers[type](p);
}
