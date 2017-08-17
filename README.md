# NECSST_FSL

## Fast System Launch
FSL enables persistent computing in which the state of the system remains intact even after the power is turned off 
and exploits this to significantly improve booting time even compared to conventional power management methods such as `sleep mode' and `hibernation mode'.
To show the effectiveness of FSL, we implement FSL on an in-house embedded test board, which we refer to as the TUNA board, equipped with pseudo new memory.

## Directory Tree
- FSL_Linux: Linux source (version 3.18) for FSL
- FSL_UBOOT: U-Boot for FSL
- FSL_Minicom: Time check feature is added

## Acknowledgment
This work was supported by IT R&D program MKE/KEIT
(No. 10041608, Embedded System Software for New-memory based Smart Device).
