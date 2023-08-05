#ifndef AUDIOCONFIGADAFRUITMACROPAD_H
#define AUDIOCONFIGADAFRUITMACROPAD_H

#if not IS_ADAFRUIT_MACROPAD()
#error This header should be included for Adafruit MacroPad, only
#endif

#define AUDIO_BITS                  12
#define BYPASS_MOZZI_OUTPUT_BUFFER  false
#define AUDIO_BITS_PER_CHANNEL      AUDIO_BITS
#define AUDIO_BIAS                  ((uint16_t) 1<<(AUDIO_BITS-1))

#endif //  #ifndef AUDIOCONFIGADAFRUITMACROPAD_H
