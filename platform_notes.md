OSX
sleep seems to share a mutex with waiting for a conditional variable on OSX.  This leads to deadlock if a conditional variable is used and sleep is interrupted by a signal.