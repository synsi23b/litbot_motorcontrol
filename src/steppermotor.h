#pragma once
#include <synhal.h>

class CountingTimer
{
public:
    CountingTimer(uint16_t number)
    {
        _count = 0;
        _tim.init(number);
    }

    void init()
    {
      // enable interrupts of the timer. triggers every time the timer runs out
        _tim.enableCallback(0);
        // TODO initialize timer PWM, dont start the timer
        //_tim.configStepperPWM();
    }

    void isr()
    {
        if(_count > 0)
        {
            _count -= 1;
            if(_count == 0)
            {
                _tim.stop();
            }
        }
    }

    void run_for_x_times(uint32_t x, syn::Thread& ownning_thread)
    {
        while(!is_finished())
        {
            ownning_thread.sleep(1);
        }
        _count = x;
        _tim.start();
    }

    bool is_finished() const
    {
        return _count == 0;
    }

    uint32_t count() const
    {
        return _count;
    }
private:
    volatile uint32_t _count;
    syn::Timer _tim;
};


class Stepper
{
  static const uint32_t steps_per_percent = 250;
public:
  Stepper(CountingTimer& tim, syn::Gpio direction) : _tim(tim), _direction(direction)
  {
  }

  void init()
  {
    _direction.mode(syn::Gpio::out_push_pull, syn::Gpio::MHz_10);
    _direction.set();
    _current = _goal = 0;
  }

  void set_speed(uint8_t speed_percent)
  {
      // speed -> change Hardware Timer reload value to get a different frequency PWM -> low value = faster
      // TIMx->ARR
      // also set the timer compare value to half the reload value, to get an even 50 / 50 PWM ratio
      // TIMx->CCRy
  }


  void set_position(uint8_t pos_percent, syn::Thread& owning_thread)
  {
      // position -> compare with the current position to figure out the needed direction or if need to move at all
      // calculate the neccesary steps to reach the position, than start the timer to run for that many steps
      if(pos_percent > _current)
      {
        uint32_t steps = (pos_percent - _current) * steps_per_percent;
        _goal = pos_percent;
        _direction.set();
        _tim.run_for_x_times(steps, owning_thread);
      }
      else
      {
        uint32_t steps = (_current - pos_percent) * steps_per_percent;
        _goal = pos_percent;
        _direction.clear();
        _tim.run_for_x_times(steps, owning_thread);
      }
  }

  void tick()
  { 
    // maybe also do things like smooth accelartion / deacceleration here
    if(_tim.is_finished())
    {
      _current = _goal;
    }
  }

  uint8_t get_position()
  {
      // get counting timer current value
      // than calculate the steps needed and done already to find current position
      uint32_t remaining_perc = _tim.count() / steps_per_percent;
      if(_goal >= _current)
      {
        return _goal - remaining_perc;
      }
      else
      {
        return _goal + remaining_perc;
      }
  }

  bool is_finished()
  {
    return _current == _goal;
  }

  bool is_running()
  {
    return _current != _goal;
  }
private:
  CountingTimer& _tim;
  syn::Gpio _direction;
  uint8_t _current;
  uint8_t _goal;
};