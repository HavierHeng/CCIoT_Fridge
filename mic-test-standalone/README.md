# Mic-Test-Standalone

Everyone hates Mics in ESP-32. This one just tests INMP441 mic and gets it to upload the WAV file via a seperate task to a web server.

The key things being tested here are
- SD Card
- HTTP Client
    - The uploading task is on its own Task
- INMP441 
    - It is apparently way more complex than it sounds. To get the ordering of data recorded is a nightmare in and of itself, with different outputs depending on whether you are recording in 8 or 16 bit depth (which necessitates you swap every 2 bit) or whether you are recording in stereo or mono.
    - In the setup, the INMP441 is strictly set to mono via a pulldown on the LRCLK
    - The recording task is also on its own Task
- WAV headers
    - These 44 bytes have to be painstakingly hardcoded 
- Double buffering
    - The file that is being recorded to is the not one being uploaded at any point. 
    - This way I can continuously record nonstop

Remember to close file handles
Ordering of task priority matters - else some tasks will never have the chance to do things.

## Components to menuconfig
Configure screen pinout
Configure SD card pinout and settings
Configure Long file names in FAT Filesystem support


## To run python server

Just use `python3 python_server.py`

You may test uploading using: `curl -X PUT --data-binary @<source_file> http://localhost:8080/<server_file_path>`

This PUT will be done using ESP32 in practice.
