#pragma once
#include <synhal.h>
#include "steppermotor.h"
#include "synrpc_usbcon.h"

extern CountingTimer tim_1;
extern CountingTimer tim_2;
extern CountingTimer tim_3;
extern CountingTimer tim_4;
extern CountingTimer tim_5;

extern syn::MailBox<syn::motorcontrolMsg, 100> controlqueue;
extern syn::MailBox<syn::commandMsg, 10> commandqueue;