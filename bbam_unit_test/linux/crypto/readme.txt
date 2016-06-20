Introduction
-------------
    This cryptogrphy module is implemented and tested in kernel space. A
skeleton is construted to support all algorithms. All algorithms are
implemented in software by default, While some SOC or platform supports
hardware encryption and decryption. The ambarella A5S hardware cryptography
engine is able to perform AES and DES operations.
    See linux\Documentation\crypto for details.

Compilation
------------
    You should select "Testing module", "Hardware crypto devices",
"ambarella soc driver for AES&DES algorithm" in menuconfig_public_linux, and
compile it into modules.

Usage
------------
1). insmod ambarella_crypto.ko config_polling_mode=1  (insmod the driver in
polling mode.)
	insmod ambarella_crypto.ko config_polling_mode=0  (insmod the driver in
interrupt mode.)
    This will register and validate the aes algorithm.

2). modprobe tcrypt mode=200 sec=1 (Note:if driver works in interrupt
mode, warning will be found. Just, please, use "ctrl+c", to stop warning and
throw the first result. Do the test again, no warning will display).
    Run the tcrypt to test the speed of encryption and decryption of the selected
algorithms.
    It will select driver according to the driver's cra_priority, if there are servral
driver for an algorithms.

3). if you want to test des, just set mode=204.

Test Output(cpu clock is 468M)
-----------
aes-test-generic.txt
    speed test of aes with software implmentions.
aes-test-hw-polling.txt
    speed test of aes with ambarella hw engine in polling mode.
aes-test-hw-interrupt.txt
    speed test of aes with ambarella hw engine in interrupt mode.

                                                 2009/09/23 - Qiao
