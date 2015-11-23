from SunFounder_PiPlus import *
import random
import os

def setup():
	global MAX_FILE_NUMBER, filenum
	global Bzr, Btn, PhR
	global valueDict
	
	MAX_FILE_NUMBER = 1000
	filenum = 0
	
	Bzr = Buzzer(port='B')
	Btn = Buttons(port='B')
	PhR = Photoresistor()
	
	os.system("rm -r temp")
	os.system("mkdir temp")
	
	Btn.add_event_detect(left_both=left, down_both=down, right_both=right)
	
	valueDict = {'buttonLeft':0, 'buttonDown':0, 'buttonRight':0}

def file_count():
	global MAX_FILE_NUMBER
	if len(os.listdir('./temp')) > MAX_FILE_NUMBER:
		return False
	else:
		return True

def writeFile(valuename):
	global filenum, valueDict
	filedir = './temp/' + str(filenum)
	line = valuename + '=' + str(valueDict[valuename])
	#print filedir
	f = open(filedir, 'w')
	f.write(line)
	f.close()
	filenum += 1

def left(chn):
	global valueDict
	valueDict['buttonLeft'] = GPIO.input(Btn.LEFT)
	#print buttonLeft

def down(chn):
	global valueDict
	valueDict['buttonDown'] = GPIO.input(Btn.DOWN)
	#print buttonDown

def right(chn):
	global valueDict
	valueDict['buttonRight'] = GPIO.input(Btn.RIGHT)
	#print buttonRight

def main():
	global valueDict

	tmpButtonLeft = 0
	tmpButtonDown = 0
	tmpButtonRight = 0
	while True:
		if not file_count():
			Bzr.beep(dt=1, times=1)
		else:
			if valueDict['buttonLeft'] != tmpButtonLeft:
				tmpButtonLeft = valueDict['buttonLeft']
				print valueDict['buttonLeft']
				writeFile('buttonLeft')
				
			if valueDict['buttonRight'] != tmpButtonRight:
				tmpButtonRight = valueDict['buttonRight']
				print valueDict['buttonRight']
				writeFile('buttonRight')
				
			if valueDict['buttonDown'] != tmpButtonDown:
				tmpButtonDown = valueDict['buttonDown']
				print valueDict['buttonDown']
				writeFile('buttonDown')
	
def destroy():
	Btn.destroy()
	Bzr.destroy()
	PhR.destroy()
	GPIO.cleanup()

if __name__ == "__main__":
	try:
		setup()
		main()
	except KeyboardInterrupt:
		destroy()
