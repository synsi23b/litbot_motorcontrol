
#include "synrpc_usbcon.h"

/* Here come the handler definitions. They will be evoked with the message in an internal buffer
 * and on the PacketHandler Thread, thus any RTOS function calls are allowed.
 * The packet reception continues in the background to another buffer. If you stall too long, it
 * will start to stall the Host.
 * Since the programmer is responsible for keeping align requirements, please be careful!
 * Also, there will be a check of the packets checksum before it gets here, An error will be send automatically
 * in case of mismatched messages.
 * Furthermore, returning anything else other than 0 will be regarded as an error and interpreted as
 * a pointer to a zero terminated C-String. This string will be copied into the internal Error-Message buffer.
 * It allows only a maximum size of 26 characters. Anything more will be discarded. You have been warned.
 */
/// USER INCLUDES ///
#include "globals.h"
/// USER INCLUDES END ///

/* motorcontrolMsg message definition
* # set the motor position 0 .. 100 %
* uint8 position[5]
* # set the speed at which each motor is supposed to move 0 .. 100%
* uint8 speed[5]
*/
const char* syn::motorcontrolHandler(const syn::motorcontrolMsg& msg){
  (void)msg; // avoid "unused variable" compiler warning
/// USER CODE motorcontrolMsg ///
  controlqueue.push(msg);
/// END USER CODE ///
  return 0;
}


/* motorpositionMsg message definition
* # return the current motor position 0 .. 100 %
* uint8 position[5]
*/
const char* syn::motorpositionHandler(const syn::motorpositionMsg& msg){
  (void)msg; // avoid "unused variable" compiler warning
/// USER CODE motorpositionMsg ///
  // we only send this message back to the host, so just do nothing here is ok
/// END USER CODE ///
  return 0;
}


/* commandMsg message definition
* # send general commands to motor controler
* # 1 motors on
* # 2 motors off
* # 3 initialize (find motor zero position and reset current position)
* uint16 command
*/
const char* syn::commandHandler(const syn::commandMsg& msg){
  (void)msg; // avoid "unused variable" compiler warning
/// USER CODE commandMsg ///
  commandqueue.push(msg);
/// END USER CODE ///
  return 0;
}

