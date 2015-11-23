#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <memory.h>
#include <sys/time.h>
#include <limits.h>

#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_shadow_interface.h"
#include "aws_iot_shadow_json_data.h"
#include "aws_iot_config.h"
#include "aws_iot_mqtt_interface.h"

int num = 0;
uint8_t forward = 1;
uint8_t backward = 2;
uint8_t left = 3;
uint8_t right = 4;
uint8_t standup = 6;
uint8_t sitdown = 7;

int read_file(uint8_t *pbotStatusValue, char *plightValue, char *pmusicValue){
	char invalue[50];
	char *value = invalue;
	
	char valueName[20];
	char *valueValue;
	int valueNameSize;
	char *valueNameBegin;
	char *valueNameEnd;
	char *va;
	
	FILE *fp;
	char fileDir[] = "./temp/";
	char fileName[5];
	
	sprintf(fileName, "%d", num);
	strcat(fileDir, fileName);
	
	if((fp = fopen(fileDir, "r")) == NULL){
		return -1;
	}
	fgets(value, 50, fp);
	fclose(fp);
	
	valueValue = strstr(value, "=");
	valueNameEnd = strstr(value, "=");
	valueNameBegin = value;
	valueNameSize = valueNameEnd - valueNameBegin;
	valueValue ++;
	va = valueNameBegin;
	strncpy(valueName, va, valueNameSize);
	if		(strcmp(valueName, "botStatus") == 0) *pbotStatusValue = atoi(valueValue);
	else if	(strcmp(valueName, "light") == 0) strcpy(plightValue, valueValue);
	else if	(strcmp(valueName, "Music") == 0) strcpy(pmusicValue, valueValue);
	
	return 0;
}


void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
		const char *pReceivedJsonDocument, void *pContextData) {

	if (status == SHADOW_ACK_TIMEOUT) {
		INFO("Update Timeout--");
	} else if (status == SHADOW_ACK_REJECTED) {
		INFO("Update RejectedXX");
	} else if (status == SHADOW_ACK_ACCEPTED) {
		INFO("Update Accepted !!");
	}
}


char certDirectory[PATH_MAX + 1] = "../../certs";
char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;
uint8_t numPubs = 5;

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200


int main(int argc, char** argv) {
	char rmcmd[] = "rm ./temp/";
	char fileName[5];
	
	int FileError;	// No file read
	
	IoT_Error_t rc = NONE_ERROR;

	uint8_t botStatusValue = sitdown;
	char lightValue[] = "off";
	char musicValue[] = "off";

	MQTTClient_t mqttClient;
	aws_iot_mqtt_init(&mqttClient);

	char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
	size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
	char *pJsonStringToUpdate;

	jsonStruct_t botStatusHandler;
	botStatusHandler.cb = NULL;
	botStatusHandler.pKey = "botStatusValue";
	botStatusHandler.pData = &botStatusValue;
	botStatusHandler.type = SHADOW_JSON_UINT8;	// Notice

	jsonStruct_t lightHandler;
	lightHandler.cb = NULL;
	lightHandler.pKey = "lightValue";
	lightHandler.pData = &lightValue;
	lightHandler.type = SHADOW_JSON_STRING;

	jsonStruct_t musicHandler;
	musicHandler.cb = NULL;
	musicHandler.pKey = "musicValue";
	musicHandler.pData = &musicValue;
	musicHandler.type = SHADOW_JSON_STRING;
	
	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char CurrentWD[PATH_MAX + 1];
	char cafileName[] = AWS_IOT_ROOT_CA_FILENAME;
	char clientCRTName[] = AWS_IOT_CERTIFICATE_FILENAME;
	char clientKeyName[] = AWS_IOT_PRIVATE_KEY_FILENAME;

	INFO("\nAWS IoT SDK Version(dev) %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	getcwd(CurrentWD, sizeof(CurrentWD));
	sprintf(rootCA, "%s/%s/%s", CurrentWD, certDirectory, cafileName);
	sprintf(clientCRT, "%s/%s/%s", CurrentWD, certDirectory, clientCRTName);
	sprintf(clientKey, "%s/%s/%s", CurrentWD, certDirectory, clientKeyName);

	DEBUG("Using rootCA %s", rootCA);
	DEBUG("Using clientCRT %s", clientCRT);
	DEBUG("Using clientKey %s", clientKey);

	ShadowParameters_t sp = ShadowParametersDefault;
	sp.pMyThingName = AWS_IOT_MY_THING_NAME;
	sp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	sp.pHost = HostAddress;
	sp.port = port;
	sp.pClientCRT = clientCRT;
	sp.pClientKey = clientKey;
	sp.pRootCA = rootCA;

	INFO("Shadow Init");
	rc = aws_iot_shadow_init(&mqttClient);

	INFO("Shadow Connect");
	rc = aws_iot_shadow_connect(&mqttClient, &sp);

	if (NONE_ERROR != rc) {
		ERROR("Shadow Connection Error %d", rc);
	}

	while (NONE_ERROR == rc) {
		rc = aws_iot_shadow_yield(&mqttClient, 200);
		
		INFO("\n=======================================================================================\n");
		INFO("On Device: \n    Robot status: %d\n    Light status: %s\n    Music status: %s\n", botStatusValue, lightValue, musicValue);
		FileError = read_file(&botStatusValue, &lightValue, &musicValue);
		if (FileError == 0){
			rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
			if (rc == NONE_ERROR) {
				rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 3, &botStatusHandler, &lightHandler, &musicHandler);
				if (rc == NONE_ERROR) {
					rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
					if (rc == NONE_ERROR) {
						INFO("Update Shadow: %s", JsonDocumentBuffer);
						rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer, ShadowUpdateStatusCallback,
						NULL, 4, true);
						
						if (rc == NONE_ERROR) {
							printf("done");
							sprintf(fileName, "%d", num);
							strcat(rmcmd, fileName);
							system(rmcmd);
							num ++;
						}
					}
				}
			}
			INFO("*****************************************************************************************\n");
		}
		else if (FileError == -1) printf("No Command. Waiting.....");
		sleep(0.1);
	}

	if (NONE_ERROR != rc) {
		ERROR("An error occurred in the loop %d", rc);
	}

	INFO("Disconnecting");
	rc = aws_iot_shadow_disconnect(&mqttClient);

	if (NONE_ERROR != rc) {
		ERROR("Disconnect error %d", rc);
	}

	return rc;
}
