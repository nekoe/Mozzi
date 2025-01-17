/*
 * MozziGuts.cpp
 *
 * Copyright 2012 Tim Barrass.
 *
 * This file is part of Mozzi.
 *
 * Mozzi by Tim Barrass is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License.
 *
 */

#if !(IS_RENESAS())
#  error "Wrong implementation included for this platform"
#endif

#include <FspTimer.h>


////// BEGIN analog input code ////////

//#define MOZZI_FAST_ANALOG_IMPLEMENTED

#define getADCReading() 0


#define channelNumToIndex(channel) channel
uint8_t adcPinToChannelNum(uint8_t pin) {
  return pin;
}

/** NOTE: Code needed to trigger a conversion on a new channel */
void adcStartConversion(uint8_t channel) {
#warning Fast analog read not implemented on this platform
}

/** NOTE: Code needed to trigger a subsequent conversion on the latest channel. If your platform has no special code for it, you should store the channel from
 *  adcStartConversion(), and simply call adcStartConversion(previous_channel), here. */
void startSecondADCReadOnCurrentChannel() {
#warning Fast analog read not implemented on this platform
}

/** NOTE: Code needed to set up faster than usual analog reads, e.g. specifying the number of CPU cycles that the ADC waits for the result to stabilize.
 *  This particular function is not super important, so may be ok to leave empty, at least, if the ADC is fast enough by default. */
void setupFastAnalogRead(int8_t speed) {
#warning Fast analog read not implemented on this platform
}

/** NOTE: Code needed to initialize the ADC for asynchronous reads. Typically involves setting up an interrupt handler for when conversion is done, and
 *  possibly calibration. */
void setupMozziADC(int8_t speed) {
#warning Fast analog read not implemented on this platform
}

/* NOTE: Most platforms call a specific function/ISR when conversion is complete. Provide this function, here.
 * From inside its body, simply call advanceADCStep(). E.g.:
 void stm32_adc_eoc_handler() {
 advanceADCStep();
 }
*/
////// END analog input code ////////



//// BEGIN AUDIO OUTPUT code ///////

/*
The strategy to output sound on this platform differs somehow from what is usually done in Mozzi.
Usually, Mozzi's circular buffer are read from the outputting device (PWM, DAC...) or committed as
a whole (for the MBED platform) and thus emptied. Once a free spot is present, Mozzi fills it with updateAudio().
Here, the DAC works straight from a buffer, outputting samples from it at a fixed rate. 
This is sweet as we can branch it straight to Mozzi's buffer and it will read from it without
further action from us.
The big difference is that it *does not* empty the buffer, nor notify that something has been
read, which is okay for outputting a periodic signal where just a full period is present
in the buffer.
As a consequence we need to artificially empty the buffer at the same rate that the DAC is reading
it.
*/

FspTimer timer;

#if (EXTERNAL_AUDIO_OUTPUT != true) // otherwise, the last stage - audioOutput() - will be provided by
CircularBuffer<uint16_t> output_buffer;
#include "MozziGuts_impl_RENESAS_analog.hpp"
#endif


//////////////// TIMER ////////////////

#if EXTERNAL_AUDIO_OUTPUT == true
void timer_callback_dummy(timer_callback_args_t __attribute__((unused)) *args){defaultAudioOutput();};
#else
//void timer_callback_dummy(timer_callback_args_t __attribute__((unused)) *args){
void timer_callback_dummy(timer_callback_args_t __attribute__((unused)) *args){output_buffer.read();}; // to empty the buffer (the dac does not take care of it), a bit a waste of timer...
#endif

void timer_init() {
  uint8_t type;
  int8_t tindex = FspTimer::get_available_timer(type);

  if (tindex < 0) {
    tindex = FspTimer::get_available_timer(type, true);
  }

  if (tindex >= 0) {
    timer.begin(TIMER_MODE_PERIODIC, type, tindex, AUDIO_RATE, 50.0,timer_callback_dummy);
    timer.setup_overflow_irq();
  }
  
#if EXTERNAL_AUDIO_OUTPUT != true // we need to set up another timer for dac caring
  // note: it is running at the same speed than the other one, but could not manage
  // to get the other one updating the dac and removing the samples from the buffer…
    tindex = FspTimer::get_available_timer(type);

  if (tindex < 0) {
    tindex = FspTimer::get_available_timer(type, true);
  }

  if (tindex >= 0) {
    FspTimer::force_use_of_pwm_reserved_timer();
    timer_dac.begin(TIMER_MODE_PERIODIC, type, tindex, AUDIO_RATE, 50.0);
    timer_dac.setup_overflow_irq();    
     dtc_cfg_extend.activation_source = timer_dac.get_cfg()->cycle_end_irq;
     timer_dac.open();    
#endif
    timer.open();
  }
  }


inline void audioOutput(const AudioOutput f) {
  output_buffer.write(f+AUDIO_BIAS);
}
#define canBufferAudioOutput() (!output_buffer.isFull())


static void startAudio() {
#if EXTERNAL_AUDIO_OUTPUT != true
  dac_creation(AUDIO_CHANNEL_1_PIN);
#endif
  timer_init(); // this need to be done between the DAC creation and initialization in the case where the on-board DAC is used, hence the ugly repetition here.
#if EXTERNAL_AUDIO_OUTPUT != true
  dac_init();
  R_DTC_Open(&dtc_ctrl, &dtc_cfg);
  R_DTC_Enable(&dtc_ctrl);

  // The following branches the DAC straight on Mozzi's circular buffer.
  dtc_cfg.p_info->p_src = output_buffer.address();
  dtc_cfg.p_info->length = MOZZI_BUFFER_SIZE;
  R_DTC_Reconfigure(&dtc_ctrl, dtc_cfg.p_info);
  timer_dac.start();
#endif
  timer.start();
  
}

void stopMozzi() {
  timer.stop();
}
//// END AUDIO OUTPUT code ///////
