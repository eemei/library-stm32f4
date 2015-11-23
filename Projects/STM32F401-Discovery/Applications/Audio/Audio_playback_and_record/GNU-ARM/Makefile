PROJECT_NAME=STM32F401-DISCO

PROJECT_PATH=$(PWD)/..

# Middleware options
USE_USB_DEVICE = 0
USE_USB_HOST   = 1
USE_FATFS      = 1
USE_AUDIO_PDM  = 1

# Drivers options
USE_AUDIO      = 1
USE_ADC        = 0
USE_GYROSCOPE  = 0
USE_RTC        = 0

PROJECT_SOURCES := main.c\
                   stm32f4xx_it.c\
                   system_stm32f4xx.c\
                   usbh_conf.c\
                   waveplayer.c\
                   waverecorder.c

# Use to set additional action in the program makefile rule
PROJECT_POST_PROGRAM :=

include $(STM_COMMON)/GNU-ARM/STM32F401-Discovery.mk
