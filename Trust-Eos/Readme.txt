Truested Embedded operating system to run:
=========================================

1. The programming tool (DNW) and the u-boot.bin or file system utv210_root.img(Target Board with Data Disk) 
   burn into the target version of S5PV210£» (See the UT-S5PV210 UserManual for more details)

2. Through the DNW, when the u-boot starts up normally, burning the trusted operating system (T-OS):dnw 5f000000;

3. Through the DNW, when the T-OS programmer is successful, Linux kernal (linux_zimage): dnw 30000000;

4. When all image files are successful programmer can start: go 5f000000£»

5. When the target board to start, through the SD-Card or ADB push upload TrustZone driver and the Client API 
   and the application of demo to the target board and loading£»

6. Now£¬congratulations, trusted operating system run success£»
