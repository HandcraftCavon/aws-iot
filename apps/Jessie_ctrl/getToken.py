from SunFounder_PiPlus import *


client_id = "PYG0eKAwDuyXspmHyLzFqXVY"
client_secret = "a51740e688ee2ba31ab859c484f3563e"
listen = 'curl -s "https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s"' % (client_id, client_secret)
status, command=commands.getstatusoutput(listen)
print command

token = command.split(",")
token = token[0].split(':')
token = token[1].strip('"')
print token
