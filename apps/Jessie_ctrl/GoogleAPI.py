from SunFounder_PiPlus import *

def setup():
	global Btn, RGB, command, listen
	Btn = Buttons(port = "B")
	RGB = RGB_LED(port = "B")

	Btn.add_event_detect(up_falling = up)
	command = ' '
	language = 'en'
	listen = "arecord -D plughw:1,0 -f cd -t wav -d 3 -r 16000 | flac - -f --best --sample-rate 16000 -o /dev/shm/out.flac 1>/dev/shm/voice.log 2>/dev/shm/voice.log; " + 'curl -X POST --data-binary @/dev/shm/out.flac' + " --user-agent 'Mozilla/5.0' --header 'Content-Type: audio/x-flac; rate=16000;' " + '"https://www.google.com/speech-api/v2/recognize?output=json&lang=' + language + '&key=AIzaSyBOti4mM-6x9WDnZIjIeyEU21OpBXqWBgw&client=Mozilla/5.0"' + " | sed -e 's/[{}]/''/g' | awk" + ' -F":"' + " '{print $4}' | awk " + '-F"," ' + "'{print $1}' | tr -d '\n'"
	
def up(chn):
	global command
	print 'Say the command'
	status, command=commands.getstatusoutput(listen)
	print command
	command =  command.split('"')[1]
	print command

def findValue(sentens, word):
	try:
		if sentens.index(word) >= 0:
			return True
	except:
		return False

def main():
	global command
	
	while True:
		#print command.strip()
		#print 'command = ' + command
		if findValue(command, 'light'):
			if findValue(command, 'on'):
				print 'light ON'
				RGB.rgb(100, 100, 100)
				command = ''
			elif command.index('off') >= 0:
				print 'light OFF'
				RGB.rgb(0, 0, 0)
				command = ''
			else:
				print "Not on or off"
		else:
			print 'Unknow command'
		time.sleep(1)
	
def destroy():
	RGB.destroy()
	Btn.destroy()
	GPIO.cleanup()
	
if __name__ == "__main__":
	try:
		setup()
		main()
	except KeyboardInterrupt:
		destroy()
	
#arecord -D plughw:1,0 -f cd -t wav -d 3 -r 16000 | flac - -f --best --sample-rate 16000 -o /dev/shm/out.flac 1>/dev/shm/voice.log 2>/dev/shm/voice.log; curl -X POST --data-binary @/dev/shm/out.flac --user-agent 'Mozilla/5.0' --header 'Content-Type: audio/x-flac; rate=16000;' "https://www.google.com/speech-api/v2/recognize?output=json&lang=zh&key=AIzaSyBOti4mM-6x9WDnZIjIeyEU21OpBXqWBgw&client=Mozilla/5.0" | sed -e 's/[{}]/''/g' | awk -F":" '{print $4}' | awk -F"," '{print $1}' | tr -d '\n'
