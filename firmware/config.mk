APP=rover_firmware

# Add lib modules with your custom libraries
MODULES=app \
        app/lib/uart_comm \
        app/lib/uart_mef \
        app/lib/communication \
        app/lib/motor \
        lpc_chip_43xx \
        lpc_board_ciaa_edu_4337

DEFINES=CORE_M4 __USE_LPCOPEN __USE_NEWLIB

VERBOSE=n
OPT=g
USE_NANO=y
SEMIHOST=n
USE_FPU=y