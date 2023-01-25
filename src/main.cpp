#include <synhal.h>
#include <stdio.h>
#include "globals.h"

// uncomment when using usb-synrpc
#include "synrpc_usbcon.h"
syn::UsbRpc::Handler packethandler;

syn::Mutex mut;

static void printf_int(char const *s, int i)
{
  mut.lock();
  printf(s, i);
  mut.unlock();
}

// initialize global timers with respective timer number
CountingTimer tim_1(1);
CountingTimer tim_2(2);
CountingTimer tim_3(3);
CountingTimer tim_4(4);
CountingTimer tim_5(5);

// initialize global message queues for receiving commands
syn::MailBox<syn::motorcontrolMsg, 100> controlqueue;
syn::MailBox<syn::commandMsg, 10> commandqueue;

// initialize file local stepper controlers
// TODO set the correct direction GPIO used on the actual cableing
Stepper stepper_1(tim_1, syn::Gpio('A', 1));
Stepper stepper_2(tim_2, syn::Gpio('A', 1));
Stepper stepper_3(tim_3, syn::Gpio('A', 1));
Stepper stepper_4(tim_4, syn::Gpio('A', 1));
Stepper stepper_5(tim_5, syn::Gpio('A', 1));

// temporary variable not on stack
syn::motorpositionMsg motposmsg;

// stacksize for the motor thread, in registers (4 byte each)
#define MOTOR_STACK_SIZE 1024

class MotorTask : public syn::Thread
{
public:
  MotorTask() : syn::Thread("MOT_Task", SYN_OS_PRIO_NORMAL, MOTOR_STACK_SIZE, stack)
  {
  }

  void run()
  {
    steppers[0] = &stepper_1;
    steppers[1] = &stepper_2;
    steppers[2] = &stepper_3;
    steppers[3] = &stepper_4;
    steppers[4] = &stepper_5;

    for(auto& step : steppers)
    {
      step->init();
    }

    int32_t next_pos_update = 100;
    while (1)
    {
      process_command_message();
      process_control_message();
      // tick steppers so they can update their status
      for(auto& step : steppers)
      {
        step->tick();
      }
      // send current motor position back to host every 100ms
      if(syn::System::milliseconds() >= next_pos_update)
      {
        next_pos_update = syn::System::milliseconds() + 100;
        
        for(uint16_t i = 0; i < motposmsg.POSITION_SIZE; ++i)
        {
          motposmsg.position[i] = steppers[i]->get_position();
        }
        packethandler.sendMessage(motposmsg, 1);
      }
    }
  }

  void process_command_message()
  {
    // send general commands to motor controler
    // 1 motors on  -> RAMPS enable pin ??
    // 2 motors off
    // 3 initialize (find motor zero position and reset current position) -> Goto endstop, set position zero
    syn::commandMsg msg;
    if(commandqueue.try_pop(msg))
    {
      printf_int("Got Command %d\n", msg.command);
    }
  }

  void process_control_message()
  {
    // confirm all motors done before fetching new message
    for(auto& step : steppers)
    {
      if(step->is_running())
      {
        return;
      }
    }

    // fetch new message and turn on all steppers
    syn::motorcontrolMsg control_msg;
    if(controlqueue.try_pop(control_msg))
    {
      // got a motor control message, run to position
      for(uint16_t i = 0; i < sizeof(steppers); ++i)
      {
        printf_int("Set stepper %d\n", i);
        steppers[i]->set_speed(control_msg.speed[i]);
        steppers[i]->set_position(control_msg.position[i], *this);
      }
    }
  }

private:
  Stepper* steppers[5];
  uint32_t stack[MOTOR_STACK_SIZE];
};


// class LPTask : public syn::Thread
// {
// public:
//   LPTask() : syn::Thread("LP_Task", 50, 256, 0)
//   {
//   }

//   void run()
//   {
//     int i = 0;
//     while (1)
//     {
//       _Write("LPTask %d\n", i++);
//       sleep(100);
//     }
//   }

//   //uint32_t stack[256];
// };


MotorTask mottask;
//LPTask lptask;

int main(void)
{
  syn::System::init();
  // also start the usb-synrpc handler
  packethandler.start();

  mottask.start();
  //lptask.start();

  mut.init();
  /* Start scheduler */
  syn::System::spin();
  /* We should never get here as control is now taken by the scheduler */
  for (;;)
    ;
}
