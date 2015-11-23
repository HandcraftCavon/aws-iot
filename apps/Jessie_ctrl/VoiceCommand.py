from SunFounder_PiPlus import *
import random
import os

def setup():
	global MAX_FILE_NUMBER, filenum
	global Bzr, Btn, RGB
	global valueDict,listen
	
	MAX_FILE_NUMBER = 1000
	filenum = 0
	
	Bzr = Buzzer(port='B')
	Btn = Buttons(port='B')
	RGB = RGB_LED(port = "B")
	
	os.system("rm -r temp")
	os.system("mkdir temp")
	
	Btn.add_event_detect(up_falling = up)
	
	language = 'en'
	listen = "arecord -D plughw:1,0 -f cd -t wav -d 3 -r 16000 | flac - -f --best --sample-rate 16000 -o /dev/shm/out.flac 1>/dev/shm/voice.log 2>/dev/shm/voice.log; " + 'curl -X POST --data-binary @/dev/shm/out.flac' + " --user-agent 'Mozilla/5.0' --header 'Content-Type: audio/x-flac; rate=16000;' " + '"https://www.google.com/speech-api/v2/recognize?output=json&lang=' + language + '&key=AIzaSyBOti4mM-6x9WDnZIjIeyEU21OpBXqWBgw&client=Mozilla/5.0"' + " | sed -e 's/[{}]/''/g' | awk" + ' -F":"' + " '{print $4}' | awk " + '-F"," ' + "'{print $1}' | tr -d '\n'"

	valueDict = {'botStatus':'stop', 'music':'off', 'light':'off'}

def file_count():
	global MAX_FILE_NUMBER
	if len(os.listdir('./temp')) > MAX_FILE_NUMBER:
		return False
	else:
		return True

def writeFile(valuename):
	global filenum, valueDict
	filedir = './temp/' + str(filenum)
	line = valuename + '=' + valueDict[valuename]
	#print filedir
	f = open(filedir, 'w')
	f.write(line)
	f.close()
	filenum += 1
	
def up(chn):
	print 'Say the command'
	status, command=commands.getstatusoutput(listen)
	print command
	command =  command.split('"')[1]
	print command
	voiceRecogize(command)

def findValue(sentence, word):	# Find if sentence includes word.
	try:
		if sentence.index(word) >= 0:
			return True
	except:
		return False

def voiceRecogize(commandline):
	#print command.strip()
	print 'command = ' + commandline
	if findValue(commandline, "don't"):
		if findValue(commandline, "move"):
			print 'Robot stopping'
			valueDict['botStatus'] = 'stop'
		else:
			return 0

	if findValue(commandline, 'stop'):
		print 'Robot stopping'
		valueDict['botStatus'] = 'stop'
	if  findValue(commandline, 'light') or findValue(commandline, 'lamp'):
		if findValue(commandline, 'on'):
			print 'Lamp ON'
			RGB.rgb(100, 100, 100)
			valueDict['light'] = 'on'
		elif findValue(commandline, 'off'):
			print 'Lamp OFF'
			RGB.rgb(0, 0, 0)
			valueDict['light'] = 'off'
		else:
			print "Lamp operation no found, Light only oprate on or off"

	if findValue(commandline, 'music'):
		if findValue(commandline, 'on'):
			print 'Music ON'
			RGB.rgb(100, 0, 0)
			valueDict['music'] = 'on'
		elif findValue(commandline, 'off'):
			print 'Music OFF'
			RGB.rgb(0, 0, 100)
			valueDict['music'] = 'off'
		else:
			print "Music operation no found, Music only oprate on or off"

	if findValue(commandline, 'forward'):
		print 'Robot going forward'
		valueDict['botStatus'] = 'forward'
	elif findValue(commandline, 'backward'):
		print 'Robot going backward'
		valueDict['botStatus'] = 'backward'
	elif findValue(commandline, 'left'):
		print 'Robot turning left'
		valueDict['botStatus'] = 'left'
	elif findValue(commandline, 'right'):
		print 'Robot turing right'
		valueDict['botStatus'] = 'right'
	'''
	if findValue(commandline, 'move') or findValue(commandline, 'go'):
		if findValue(commandline, 'forward'):
			print 'Robot going forward'
			valueDict['botStatus'] = 'forward'
		elif findValue(commandline, 'backward'):
			print 'Robot going backward'
			valueDict['botStatus'] = 'backward'
		elif findValue(commandline, 'left'):
			print 'Robot turning left'
			valueDict['botStatus'] = 'left'
		elif findValue(commandline, 'right'):
			print 'Robot turing right'
			valueDict['botStatus'] = 'right'
	'''
	return 0

def main():
	global valueDict, command

	tmpBotStatus = 'stop'
	tmpLight = 'off'
	tmpMusic = 'off'
	while True:
		if not file_count():
			Bzr.beep(dt=1, times=1)
		else:				
			if valueDict['light'] != tmpLight:
				tmpLight = valueDict['light']
				print 'Light: ' + valueDict['light']
				writeFile('light')

			if valueDict['music'] != tmpMusic:
				tmpMusic = valueDict['music']
				print 'Music: ' + valueDict['music']
				writeFile('music')

			if valueDict['botStatus'] != tmpBotStatus:
				tmpBotStatus = valueDict['botStatus']
				print 'Robot Status: ' + valueDict['botStatus']
				writeFile('botStatus')
	
def destroy():
	Btn.destroy()
	Bzr.destroy()
	RGB.destroy()
	GPIO.cleanup()

if __name__ == "__main__":
	try:
		setup()
		main()
	except KeyboardInterrupt:
		destroy()
